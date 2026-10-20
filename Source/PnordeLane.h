#pragma once

// One model for every optional PNORDE lane. MIDI rendering and the editor use
// this shared type definition; the legacy physical row indices remain stable.
// No allocation, virtual dispatch or string manipulation is needed on audio thread.
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
    static constexpr int firstRow = 7;
    static constexpr int rowCount = 14;
    static constexpr bool unique(Type kind) noexcept { return kind != Type::ccMacro; }
    static constexpr int cc(Type kind) noexcept
    {
        switch (kind)
        {
            case Type::modWheel: return 1;
            case Type::expression: return 11;
            case Type::vibrato: return 76; // second axis is CC77
            case Type::pan: return 10;
            case Type::sustain: return 64;
            default: return -1;
        }
    }
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
