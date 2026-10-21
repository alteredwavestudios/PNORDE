#pragma once
#include <cstdint>

// A PROGRAM lane request is valid only if no manual program selection occurred
// after that audio-block request was generated. Pure integer operations: safe
// in the audio callback. Zero is the empty mailbox.
namespace PnordeProgramRequests
{
    inline std::uint64_t pack(std::uint32_t manualEpoch, int programOneBased) noexcept
    {
        return (programOneBased >= 1 && programOneBased <= 4)
            ? (static_cast<std::uint64_t>(manualEpoch) << 3)
                | static_cast<std::uint64_t>(programOneBased) : 0;
    }
    inline bool unpack(std::uint64_t packet, std::uint32_t latestManualEpoch,
                       int& zeroBasedProgram) noexcept
    {
        if (packet == 0 || (packet >> 3) != latestManualEpoch) return false;
        const auto slot = static_cast<int>(packet & 7u);
        if (slot < 1 || slot > 4) return false;
        zeroBasedProgram = slot - 1;
        return true;
    }
}
