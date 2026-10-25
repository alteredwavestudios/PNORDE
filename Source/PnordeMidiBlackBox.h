// ============================================================================
// PnordeMidiBlackBox.h — Optional MIDI flight recorder
// ============================================================================
// Tracks a bounded history of MIDI emitted by PNORDE. It cannot observe a receiving synth voice, volume, or host CPU. Audio-thread writes must remain allocation-free.
//
// Beginner notes:
//   - A header (.h) usually DECLARES names, types and helper functions.
//   - A source file (.cpp) usually IMPLEMENTS behaviour.
//   - `noexcept` promises an operation will not throw C++ exceptions.
//   - `const` generally indicates a value cannot be modified through that use.
//   - Avoid changing audio-thread behaviour merely to add documentation.
// See docs/SOURCE_GUIDE.md for terms, architecture and call-flow diagrams.
// ============================================================================

#pragma once
// PNORDE v0.6.29: MIDI flight recorder (plain C++17; NO JUCE dependency).
//
// This is an OUTPUT observer, NOT a MIDI effect. No allocations, mutexes,
// IO, formatting, or waits in the audio callback. It observes MIDI just
// before PNORDE sends it to Ableton / the external MIDI queue. It cannot
// measure the receiving synth's voice state or actual loudness.
//
// The AUDIO thread is the only writer. UI is a read-only observer. Each
// field is atomic to prevent data races even when the 512-event ring wraps.
// A sequence stamp guards against reading part of two different events.
#include <array>
#include <atomic>
#include <cstdint>

namespace PnordeMidiBlackBox {
struct Event {
    std::uint64_t frame = 0;  // PNORDE audio sample frame count, not wall time
    std::uint8_t status = 0;
    std::uint8_t data1 = 0;
    std::uint8_t data2 = 0;
    std::uint8_t marker = 0; // 1=PNORDE Play off, 2=host stop, 4=Quick Program switched
};
struct Summary {
    std::uint64_t emittedNoteOns=0, emittedNoteOffs=0, unmatchedNoteOffs=0;
    std::uint64_t emittedCC64On=0, emittedCC64Off=0;
    std::uint64_t emittedCC120=0, emittedCC123=0;
    std::uint64_t otherCCs=0, pitchWheels=0, droppedTraceEvents=0;
    int outstandingKeys=0;
    bool frozen=false;
    std::uint64_t totalEvents=0, stopFrame=0;
    int stopKind=0;
};
class Recorder {
public:
    static constexpr unsigned capacity=512;
    // Called by UI; producer handles reset at the start of an audio callback.
    void requestReset() noexcept { resetGeneration.fetch_add(1, std::memory_order_release); }
    void beginBlock(int samples) noexcept {
        const auto wanted=resetGeneration.load(std::memory_order_acquire);
        if (wanted != appliedGeneration) {
            appliedGeneration=wanted; writeCount=0; audioFrame=0; hold=false;
            activeNotes.fill(0); outstanding=0;
            noteOns.store(0); noteOffs.store(0); unmatched.store(0);
            cc64On.store(0); cc64Off.store(0); cc120.store(0); cc123.store(0);
            otherCC.store(0); pitchCount.store(0); overwritten.store(0);
            stopFrame.store(0); stopKind.store(0);
            publishedCount.store(0, std::memory_order_release);
            frozen.store(false, std::memory_order_release);
        }
        blockBase=audioFrame;
        audioFrame += samples > 0 ? static_cast<unsigned>(samples) : 0u;
    }
    // Short voice/CC messages are all that PNORDE emits through emit().
    void recordMidi(const std::uint8_t* bytes, int length, int offset) noexcept {
        if (hold || bytes == nullptr || length < 1) return;
        const auto status=bytes[0];
        const auto type=status & 0xf0;
        const auto channel=status & 0x0f;
        const auto d1=length>1 ? bytes[1] : 0;
        const auto d2=length>2 ? bytes[2] : 0;
        bool capture=false;
        if ((type==0x90 && d2>0) && d1<128) {
            noteOns.fetch_add(1, std::memory_order_relaxed);
            auto& count=activeNotes[static_cast<unsigned>(channel)*128u+d1];
            if (count<255) {if (count++==0) ++outstanding;}
            capture=true;
        } else if ((type==0x80 || (type==0x90 && d2==0)) && d1<128) {
            noteOffs.fetch_add(1, std::memory_order_relaxed);
            auto& count=activeNotes[static_cast<unsigned>(channel)*128u+d1];
            if (count>0) { if (--count==0) --outstanding; }
            else unmatched.fetch_add(1, std::memory_order_relaxed);
            capture=true;
        } else if (type==0xb0) {
            if (d1==64) {if (d2>=64) cc64On.fetch_add(1); else cc64Off.fetch_add(1); capture=true;}
            else if (d1==120) {
                cc120.fetch_add(1); capture=true;
                // All Sound Off is a panic command; treat PNORDE's keys as
                // cleared for approximate note accounting, not synth proof.
                for (unsigned note=0;note<128;++note) {
                    auto& count=activeNotes[static_cast<unsigned>(channel)*128u+note];
                    if (count>0) {count=0; --outstanding;}
                }
            } else if (d1==123) {cc123.fetch_add(1);capture=true;}
            else {otherCC.fetch_add(1, std::memory_order_relaxed); capture=(d1==7 || d1==11);}
        }
        if (type==0xe0)
        {
            pitchCount.fetch_add(1, std::memory_order_relaxed);
            // Wheel curves can produce hundreds of events per second. Record
            // only the neutral resets in the ring; count every wheel event.
            if (d1==0 && d2==64) capture=true;
        }
        if (capture) append({blockBase+static_cast<std::uint64_t>(offset<0?0:offset),
                             status,static_cast<std::uint8_t>(d1),static_cast<std::uint8_t>(d2),0});
        outstandingPublic.store(outstanding,std::memory_order_relaxed);
    }
    // Message thread posts a tiny atomic marker. Only the audio thread
    // writes to this ring, so there is never a two-writer race.
    void markProgram(int from, int to) noexcept {
        if (hold) return;
        append({blockBase, 0, static_cast<std::uint8_t>(from+1),
            static_cast<std::uint8_t>(to+1), 4});
    }
    // Called AFTER stop-triggered output has been emitted. The trace freezes
    // until reset; a future restart cannot overwrite evidence of a loud stop.
    void markStop(int kind) noexcept {
        if (hold) return;
        append({blockBase,0,0,0,static_cast<std::uint8_t>(kind)});
        stopFrame.store(blockBase,std::memory_order_relaxed);
        stopKind.store(kind,std::memory_order_relaxed);
        hold=true;
        frozen.store(true,std::memory_order_release);
    }
    Summary summary() const noexcept {
        Summary s;
        s.emittedNoteOns=noteOns.load(); s.emittedNoteOffs=noteOffs.load();
        s.unmatchedNoteOffs=unmatched.load(); s.emittedCC64On=cc64On.load();
        s.emittedCC64Off=cc64Off.load(); s.emittedCC120=cc120.load();
        s.emittedCC123=cc123.load(); s.otherCCs=otherCC.load();
        s.pitchWheels=pitchCount.load();
        s.droppedTraceEvents=overwritten.load(); s.outstandingKeys=outstandingPublic.load();
        s.frozen=frozen.load(std::memory_order_acquire);
        s.totalEvents=publishedCount.load(std::memory_order_acquire);
        s.stopFrame=stopFrame.load(); s.stopKind=stopKind.load();
        return s;
    }
    // Read only on message/UI thread. Returns false if writer wrapped this
    // slot during a read, or the event is older than the retained ring.
    bool read(std::uint64_t serial, Event& out) const noexcept {
        const auto total=publishedCount.load(std::memory_order_acquire);
        if (serial>=total || total-serial>capacity) return false;
        const Slot& slot=slots[static_cast<unsigned>(serial)%capacity];
        const auto expected=serial*2+2;
        if (slot.stamp.load(std::memory_order_acquire)!=expected) return false;
        Event e;
        e.frame=slot.frame.load(std::memory_order_relaxed);
        const auto word=slot.midi.load(std::memory_order_relaxed);
        e.status=static_cast<std::uint8_t>(word);
        e.data1=static_cast<std::uint8_t>(word>>8);
        e.data2=static_cast<std::uint8_t>(word>>16);
        e.marker=static_cast<std::uint8_t>(word>>24);
        if (slot.stamp.load(std::memory_order_acquire)!=expected) return false;
        out=e;return true;
    }
private:
    struct Slot {
        std::atomic<std::uint64_t> stamp {0};
        std::atomic<std::uint64_t> frame {0};
        std::atomic<std::uint32_t> midi {0};
    };
    void append(Event e) noexcept {
        const auto serial=writeCount++;
        auto& slot=slots[static_cast<unsigned>(serial)%capacity];
        slot.stamp.store(serial*2+1,std::memory_order_release);
        slot.frame.store(e.frame,std::memory_order_relaxed);
        const std::uint32_t packed=e.status|(std::uint32_t(e.data1)<<8)
                             |(std::uint32_t(e.data2)<<16)|(std::uint32_t(e.marker)<<24);
        slot.midi.store(packed,std::memory_order_relaxed);
        slot.stamp.store(serial*2+2,std::memory_order_release);
        if (serial>=capacity) overwritten.fetch_add(1,std::memory_order_relaxed);
        publishedCount.store(serial+1,std::memory_order_release);
    }
    std::array<Slot,capacity> slots{};
    std::array<std::uint8_t,16*128> activeNotes{}; // audio thread only
    int outstanding=0; std::uint64_t writeCount=0, audioFrame=0, blockBase=0;
    bool hold=false;
    unsigned appliedGeneration=0;
    std::atomic<unsigned> resetGeneration{0};
    std::atomic<std::uint64_t> publishedCount{0},stopFrame{0};
    std::atomic<std::uint64_t> noteOns{0},noteOffs{0},unmatched{0};
    std::atomic<std::uint64_t> cc64On{0},cc64Off{0},cc120{0},cc123{0},otherCC{0},pitchCount{0},overwritten{0};
    std::atomic<int> outstandingPublic{0},stopKind{0};
    std::atomic<bool> frozen{false};
};
} // namespace PnordeMidiBlackBox
