// ============================================================================
// PnordeSafeCC.h — PNORDE SafeCC
// ============================================================================
// This helper isolates one part of the sequencer so it can be understood and tested without reading the whole plugin.
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

// Conservative, centrally enforced selection of assignable CC macros.
// BEGINNER NOTE: A custom Macro is DIFFERENT from a dedicated lane.
// The MOD WHEEL lane always uses CC1, whereas a user-selectable CC Macro
// can be assigned a number from the safe range below. The dedicated map
// lives in PnordeLane.h; this file is intentionally only about CC Macros.
// `sanitise()` means "replace an unsupported number with our safe default".
// This is a pure function: same input -> same output, with no side effects.
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
