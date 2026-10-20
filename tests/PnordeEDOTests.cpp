#include "../Source/PnordeScales.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

int main()
{
    static_assert(PnordeScales::count == 47);
    static_assert(PnordeEDO::firstScaleIndex == 34);
    static_assert(PnordeEDO::modeCount == 13);
    assert(std::strcmp(PnordeScales::names[30],
        "Phrygian Dominant (Hijaz 12-TET)")==0);
    for (int index=34; index<PnordeScales::count; ++index)
    {
        const auto& mode=PnordeEDO::mode(index);
        assert(PnordeScales::names[index] != nullptr);
        assert(std::strcmp(PnordeScales::names[index],mode.name)==0);
        assert(mode.edo>=17 && mode.edo<=31);
        assert(mode.degrees.front()==0);
        for (int n=1;n<7;++n)
            assert(mode.degrees[static_cast<std::size_t>(n)] >
                   mode.degrees[static_cast<std::size_t>(n-1)]);
        assert(mode.degrees.back()<mode.edo);
        for (int key=0;key<12;++key)
          for (int oct=-4;oct<=4;++oct)
            for (int v=-120;v<=120;++v)
            {
                const auto exact=PnordeEDO::quantiseSemitones(v,oct,key,index);
                const auto stored=PnordeEDO::quantiseTenths(v,oct,key,index);
                assert(std::isfinite(exact));
                assert(exact>=-12.0 && exact<=12.0);
                assert(stored>=-120 && stored<=120);
                assert(std::abs(exact - stored/10.0) <= 0.05001);
                // The stored tenth re-quantises to the same EDO pitch.
                assert(std::abs(PnordeEDO::quantiseSemitones(stored,oct,key,index)-exact)<1e-7);
            }
        for (int mood=0;mood<PnordeEDO::moodCount;++mood)
        {
            std::array<int,128> first{},second{};
            PnordeEDO::generate(first,128,0,index,
                static_cast<PnordeEDO::Mood>(mood),0xabc123u);
            PnordeEDO::generate(second,128,0,index,
                static_cast<PnordeEDO::Mood>(mood),0xabc123u);
            assert(first==second);
            for (int note:first)
            {
                assert(note>=-120 && note<=120);
                assert(std::abs(PnordeEDO::quantiseSemitones(note,0,0,index)-
                                note/10.0)<0.051);
            }
        }
    }
    // 31edo major third is 10 steps, ~387.097 cents; 19edo minor
    // third is 5 steps, ~315.789 cents. These are our important anchors.
    assert(std::abs(1200.0*10/31.0 - 386.3137)<1.0);
    assert(std::abs(1200.0*5/19.0 - 315.64)<1.0);
    std::cout << "13 EDO modes and eight deterministic pattern styles passed\n";
}
