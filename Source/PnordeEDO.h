// ============================================================================
// PnordeEDO.h — PNORDE EDO
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
#include <array>
#include <cmath>
#include <cstdint>

// PNORDE's curated EDO modes are octave-repeating 7-note subsets, not a
// cultural maqam or a replacement for arbitrary Scala tuning files.
// The five EDO sizes were chosen for useful harmonic 3-, 5-, 7- and 11-limit
// approximations. All offsets are in exact EDO steps, never approximated cents.
namespace PnordeEDO
{
constexpr int firstScaleIndex = 34; // Never change legacy indices 0..33.
struct Mode
{
    const char* name;
    int edo;
    std::array<int, 7> degrees;
};
constexpr std::array<Mode, 13> modes {{
    {"17-EDO / Subminor Twilight", 17, {{0,3,4,7,10,12,14}}},
    {"17-EDO / Neutral Nocturne", 17, {{0,3,5,7,10,12,15}}},
    {"19-EDO / Velvet Minor", 19, {{0,3,5,8,11,13,16}}},
    {"19-EDO / Bittersweet Major 7", 19, {{0,3,6,8,11,14,17}}},
    {"22-EDO / Porcupine 7", 22, {{0,3,6,9,13,16,19}}},
    {"22-EDO / Pajara-inspired", 22, {{0,4,7,9,13,16,18}}},
    {"22-EDO / Unresolved Cloud", 22, {{0,3,6,9,13,15,18}}},
    {"24-EDO / Neutral Minor", 24, {{0,4,7,10,14,17,21}}},
    {"24-EDO / Quartertone Haze", 24, {{0,3,7,10,14,17,21}}},
    {"24-EDO / Hijaz-inspired", 24, {{0,3,8,10,14,17,20}}},
    {"31-EDO / Meantone Minor", 31, {{0,5,8,13,18,21,26}}},
    {"31-EDO / Septimal Longing", 31, {{0,5,10,13,18,23,25}}},
    {"31-EDO / Neutral Dream", 31, {{0,5,9,13,18,22,27}}}
}};
constexpr int modeCount = static_cast<int>(modes.size());
constexpr bool isMode(int index) noexcept
{ return index >= firstScaleIndex && index < firstScaleIndex + modeCount; }
inline const Mode& mode(int index) noexcept
{ return modes[static_cast<std::size_t>(index-firstScaleIndex)]; } // isMode required

// Same root semantics as PNORDE's other scales: C4 at 60, key as MIDI pitch
// class, +12/-12 pitch lane range. The exact result is in semitones; the
// editable lane data remains 0.1-semitone-resolution for preset compatibility.
inline double quantiseSemitones(int valueTenths, int octave, int keyPc,
                               int scaleIndex) noexcept
{
    if (!isMode(scaleIndex)) return valueTenths / 10.0;
    if (valueTenths < -120) valueTenths = -120;
    if (valueTenths > 120) valueTenths = 120;
    if (octave < -4) octave = -4;
    if (octave > 4) octave = 4;
    if (keyPc < 0) keyPc = 0;
    if (keyPc > 11) keyPc = 11;
    const auto& m = mode(scaleIndex);
    const double base = 60.0 + octave*12.0;
    const double target = base + valueTenths/10.0;
    const int approximateOctave = static_cast<int>(std::floor((target-keyPc)/12.0));
    double best = target, minError = 1.0e12;
    for (int o=approximateOctave-2; o<=approximateOctave+2; ++o)
      for (int degree : m.degrees)
      {
          const double candidate = 12.0*o + keyPc + 12.0*degree/m.edo;
          // Respect MIDI range and existing editable pitch lane limits.
          if (candidate < 0.0 || candidate > 127.0 ||
              candidate < base-12.0 || candidate > base+12.0) continue;
          const double error = std::abs(candidate-target);
          if (error < minError)
          { minError = error; best = candidate; }
      }
    if (minError == 1.0e12) return valueTenths/10.0;
    return best-base;
}
inline int quantiseTenths(int valueTenths, int octave, int keyPc,
                         int scaleIndex) noexcept
{
    const int snapped = static_cast<int>(std::lround(
        10.0*quantiseSemitones(valueTenths,octave,keyPc,scaleIndex)));
    return snapped < -120 ? -120 : snapped > 120 ? 120 : snapped;
}

// Generate a melody in *scale degrees*, leaving gates, velocity and other
// sequencer lanes untouched. UI thread only; deterministic, allocation-free,
// and independent of the MIDI/audio callback.
enum class Mood : int { longing, bittersweet, gentleTension, homeward,
                        suspended, mirror, jazzy, ascending };
constexpr int moodCount = 8;
constexpr const char* moodNames[moodCount] = {
    "Longing / unresolved", "Bittersweet arpeggio",
    "Gentle dissonance", "Homeward resolution",
    "Floating Suspensions", "Mirror / Answering Phrases",
    "Septimal Jazz Pulse", "Restless Ascent"
};
inline std::uint32_t stepRandom(std::uint32_t seed) noexcept
{
    seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
    return seed;
}
inline int nearestDegreeToCents(const Mode& m,double cents) noexcept
{
    int best=0;
    double error=1.0e10;
    for (int d=0; d<7; ++d)
    {
        const double actual=1200.0*m.degrees[static_cast<std::size_t>(d)]/m.edo;
        const double difference=std::abs(actual-cents);
        if (difference<error) {error=difference;best=d;}
    }
    return best;
}
inline int pitchForDegree(int modeIndex,int keyPc,int degree) noexcept
{
    const auto& m=mode(modeIndex);
    const int d=m.degrees[static_cast<std::size_t>(degree%7)];
    // Generated pitches now store INTERVALS from the chosen key, not absolute
    // pitches from C. Key transposition is added by the MIDI playback engine.
    // Keeping 'keyPc' in the signature avoids changing all existing callers.
    (void) keyPc;
    double offset=12.0*d/m.edo;
    while (offset>12.0) offset-=12.0;
    while (offset< -12.0) offset+=12.0;
    const int result=static_cast<int>(std::lround(offset*10.0));
    return result<-120 ? -120 : result>120 ? 120 : result;
}
inline void generate(std::array<int,128>& pitches,int length,int keyPc,
                     int scaleIndex,Mood mood,std::uint32_t seed) noexcept
{
    if (!isMode(scaleIndex)) return;
    if (length<1) return;
    if (length>128) length=128;
    if (keyPc<0) keyPc=0;
    if (keyPc>11) keyPc=11;
    // Candidate harmonic functions in cents. Each is projected to the
    // *selected EDO mode's nearest degree*, so the generator uses the actual
    // tuning's consonances instead of recycling 12-TET semitone intervals.
    // 315c ~ 6/5, 386c ~ 5/4, 702c ~ 3/2, 969c ~ 7/4.
    static constexpr double targets[] = {
        0.0, 204.0, 315.6, 386.3, 498.0, 702.0,
        884.4, 968.8, 1088.3, 347.4
    };
    static constexpr int themes[8][16] = {
        // Each entry is a harmonic role, not a MIDI semitone or raw index.
        {0,2,5,6,5,2,4,2, 5,7,6,5,4,2,1,1},
        {0,3,5,3,6,5,8,5, 0,3,5,6,8,5,3,0},
        {0,9,4,9,1,2,5,4, 6,5,4,2,1,9,2,1},
        {8,7,6,5,4,2,1,0, 6,5,4,9,2,1,0,0},
        {4,5,4,2,4,5,7,6, 4,6,5,4,9,4,2,4},
        {0,2,5,2,4,9,5,4, 7,5,9,4,2,5,2,0},
        {0,7,3,5,8,3,9,5, 0,3,7,8,5,2,9,0},
        {0,1,2,4,5,6,7,8, 1,2,4,5,6,7,8,9}
    };
    const int m=static_cast<int>(mood);
    if (m<0 || m>=moodCount) return;
    for (int i=0;i<length;++i)
    {
        const int beat=i%16;
        int degree=nearestDegreeToCents(mode(scaleIndex),
                                        targets[themes[m][beat]]);
        // Small, repeatable 4-bar variations; never randomise the cadence.
        const auto random=stepRandom(seed ^ (0x9e3779b9u*static_cast<std::uint32_t>(i+1)));
        if (beat!=0 && beat!=7 && beat!=15 && (random&7u)==0u)
            degree=(degree+((random&16u)!=0u ? 1 : 6))%7;
        // Passing tones borrow from adjacent *scale degrees*, avoiding a
        // foreign chromatic note which the current EDO-mode quantiser would
        // otherwise correctly snap back into its curated heptatonic subset.
        if (m==static_cast<int>(Mood::gentleTension) && (beat==3 || beat==11))
            degree=(degree+1)%7;
        pitches[static_cast<std::size_t>(i)] = pitchForDegree(scaleIndex,keyPc,degree);
    }
}
}
