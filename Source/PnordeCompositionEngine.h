#pragma once
#include "PnordeScales.h"
#include "PnordePatternLibrary.h"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

// Musical composition is generated OFF the audio thread into bounded scratch
// arrays, then applied by the editor. It never reads/writes Quick Programs.
// Equal-tempered maqam/raga/gamelan *inspirations* aren't authentic tunings.
namespace PnordeCompositionEngine
{
constexpr int maxSteps = 128;
constexpr int melodyCount = 7;
constexpr const char* melodyNames[melodyCount] = {
    "Lanterns in Motion (balanced motif)",
    "Midnight Enclosure (chromatic approaches)",
    "Velvet Turnaround (ii-V-I targets)",
    "Glass Cathedral (open intervals)",
    "Spiral Answer (call and response)",
    "Ghost-Note Blues (syncopation)",
    "Restless Ascent (rising tension)"
};
constexpr int arpCount = 6;
constexpr const char* arpNames[arpCount] = {
    "Crystal Staircase", "Falling Constellations", "Mirror Palace",
    "Velvet Extensions (7th / 9th)", "Fractal Harp", "Spiral of Fifths"
};
struct Composition { const char* category; const char* name; int length; int scale; int recipe; bool arp; };
constexpr std::array<Composition, 16> compositions {{
    {"Baroque", "Clockwork Invention", 64, 1, 4, true},
    {"Baroque", "Courante of Shadows", 96, 9, 0, true},
    {"Classical", "Alberti's Garden", 64, 1, 2, true},
    {"Jazz & Bebop", "52nd Street After Dark (ii-V-I)", 128, 0, 2, false},
    {"Jazz & Bebop", "The Tritone Thief", 96, 0, 1, false},
    {"Blues & Funk", "Blue Note Alchemy", 64, 6, 5, false},
    {"Blues & Funk", "Chicago Stomp", 96, 7, 5, false},
    {"Serialism", "Twelve-Tone Labyrinth", 48, 0, 6, false},
    {"Serialism", "Retrograde Mirror", 96, 0, 4, false},
    {"Maqam-inspired (12-TET)", "Hijaz at Dusk", 64, 30, 4, false},
    {"Maqam-inspired (approx.)", "Bayati's Lament", 96, 33, 0, false},
    {"Indian-inspired (12-TET)", "Monsoon Alap", 64, 9, 3, false},
    {"Gamelan-inspired (12-TET)", "Bronze Rain", 96, 10, 4, false},
    {"EDO & Xenharmonic", "Septimal Serpent", 96, 45, 1, false},
    {"EDO & Xenharmonic", "31 Shades of Velvet", 128, 44, 3, false},
    {"Ambient / Minimal", "Slow Aurora", 128, 22, 3, false}
}};

struct Phrase
{
    int length = 16;
    int scale = 1;
    std::array<bool,maxSteps> gates{};
    std::array<int,maxSteps> pitch{}, octave{}, velocity{}, duration{}, slide{}, pressure{};
};
inline int clamp(int v,int lo,int hi) noexcept {return std::max(lo,std::min(hi,v));}
inline int degreeCount(int scale) noexcept
{
    if (PnordeEDO::isMode(scale)) return 7;
    if (scale == 0 || scale == 13) return 12;
    if (scale == 6 || scale == 7 || scale == 8) return scale == 8 ? 8 : 6;
    if (scale == 29) return 6;
    if (scale >= 14 && scale < 22) return 5;
    if (scale == 27 || scale == 28) return 8;
    return 7;
}
// Degrees expressed as semitone floats, not forced into 12-TET.
inline double degreeSemis(int scale,int degree) noexcept
{
    const int n=degreeCount(scale);
    const int oct=degree>=0 ? degree/n : (degree-n+1)/n;
    const int d=degree-oct*n;
    if (PnordeEDO::isMode(scale))
    {
        const auto& mode=PnordeEDO::mode(scale);
        return oct*12.0+12.0*mode.degrees[static_cast<size_t>(d)]/mode.edo;
    }
    static constexpr int chromatic[12]={0,1,2,3,4,5,6,7,8,9,10,11};
    static constexpr int major[7]={0,2,4,5,7,9,11};
    static constexpr int minor[7]={0,2,3,5,7,8,10};
    static constexpr int dorian[7]={0,2,3,5,7,9,10};
    static constexpr int phrygian[7]={0,1,3,5,7,8,10};
    static constexpr int locrian[7]={0,1,3,5,6,8,10};
    static constexpr int blues[6]={0,3,5,6,7,10};
    static constexpr int majorBlues[6]={0,2,3,4,7,9};
    static constexpr int bebop[8]={0,2,4,5,7,9,10,11};
    static constexpr int harmonicMinor[7]={0,2,3,5,7,8,11};
    static constexpr int pelog[7]={0,1,3,5,7,8,10}; // 12-TET sketch, not gamelan tuning
    static constexpr int melodicMinor[7]={0,2,3,5,7,9,11};
    static constexpr int mixolydian[7]={0,2,4,5,7,9,10};
    static constexpr int halfWhole[8]={0,1,3,4,6,7,9,10};
    static constexpr int wholeHalf[8]={0,2,3,5,6,8,9,11};
    static constexpr int wholeTone[6]={0,2,4,6,8,10};
    const int* values=major;
    switch(scale)
    {
        case 0: case 13: values=chromatic; break;
        case 2: case 23: values=minor; break;
        case 3: values=dorian; break;
        case 4: values=phrygian; break;
        case 5: values=locrian; break;
        case 6: values=blues; break;
        case 7: values=majorBlues; break;
        case 8: values=bebop; break;
        case 9: values=harmonicMinor; break;
        case 10: values=pelog; break;
        case 11: values=melodicMinor; break;
        case 12: values=mixolydian; break;
        case 27: values=halfWhole; break;
        case 28: values=wholeHalf; break;
        case 29: values=wholeTone; break;
        default: break;
    }
    if (scale>=14 && scale<34)
    {
        const auto& s=PnordeScales::extras[static_cast<size_t>(scale-14)];
        return oct*12.0+s.offsetsTenths[static_cast<size_t>(d)]/10.0;
    }
    return oct*12.0+values[d];
}
// Encode an exact conceptual pitch as PITCH (tenths) + OCTAVE. Playback
// re-quantises EDO to the exact lattice; no host pitch-bend side effects here.
inline void encodePitch(double semis,int key,int& pitch,int& octave) noexcept
{
    const double target=semis+key;
    octave=clamp(static_cast<int>(std::floor((target+6.0)/12.0)),-4,4);
    pitch=clamp(static_cast<int>(std::lround((target-12.0*octave)*10.0)),-120,120);
}
inline std::uint32_t noise(std::uint32_t seed,int step,int salt=0) noexcept
{
    return PnordePatternLibrary::hash(seed+static_cast<std::uint32_t>(step*1013+salt*32771+0x6f23));
}
// Simple chord functions, relative to selected key. Four 4-step measures are
// expanded/rotated for longer phrases; strong notes outline the active chord.
struct Chord { int root, third, fifth, seventh; };
inline Chord chordAt(int style,int bar) noexcept
{
    static constexpr Chord iiVI[4]={{2,5,9,0},{7,11,2,5},{0,4,7,11},{0,4,7,11}};
    static constexpr Chord turn[4]={{0,4,7,11},{9,0,4,7},{2,5,9,0},{7,11,2,5}};
    static constexpr Chord tritone[4]={{2,5,9,0},{1,5,8,11},{0,4,7,11},{7,11,2,5}};
    const auto& table=style==1 ? tritone : style==4 ? turn : iiVI;
    return table[static_cast<size_t>(bar%4)];
}
inline void fill(Phrase& p,int scale,int key,int length,int recipe,
                 bool arpeggio,std::uint32_t seed) noexcept
{
    p=Phrase{};
    p.length=clamp(length,1,maxSteps);
    p.scale=clamp(scale,0,PnordeScales::count-1);
    key=clamp(key,0,11);
    recipe=clamp(recipe,0,arpeggio?arpCount-1:melodyCount-1);
    const int n=degreeCount(p.scale);
    static constexpr int motif[16]={0,1,2,4,3,2,1,0,2,3,4,5,4,2,1,0};
    static constexpr int answer[16]={0,2,4,3,2,1,0,-1,3,2,1,0,-1,0,2,0};
    static constexpr int arpDegrees[6][8]={
        {0,2,4,6,7,9,11,13}, {13,11,9,7,6,4,2,0},
        {0,4,2,6,6,2,4,0}, {0,2,4,6,8,6,4,2},
        {0,4,2,6,1,5,3,7}, {0,4,1,5,2,6,3,7}
    };
    for(int i=0;i<p.length;++i)
    {
        const int beat=i%16;
        const int bar=i/16;
        const auto r=noise(seed,i);
        int degree=0;
        if (arpeggio)
        {
            degree=arpDegrees[recipe][i%8]+(bar%3==1?1:0);
        }
        else switch(recipe)
        {
            case 0: degree=motif[beat]+static_cast<int>((bar%3)-1); break;
            case 1: degree=motif[beat]+((i%4==3)?-1:(i%4==2)?1:0); break;
            case 2: degree=answer[beat]+(bar%4==1?1:0); break;
            case 3: degree=(beat%4==0?0:beat%4==1?4:beat%4==2?2:6)+(bar%3); break;
            case 4: degree=(bar%2==0?motif[beat]:answer[15-beat]); break;
            case 5: degree=(beat%4==0?0:beat%4==2?3:5)+(bar%2); break;
            default: degree=(i/2)%n + (i/16)%3; break;
        }
        const bool chromaticJazz=p.scale==0 && !arpeggio && (recipe==1 || recipe==2);
        double semis=degreeSemis(p.scale,degree);
        if(chromaticJazz)
        {
            const Chord chord=chordAt(recipe,bar);
            static constexpr int targets[4]={0,1,3,2};
            const int tones[4]={chord.root,chord.third,chord.fifth,chord.seventh};
            const int aim=tones[targets[(i/2)%4]];
            semis=aim+(beat%4==0?0:beat%4==1?-1:beat%4==2?2:1);
            if(recipe==1 && beat%8==7) semis-=1;
            if(i%16==15) semis=chord.root;
        }
        else if(!arpeggio && recipe==1 && !PnordeEDO::isMode(p.scale)
                && (p.scale==0 || p.scale==8) && beat%4==3)
            semis-=1.0;
        if (!arpeggio && recipe==6) semis+=bar%4==3?12:0;
        if(arpeggio && recipe==4) semis+=(bar%2? -12:0);
        encodePitch(semis,key,p.pitch[static_cast<size_t>(i)],p.octave[static_cast<size_t>(i)]);
        // Strong beats, cadences and arpeggio peaks get intentional accents.
        const int accent=(beat%4==0?22:beat%2==0?9:-7);
        const int jitter=static_cast<int>(r%13)-6;
        p.velocity[static_cast<size_t>(i)]=clamp(62+accent+jitter+(beat==15?14:0),25,100);
        const bool rest=(recipe==3?beat%8==6:recipe==5?(beat==3||beat==11):
                          recipe==4?beat==7:!arpeggio && beat==14);
        p.gates[static_cast<size_t>(i)]=!rest;
        p.duration[static_cast<size_t>(i)]=clamp(
            (arpeggio?72:recipe==3?91:recipe==5?51:74)+(beat%4==3?-22:0),15,100);
        p.slide[static_cast<size_t>(i)]=recipe==3 && beat%4==1 ? 25 : 0;
        p.pressure[static_cast<size_t>(i)]=clamp(28+(beat%4==0?22:0)+static_cast<int>(r%17),0,100);
    }
    // Resolve any ending, rather than terminating mid-motif.
    const int last=p.length-1;
    double tonic=0.0;
    if(p.scale==0) tonic=0;
    encodePitch(tonic,key,p.pitch[static_cast<size_t>(last)],p.octave[static_cast<size_t>(last)]);
    p.gates[static_cast<size_t>(last)]=true;
    p.velocity[static_cast<size_t>(last)]=93;
    p.duration[static_cast<size_t>(last)]=94;
}
// A full 12-tone row, repeated under prime, inversion, retrograde and
// retrograde-inversion transformations (rather than a random chromatic walk).
constexpr int chromaticJazzCount=6;
constexpr const char* chromaticJazzNames[chromaticJazzCount]={
    "52nd Street After Dark (ii-V-I)",
    "The Tritone Thief (ii-bII7-I)",
    "Velvet Turnaround (I-vi-ii-V)",
    "Half-Diminished Moon (minor ii-V-i)",
    "Giant Steps Across the Sky (major thirds)",
    "Blue Note Alchemy (12-bar blues)"
};
// Harmonic plans supply target chord tones for each 16-step measure.
// The chromatic approach cells are NOT naively quantised back to a scale.
inline Chord jazzChord(int style,int bar) noexcept
{
    static constexpr Chord minor[4]={{2,5,8,0},{7,11,2,5},{0,3,7,10},{0,3,7,10}};
    static constexpr Chord thirds[4]={{0,4,7,11},{8,0,3,7},{4,8,11,3},{0,4,7,11}};
    static constexpr Chord blues[12]={{0,4,7,10},{0,4,7,10},{0,4,7,10},{0,4,7,10},
        {5,9,0,3},{5,9,0,3},{0,4,7,10},{0,4,7,10},
        {7,11,2,5},{5,9,0,3},{0,4,7,10},{7,11,2,5}};
    if(style==3)return minor[bar%4];
    if(style==4)return thirds[bar%4];
    if(style==5)return blues[bar%12];
    return chordAt(style==1?1:style==2?4:0,bar);
}
inline void makeChromaticJazz(Phrase& p,int key,int length,int style,
                              std::uint32_t seed) noexcept
{
    style=clamp(style,0,chromaticJazzCount-1);
    fill(p,0,key,length,2,false,seed);
    for(int i=0;i<p.length;++i)
    {
        const int beat=i%16;
        const int bar=i/16;
        const auto chord=jazzChord(style,bar);
        const int tones[4]={chord.root,chord.third,chord.seventh,chord.fifth};
        const int goal=tones[(i/4)%4];
        // A target followed by a chromatic neighbour/enclosure and return.
        const int enclosure[4]={0,2,-1,0};
        const int degree=goal+enclosure[i%4]+((bar%3==2)?-12:0);
        encodePitch(degree,key,p.pitch[static_cast<size_t>(i)],p.octave[static_cast<size_t>(i)]);
        p.gates[static_cast<size_t>(i)]=beat!=7 && beat!=14;
        p.velocity[static_cast<size_t>(i)]=clamp(57+(i%4==0?31:10)
            +static_cast<int>(noise(seed,i)%13)-6,1,100);
        p.duration[static_cast<size_t>(i)]=(i%4==3?45:76);
    }
    // Complete the last cadence on the final tonic.
    encodePitch(0.0,key,p.pitch[static_cast<size_t>(p.length-1)],
        p.octave[static_cast<size_t>(p.length-1)]);
    p.gates[static_cast<size_t>(p.length-1)]=true;
    p.duration[static_cast<size_t>(p.length-1)]=95;
}

inline void makeSerial(Phrase& phrase,bool startRetrograde,int key=0) noexcept
{
    static constexpr int prime[12]={0,1,4,2,7,5,9,8,11,3,6,10};
    for(int i=0;i<phrase.length;++i)
    {
        const int segment=(i/12)%4;
        const int offset=i%12;
        const bool backwards=((segment==2 || segment==3)!=startRetrograde);
        const int original=prime[backwards?11-offset:offset];
        const int pc=(segment==1 || segment==3) ? (12-original)%12 : original;
        const int registerShift=((i/12)%3)-1;
        encodePitch(static_cast<double>(pc+12*registerShift),key,
            phrase.pitch[static_cast<size_t>(i)],phrase.octave[static_cast<size_t>(i)]);
        phrase.gates[static_cast<size_t>(i)]=i%12!=10;
        phrase.velocity[static_cast<size_t>(i)]=clamp(42+(i*17)%55,1,100);
        phrase.duration[static_cast<size_t>(i)]=35+(i*13)%65;
    }
}
inline void makeGate(std::array<bool,128>& out,int length,int choice,
                     std::uint32_t seed) noexcept
{
    length=clamp(length,1,128);
    for(int i=0;i<length;++i)
    {
        const int b=i%16;
        const auto n=noise(seed,i);
        switch(choice)
        {
            case 0: out[static_cast<size_t>(i)]=b%4==0 || b==7 || b==14;break; // Blue Smoke
            case 1: out[static_cast<size_t>(i)]=b==0 || b==3 || b==6 || b==10 || b==14;break; // Ghost-step
            case 2: out[static_cast<size_t>(i)]=((i*5)%16)<5;break; // 5/16 euclidean
            case 3: out[static_cast<size_t>(i)]=((i*7)%16)<7;break; // 7/16 euclidean
            case 4: out[static_cast<size_t>(i)]=i%5==0 || i%7==0;break; // 5 against 7
            case 5: out[static_cast<size_t>(i)]=b==0 || b==10 || (n%19==0);break; // Aurora
            case 6: out[static_cast<size_t>(i)]=b%2==0 || b==5 || b==13;break; // acid
            default: out[static_cast<size_t>(i)]=b==0 || b==5 || b==10 || b==15;break;
        }
    }
}
constexpr int addedGateCount=8;
constexpr const char* addedGateNames[addedGateCount]={
    "Blue Smoke (jazz comping)", "Ghost-Step Funk", "Five Against Sixteen (Euclidean)",
    "Seven Moons (Euclidean)", "Phase Bloom (5 against 7)",
    "Slow Aurora (ambient)", "Acid Vein", "Broken Machine"
};
}
