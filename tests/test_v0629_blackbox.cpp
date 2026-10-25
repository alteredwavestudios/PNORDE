#include "PnordeMidiBlackBox.h"
#include "PnordeDeveloperStats.h"
#include <cstdlib>
#define CHECK(x) do { if (!(x)) { std::cerr << "FAIL line " << __LINE__ << ": " << #x << "\n"; return EXIT_FAILURE; } } while(false)
#include <iostream>
int main() {
 PnordeMidiBlackBox::Recorder r;
 r.requestReset();r.beginBlock(512);
 const unsigned char a[]={0x90,60,100}, b[]={0x80,60,0};
 r.recordMidi(a,3,20);r.recordMidi(b,3,40);
 auto s=r.summary();CHECK(s.emittedNoteOns==1 && s.emittedNoteOffs==1);
 CHECK(s.outstandingKeys==0 && s.unmatchedNoteOffs==0);
 r.markProgram(0,3);
 r.markStop(2);s=r.summary();CHECK(s.frozen && s.stopKind==2);
 PnordeMidiBlackBox::Event e{};
 CHECK(r.read(0,e) && e.status==0x90 && e.data1==60 && e.frame==20);
 CHECK(r.read(1,e) && e.status==0x80);
 CHECK(r.read(2,e) && e.marker==4 && e.data1==1 && e.data2==4);
 CHECK(r.read(3,e) && e.marker==2);
 r.recordMidi(a,3,55);CHECK(r.summary().emittedNoteOns==1); // frozen
 r.requestReset();r.beginBlock(512);CHECK(!r.summary().frozen);
 r.recordMidi(a,3,0);CHECK(r.summary().outstandingKeys==1);
 const unsigned char cc64[]={0xb0,64,127}, off[]={0xb0,64,0}, kill[]={0xb0,120,0};
 r.recordMidi(cc64,3,5);r.recordMidi(off,3,6);r.recordMidi(kill,3,7);
 CHECK(r.summary().outstandingKeys==0);
 CHECK(r.summary().emittedCC64On==1 && r.summary().emittedCC64Off==1);
 CHECK(r.summary().emittedCC120==1);
 PnordeDeveloperStats::Counters c;c.setEnabled(true);
 c.recordSwitch(0,1,1,100,6000,true);c.recordSwitch(1,2,1,300,4000,true);
 c.recordAudioBlock(100,15);c.recordAudioBlock(30,7);
 auto stats=c.snapshot();CHECK(stats.restoreTotalUs==10000 && stats.restorePeakUs==6000);
 CHECK(stats.queueTotalUs==400 && stats.queuePeakUs==300);
 CHECK(stats.midiPeakPerBlock==15 && stats.midiMessages==22);
 // Ring wrapping must discard genuinely old events but retain the newest.
 r.requestReset();r.beginBlock(512);
 for (unsigned i=0;i<PnordeMidiBlackBox::Recorder::capacity+50;++i)
     r.recordMidi(a,3,static_cast<int>(i));
 CHECK(r.summary().droppedTraceEvents==50);
 CHECK(!r.read(0,e));
 CHECK(r.read(49+PnordeMidiBlackBox::Recorder::capacity,e));
 std::cout << "PASS: v0.6.29 MIDI black box, ring wrapping and aggregate counters\n";
}
