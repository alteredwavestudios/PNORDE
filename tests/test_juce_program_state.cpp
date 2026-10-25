// ============================================================================
// PNORDE - OPTIONAL JUCE PROGRAM STATE INTEGRATION TEST (WINDOWS)
// ============================================================================
// Why is this different from the other tests?
//
// The earlier C++ tests exercise individual calculations. They cannot tell
// whether loading a whole Ableton project works, because that requires JUCE's
// actual AudioProcessor and AudioProcessorValueTreeState (APVTS) objects.
//
// This executable creates REAL PNORDE processors in memory (without Ableton),
// stores different patterns in the four Quick Programs, serialises the bank,
// loads it into another instance, then visits all four programs again.
//
// Tests are OPT-IN (PNORDE_BUILD_PROGRAM_STATE_TESTS=ON) so the default VST3
// build remains unchanged. A real Ableton test is still needed afterwards.
// ============================================================================

#include "../Source/PluginProcessor.h"
#include "../Source/PnordeStateLimits.h"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
// "require" means "stop the test and say what failed if this is false".
void require(bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

void checkProgram(PolyrhythmMIDIProcessor& processor, int index,
                  int expectedPitch, int expectedChance, int expectedRotation)
{
    processor.selectProgram(index);
    require(processor.getSelectedProgram() == index,
            "Quick Program selection did not commit: " + std::to_string(index + 1));
    require(processor.getLaneValue(1, 0) == expectedPitch,
            "Wrong PITCH value in Quick Program " + std::to_string(index + 1));
    require(processor.getCellProbability(1, 0) == expectedChance,
            "Wrong cell probability in Quick Program " + std::to_string(index + 1));
    require(processor.getLaneRotation(1) == expectedRotation,
            "Wrong non-destructive rotation in Quick Program " + std::to_string(index + 1));
}
}

int main()
{
    try
    {
        // JUCE creates its "message thread" here. PNORDE deliberately
        // restores Quick Programs on this thread, never from processBlock().
        juce::ScopedJuceInitialiser_GUI initialiseJuce;

        PolyrhythmMIDIProcessor original;
        const int registered = original.getParameters().size();
        std::cout << "Registered host parameters: " << registered << '\n';
        require(PnordeStateLimits::hasExpansionRoom(registered),
                "Too many automation parameters for the structural safety ceiling");

        // Give each scene visibly different data. If a change loads the wrong
        // snapshot or silently discards one, the values will not match.
        const int pitches[] = {10, 20, 30, 40};
        const int chances[] = {31, 42, 53, 64};
        const int rotations[] = {0, 1, 2, 3};
        for (int program = 0; program < 4; ++program)
        {
            original.selectProgram(program);
            require(original.getSelectedProgram() == program,
                    "Could not create Quick Program " + std::to_string(program + 1));
            original.setLaneValue(1, 0, pitches[program]);
            original.setCellProbability(1, 0, chances[program]);
            original.setLaneRotation(1, rotations[program]);
        }

        // This is the normal AU/VST host save callback. It must store the
        // selected program AND all four musical snapshots.
        juce::MemoryBlock bank;
        original.getStateInformation(bank);
        require(!bank.isEmpty(), "Host save returned an empty data block");

        // This is the exact validated restore method used by the plugin.
        PolyrhythmMIDIProcessor restored;
        require(restored.loadValidatedState(bank.getData(),
                                             static_cast<int>(bank.getSize())),
                "VALID four-program bank rejected by state loader");
        require(restored.getSelectedProgram() == 3,
                "Saved active program was not restored");

        // Visit programs in a non-sequential order, then again in reverse.
        // This catches state leak/snap-back that a single P1->P2 test misses.
        for (int index : {0, 3, 1, 2, 0, 2, 3, 1})
            checkProgram(restored, index, pitches[index], chances[index], rotations[index]);

        // v0.6.29: Play is GLOBAL, not part of any Quick Program snapshot.
        // This catches the exact P4-pauses-when-revisited bug, including
        // the optimised internal ValueTree restore path.
        restored.setPlayEnabled(false);
        restored.selectProgram(0);
        require(!restored.getPlayEnabled(),
                "Changing to P1 unexpectedly resumed playback");
        restored.setPlayEnabled(true);
        restored.selectProgram(3);
        require(restored.getPlayEnabled(),
                "Returning to P4 unexpectedly paused playback");
        restored.selectProgram(1);
        require(restored.getPlayEnabled(),
                "Another Quick Program overwrote global Play state");

        // Corrupt ONE nested scene while leaving the outer bank well-formed.
        // The state loader must validate EVERY saved program before touching
        // any current values, even programs that are not presently selected.
        auto corruptXml = std::unique_ptr<juce::XmlElement>(
            juce::AudioProcessor::getXmlFromBinary(bank.getData(),
                static_cast<int>(bank.getSize())));
        require(corruptXml != nullptr, "Could not decode freshly saved XML");
        corruptXml->setAttribute("PnordeProgramSlot1", "not_valid_base64!!");
        juce::MemoryBlock brokenBank;
        juce::AudioProcessor::copyXmlToBinary(*corruptXml, brokenBank);
        PolyrhythmMIDIProcessor protectedProcessor;
        protectedProcessor.setLaneValue(1, 0, 75);
        require(!protectedProcessor.loadValidatedState(brokenBank.getData(),
                 static_cast<int>(brokenBank.getSize())),
                "Bank with a corrupt nested scene was accepted");
        require(protectedProcessor.getLaneValue(1, 0) == 75,
                "Rejected nested scene changed the current musical pattern");

        // Simulate the old PRE-BANK format by removing the newer bank-only
        // attributes. It should be accepted as a single-scene legacy preset.
        auto legacyXml = std::unique_ptr<juce::XmlElement>(
            juce::AudioProcessor::getXmlFromBinary(bank.getData(),
                static_cast<int>(bank.getSize())));
        require(legacyXml != nullptr, "Could not prepare legacy-style test XML");
        legacyXml->removeAttribute("PnordeProgramSelected");
        for (int index = 0; index < 4; ++index)
            legacyXml->removeAttribute("PnordeProgramSlot" + juce::String(index));
        juce::MemoryBlock legacyState;
        juce::AudioProcessor::copyXmlToBinary(*legacyXml, legacyState);
        PolyrhythmMIDIProcessor legacyProcessor;
        require(legacyProcessor.loadValidatedState(legacyState.getData(),
                static_cast<int>(legacyState.getSize())),
                "Single-scene legacy-style state was rejected");
        require(legacyProcessor.getSelectedProgram() == 0,
                "Legacy state did not migrate to Program 1");

        // A broken/corrupted state should FAIL CLOSED (return false), WITHOUT
        // modifying existing live musical values. This is a major requirement.
        juce::MemoryBlock garbage;
        const char nonsense[] = "not a JUCE plugin state";
        garbage.append(nonsense, sizeof(nonsense));
        restored.setLaneValue(1, 0, 70);
        require(!restored.loadValidatedState(garbage.getData(),
                                             static_cast<int>(garbage.getSize())),
                "Malformed state was accepted");
        require(restored.getLaneValue(1, 0) == 70,
                "Rejected state modified a live lane");

        // The v0.6.25 parameter IDs must be unchanged by this maintenance
        // patch, especially the host-visible Quick Program automation ID.
        require(original.parameters.getParameter("pn_program_slot") != nullptr,
                "Lost pn_program_slot host automation parameter");

        std::cout << "PASS: real JUCE save/load, four isolated programs, "
                     "rotation/probability persistence, legacy migration and "
                     "invalid-state rejection\n";
        return 0;
    }
    catch (const std::exception& failure)
    {
        std::cerr << "FAIL: " << failure.what() << '\n';
        return 1;
    }
}
