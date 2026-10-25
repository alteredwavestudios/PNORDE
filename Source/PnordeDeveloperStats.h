// ============================================================================
// PnordeDeveloperStats.h — Opt-in performance counters
// ============================================================================
// Stores elapsed timings and event counts as atomic counters so the UI can read approximate snapshots without blocking the audio callback.
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
// PNORDE Developer Tools - plain-C++ counters, independent of JUCE.
//
// Every statistic is optional. With Developer Mode OFF the audio path does
// not perform timing calls or increment diagnostic counters. The real-time
// callback NEVER formats strings, prints to a console, allocates log entries,
// or asks the message thread to do anything.
//
// A snapshot is an approximate, thread-safe observation; numbers gathered
// across separate atomics are NOT an indivisible instant in time.
#include <atomic>
#include <cstdint>

namespace PnordeDeveloperStats
{
struct Snapshot
{
    std::uint64_t audioBlocks = 0;
    std::uint64_t audioTotalUs = 0;
    std::uint64_t audioPeakUs = 0;
    std::uint64_t midiMessages = 0;
    std::uint64_t midiPeakPerBlock = 0;
    std::uint64_t programRequests = 0;
    std::uint64_t overwrittenRequests = 0;
    std::uint64_t programSwitches = 0;
    std::uint64_t programRestoreFailures = 0;
    std::uint64_t lastQueueUs = 0;
    std::uint64_t lastRestoreUs = 0;
    std::uint64_t lastCaptureUs = 0; // Saving the outgoing Quick Program
    std::uint64_t lastPrepareUs = 0; // Parsing/adjusting incoming scene
    std::uint64_t lastLoadUs = 0;    // Validating/applying prepared scene
    std::uint64_t lastParameterReplaceUs = 0; // JUCE host-parameter tree replacement
    std::uint64_t restoreTotalUs = 0;
    std::uint64_t restorePeakUs = 0;
    std::uint64_t queueTotalUs = 0;
    std::uint64_t queuePeakUs = 0;
    int lastFrom = -1;
    int lastTo = -1;
    int lastSource = 0; // 0=none, 1=program lane, 2=manual/host
};

class Counters
{
public:
    bool enabled() const noexcept { return measuring.load(std::memory_order_relaxed); }
    void setEnabled(bool on) noexcept
    {
        // Not a host parameter or preset property. The instrumentation is
        // temporary and never changes actual musical or project state.
        measuring.store(on, std::memory_order_release);
    }
    void reset() noexcept
    {
        audioBlocks.store(0); audioTotalUs.store(0); audioPeakUs.store(0);
        midiMessages.store(0); midiPeakPerBlock.store(0); programRequests.store(0);
        overwrittenRequests.store(0); programSwitches.store(0);
        programRestoreFailures.store(0); lastQueueUs.store(0);
        lastRestoreUs.store(0); lastCaptureUs.store(0);
        lastPrepareUs.store(0); lastLoadUs.store(0);
        lastParameterReplaceUs.store(0);
        restoreTotalUs.store(0); restorePeakUs.store(0);
        queueTotalUs.store(0); queuePeakUs.store(0);
        lastFrom.store(-1); lastTo.store(-1);
        lastSource.store(0);
    }
    void recordAudioBlock(std::uint64_t us, std::uint64_t midi) noexcept
    {
        if (!enabled()) return;
        audioBlocks.fetch_add(1, std::memory_order_relaxed);
        audioTotalUs.fetch_add(us, std::memory_order_relaxed);
        midiMessages.fetch_add(midi, std::memory_order_relaxed);
        updateMaximum(midiPeakPerBlock, midi);
        auto peak = audioPeakUs.load(std::memory_order_relaxed);
        while (us > peak && !audioPeakUs.compare_exchange_weak(
                   peak, us, std::memory_order_relaxed)) {}
    }
    void recordRequest(bool overwrote) noexcept
    {
        if (!enabled()) return;
        programRequests.fetch_add(1, std::memory_order_relaxed);
        if (overwrote) overwrittenRequests.fetch_add(1, std::memory_order_relaxed);
    }
    void recordSwitch(int from, int to, int source,
                      std::uint64_t queueUs, std::uint64_t restoreUs,
                      bool success) noexcept
    {
        if (!enabled()) return;
        if (success)
        {
            programSwitches.fetch_add(1, std::memory_order_relaxed);
            restoreTotalUs.fetch_add(restoreUs, std::memory_order_relaxed);
            queueTotalUs.fetch_add(queueUs, std::memory_order_relaxed);
            updateMaximum(restorePeakUs, restoreUs);
            updateMaximum(queuePeakUs, queueUs);
        }
        else programRestoreFailures.fetch_add(1, std::memory_order_relaxed);
        lastFrom.store(from, std::memory_order_relaxed);
        lastTo.store(to, std::memory_order_relaxed);
        lastSource.store(source, std::memory_order_relaxed);
        lastQueueUs.store(queueUs, std::memory_order_relaxed);
        lastRestoreUs.store(restoreUs, std::memory_order_relaxed);
    }
    // Message-thread-only instrumentation; this is NOT audio callback work.
    // Durations are separated only for diagnostics. They never alter what
    // happens when a scene is restored, nor the host's saved state.
    void recordRestoreBreakdown(std::uint64_t captureUs,
                                std::uint64_t prepareUs,
                                std::uint64_t loadUs) noexcept
    {
        if (!enabled()) return;
        lastCaptureUs.store(captureUs, std::memory_order_relaxed);
        lastPrepareUs.store(prepareUs, std::memory_order_relaxed);
        lastLoadUs.store(loadUs, std::memory_order_relaxed);
    }
    void recordParameterReplace(std::uint64_t elapsedUs) noexcept
    {
        if (!enabled()) return;
        lastParameterReplaceUs.store(elapsedUs, std::memory_order_relaxed);
    }
    Snapshot snapshot() const noexcept
    {
        Snapshot s;
        s.audioBlocks=audioBlocks.load(std::memory_order_relaxed);
        s.audioTotalUs=audioTotalUs.load(std::memory_order_relaxed);
        s.audioPeakUs=audioPeakUs.load(std::memory_order_relaxed);
        s.midiMessages=midiMessages.load(std::memory_order_relaxed);
        s.midiPeakPerBlock=midiPeakPerBlock.load(std::memory_order_relaxed);
        s.programRequests=programRequests.load(std::memory_order_relaxed);
        s.overwrittenRequests=overwrittenRequests.load(std::memory_order_relaxed);
        s.programSwitches=programSwitches.load(std::memory_order_relaxed);
        s.programRestoreFailures=programRestoreFailures.load(std::memory_order_relaxed);
        s.lastQueueUs=lastQueueUs.load(std::memory_order_relaxed);
        s.lastRestoreUs=lastRestoreUs.load(std::memory_order_relaxed);
        s.lastCaptureUs=lastCaptureUs.load(std::memory_order_relaxed);
        s.lastPrepareUs=lastPrepareUs.load(std::memory_order_relaxed);
        s.lastLoadUs=lastLoadUs.load(std::memory_order_relaxed);
        s.lastParameterReplaceUs=lastParameterReplaceUs.load(std::memory_order_relaxed);
        s.restoreTotalUs=restoreTotalUs.load(std::memory_order_relaxed);
        s.restorePeakUs=restorePeakUs.load(std::memory_order_relaxed);
        s.queueTotalUs=queueTotalUs.load(std::memory_order_relaxed);
        s.queuePeakUs=queuePeakUs.load(std::memory_order_relaxed);
        s.lastFrom=lastFrom.load(std::memory_order_relaxed);
        s.lastTo=lastTo.load(std::memory_order_relaxed);
        s.lastSource=lastSource.load(std::memory_order_relaxed);
        return s;
    }
private:
    static void updateMaximum(std::atomic<std::uint64_t>& target,
                              std::uint64_t value) noexcept
    {
        auto old = target.load(std::memory_order_relaxed);
        while (old < value && !target.compare_exchange_weak(
            old, value, std::memory_order_relaxed)) {}
    }
    std::atomic<std::uint64_t> restoreTotalUs {0}, restorePeakUs {0};
    std::atomic<std::uint64_t> queueTotalUs {0}, queuePeakUs {0};
    std::atomic<bool> measuring {false};
    std::atomic<std::uint64_t> audioBlocks {0},audioTotalUs {0},audioPeakUs {0};
    std::atomic<std::uint64_t> midiMessages {0},midiPeakPerBlock {0},programRequests {0},overwrittenRequests {0};
    std::atomic<std::uint64_t> programSwitches {0},programRestoreFailures {0};
    std::atomic<std::uint64_t> lastQueueUs {0},lastRestoreUs {0};
    std::atomic<std::uint64_t> lastCaptureUs {0},lastPrepareUs {0},lastLoadUs {0};
    std::atomic<std::uint64_t> lastParameterReplaceUs {0};
    std::atomic<int> lastFrom {-1},lastTo {-1},lastSource {0};
};
} // namespace PnordeDeveloperStats
