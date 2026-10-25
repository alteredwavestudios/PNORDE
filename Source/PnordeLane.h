// ============================================================================
// PnordeLane.h — Lane types and MIDI mappings
// ============================================================================
// Defines the data and meanings of lane kinds, MIDI controller mappings, and related utility functions.
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

// ============================================================================
// BEGINNER'S TOUR: WHAT IS A "LANE TYPE"?
// ============================================================================
// PNORDE has several optional rows underneath its main sequencing controls.
// An optional row can behave as a custom CC Macro, a modulation wheel, a
// sustain pedal, and so on. The Type enumeration below gives each behaviour
// a READABLE NAME instead of scattering mysterious numbers throughout C++.
//
// `enum class` is a group of named integer constants. For example,
// Type::modWheel is the name for the role whose saved integer value is 1.
// `static constexpr` means the answer is known at compile time and does not
// depend on an instance of PnordeLane existing at runtime.
// `noexcept` promises this little mapping function cannot throw an exception.
//
// IMPORTANT: The integer values, row positions, and MIDI controller numbers
// form part of PNORDE's EXISTING behaviour. Do not renumber these casually:
// older Ableton projects rely on being able to recover the same lane roles.
// All lookups below are allocation-free, appropriate for the audio thread.
// ============================================================================
struct PnordeLane final
{
    enum class Type : int
    {
        ccMacro = 0,
        modWheel = 1,
        expression = 2,
        vibrato = 3,
        pan = 4,
        sustain = 5,
        midiChannel = 6
    };

    // Physical optional rows 7..20. These are not MIDI note numbers; they are
    // positions in PNORDE's internal lane array. Existing saves use them.
    static constexpr int firstRow = 7;
    static constexpr int rowCount = 14;
    // At most one lane can own most dedicated roles, whereas multiple generic
    // Macro rows can be assigned different user-selected CC numbers.
    static constexpr bool unique(Type kind) noexcept { return kind != Type::ccMacro; }

    // Standard MIDI Control Change messages have a controller number (0..127)
    // and a value (0..127). The number identifies WHICH control is adjusted;
    // the value tells the receiving instrument HOW FAR to move that control.
    //
    // This function is the single source of truth for dedicated controller
    // numbers. Both the on-screen tooltip and the MIDI sending code use it.
    // -1 means there is no fixed controller for this role (e.g. CC Macro has
    // its own user-selected number, and MIDI Channel Select is not a CC).
    static constexpr int cc(Type kind) noexcept
    {
        switch (kind)
        {
            case Type::modWheel: return 1;
            case Type::expression: return 11;
            case Type::vibrato: return 76; // horizontal depth axis uses CC77
            case Type::pan: return 10;
            case Type::sustain: return 64;
            default: return -1;
        }
    }
    // The vibrato lane controls TWO axes; its second axis uses CC77.
    // Centralise this too, instead of hiding another number in the processor.
    static constexpr int vibratoDepthCC = 77;

    // `defaultType` answers: "which role should optional row N begin as?"
    // This is a DEFAULT, not a restriction: optional rows can be reassigned.
    // The fall-through default (CC Macro) is deliberate for rows 7..14.
    static constexpr Type defaultType(int row) noexcept
    {
        switch (row)
        {
            case 15: return Type::modWheel;
            case 16: return Type::expression;
            case 17: return Type::vibrato;
            case 18: return Type::pan;
            case 19: return Type::sustain;
            case 20: return Type::midiChannel;
            default: return Type::ccMacro;
        }
    }
    // Short human-readable names for menus / tooltips. `const char*` is a
    // pointer to fixed text stored in the program; nothing is allocated here.
    static const char* label(Type kind) noexcept
    {
        switch (kind)
        {
            case Type::modWheel: return "MOD WHEEL";
            case Type::expression: return "EXPRESSION PEDAL";
            case Type::vibrato: return "VIBRATO";
            case Type::pan: return "PAN";
            case Type::sustain: return "SUSTAIN PEDAL";
            case Type::midiChannel: return "MIDI CHANNEL SELECT";
            default: return "CC MACRO";
        }
    }
};

// These assertions are checked when the C++ compiler builds PNORDE. They are
// also a safeguard against an accidental change to the historical MIDI map.
// If one fails, compilation stops before a broken plugin can be installed.
static_assert(PnordeLane::cc(PnordeLane::Type::modWheel) == 1, "Mod wheel is CC1");
static_assert(PnordeLane::cc(PnordeLane::Type::expression) == 11, "Expression is CC11");
static_assert(PnordeLane::cc(PnordeLane::Type::vibrato) == 76, "Vibrato speed is CC76");
static_assert(PnordeLane::vibratoDepthCC == 77, "Vibrato depth is CC77");
static_assert(PnordeLane::cc(PnordeLane::Type::pan) == 10, "Pan is CC10");
static_assert(PnordeLane::cc(PnordeLane::Type::sustain) == 64, "Sustain is CC64");
static_assert(PnordeLane::cc(PnordeLane::Type::ccMacro) == -1, "Macros choose their CC");
static_assert(PnordeLane::cc(PnordeLane::Type::midiChannel) == -1, "Channels aren't CC");
