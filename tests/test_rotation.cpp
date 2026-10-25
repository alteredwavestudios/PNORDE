// PNORDE v0.6.25: no-JUCE proof that pattern rotation is non-destructive.
// Imagine that each stored card has a unique note, probability and glide.
// Rotating the visible order MUST move all those values together, without
// rewriting the original arrays. Going back to zero returns the exact order.
#include "../Source/PnordeRotation.h"
#include <array>
#include <cassert>
#include <iostream>

int main()
{
    namespace R = PnordeRotation;
    constexpr std::array<int, 8> pitches {{0,2,4,5,7,9,11,12}};
    constexpr std::array<int, 8> probability {{100,50,75,100,0,100,25,80}};
    constexpr std::array<int, 8> glides {{0,10,20,30,40,50,60,70}};
    for (int length=1; length<=128; ++length)
    for (int rotation=-258; rotation<=258; ++rotation)
    for (int v=0; v<length; ++v)
    {
        const int source=R::sourceIndex(v,length,rotation);
        assert(source>=0 && source<length);
        assert(R::visibleIndex(source,length,rotation)==v);
    }
    for (int rotation=-16;rotation<=16;++rotation)
    {
        for (int i=0;i<8;++i)
        {
            int source=R::sourceIndex(i,8,rotation);
            assert(pitches[source]>=0 && pitches[source]<=12);
            assert(probability[source]>=0 && probability[source]<=100);
            assert(glides[source]>=0 && glides[source]<=100);
            if (rotation==0)
            {
                assert(pitches[source]==pitches[i]);
                assert(probability[source]==probability[i]);
                assert(glides[source]==glides[i]);
            }
        }
    }
    // Rotating +2 shifts the FIRST stored note to visible slot 2.
    assert(R::sourceIndex(2,8,2)==0);
    assert(R::sourceIndex(0,8,2)==6);
    assert(pitches[R::sourceIndex(2,8,2)]==0);
    assert(probability[R::sourceIndex(2,8,2)]==100);
    std::cout << "Rotation: wrap, inverse, bounded mapping and values/probability passed\n";
}
