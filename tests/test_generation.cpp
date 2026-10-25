// Ensures composition helpers store relative intervals, not twice-transposed notes.
#include "../Source/PnordeCompositionEngine.h"
#include "../Source/PnordeRelativePitch.h"
#include <cassert>
#include <cmath>
#include <iostream>
int main() {
    int pitch=0, octave=0;
    PnordeCompositionEngine::encodePitch(4.0, 2, pitch, octave);
    assert(pitch == 40 && octave == 0);
    assert(std::abs(PnordeRelativePitch::outputSemitones(pitch,2,1) - 6.0)<1e-8);
    // Generator stores tonic as 0 even if the selected key is D.
    assert(PnordeEDO::pitchForDegree(PnordeEDO::firstScaleIndex,2,0)==0);
    PnordeCompositionEngine::Phrase phrase;
    PnordeCompositionEngine::fill(phrase,1,2,16,0,false,33);
    assert(phrase.pitch[15]==0); // ending on the tonic
    std::cout << "Generated phrases store relative, not double-transposed, pitches.\n";
}
