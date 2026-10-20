#pragma once

// Conservative, centrally enforced selection of assignable CC macros.
// CC20..31 are left unassigned by the standard MIDI controller list; a
// particular instrument may nevertheless implement private mappings.
// Do not include bank select, volume, pan, sustain, data-entry/RPN/NRPN,
// dedicated PNORDE controls, or channel-mode numbers in this whitelist.
struct PnordeSafeCC final
{
    static constexpr int first = 20;
    static constexpr int last = 31;
    static constexpr int fallback = 20;
    static constexpr bool allowed(int number) noexcept
    {
        return number >= first && number <= last;
    }
    static constexpr int sanitise(int number) noexcept
    {
        return allowed(number) ? number : fallback;
    }
};
