#include "../Source/PnordeScales.h"
#include <cassert>
#include <cstring>
#include <iostream>
int main()
{
    static_assert(PnordeScales::count == 47);
    assert(std::strcmp(PnordeScales::names[30], "Phrygian Dominant (Hijaz 12-TET)")==0);
    const int wanted[] = {0,10,40,50,70,80,100};
    for (int offset : wanted)
        assert(PnordeScales::quantiseExtra(offset,0,0,30)==offset);
    assert(PnordeScales::quantiseExtra(35,0,0,32)==35); // microtonal Rast third
    assert(PnordeScales::quantiseExtra(15,0,0,33)==15); // microtonal Bayati second
    for (int scale=14; scale<47; ++scale)
      for (int key=0; key<12; ++key)
        for (int octave=-4; octave<=4; ++octave)
          for (int v=-120; v<=120; ++v)
          {
            const auto q=PnordeScales::quantiseExtra(v,octave,key,scale);
            assert(q>=-120 && q<=120);
          }
    std::cout<<"Extended scale sanitizer tests passed\n";
}
