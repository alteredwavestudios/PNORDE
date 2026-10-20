# PNORDE: EDO harmony research and generative melodies

This is the theory behind PNORDE's *experimental* EDO options. It is not a claim that particular cultures or tunings objectively produce specific emotions. The named modes **Subminor Twilight**, **Neutral Nocturne**, **Velvet Minor**, **Bittersweet Major 7**, **Unresolved Cloud**, **Quartertone Haze**, **Meantone Minor**, **Septimal Longing**, and **Neutral Dream** are PNORDE's creative scale subsets, not traditional named musical systems. "Porcupine 7" reflects the 3-3-3-4-3-3-3 pattern in 22-EDO; the Pajara option is explicitly *inspired* by, not identical to, a canonical Pajara MOS mode.

## Which EDOs, and why?

| EDO | Step size | Example harmonic interval | Why it is interesting |
|---|---:|---|---|
| 17 | 70.588 cents | 5 steps = 352.94c, near neutral third (11/9 ~347.4c) | Stable fifths but unexpectedly ambiguous thirds. |
| 19 | 63.158 cents | 5 steps = 315.79c, extremely close to 6/5 (~315.64c) | Soft minor chords and distinct enharmonic spellings. |
| 22 | 54.545 cents | 7 steps = 381.82c (~5/4); 13 steps = 709.09c (~3/2) | Porcupine/Pajara families; 18 steps = 981.82c harmonic-seventh approximation. |
| 24 | 50.000 cents | 7 steps = 350c, a neutral third | Quarter-tones and expressive between-note movement. A 24-EDO scale is **not** a full Arabic maqam. |
| 31 | 38.710 cents | 10 steps = 387.10c (~5/4); 25 steps = 967.74c (~7/4) | Exceptionally close thirds and septimal sevenths allow bittersweet extensions. |

All EDO frequencies here repeat at 2:1 octaves and are mathematically exact. An EDO *scale* in PNORDE is a curated subset of seven degrees within a tuning, not a restriction to only seven possible notes in that EDO generally.

## The four generation moods

The generators are deliberately *melodic*, not chord generators; PNORDE currently emits a single sequenced voice. To suggest harmony, they **arpeggiate chord-like relationships** or trace tensions:

- **Longing / unresolved:** returns to near-minor thirds and fifths, reaches for a sixth and a 7/4-like seventh, and deliberately ends on an unresolved second rather than the tonic.
- **Bittersweet arpeggio:** alternates root, 5/4-like third, fifth, sixth and major seventh. This is a voice-leading design choice, *not* a guarantee that the resulting tonality feels bittersweet.
- **Gentle dissonance:** leans on neutral thirds (~11/9), fourths and passing scale tones, releasing some of that tension without harsh random jumps.
- **Homeward resolution:** begins on upper chord tones, descends through nearby degrees and settles onto the tonic.

The generated scale degree is selected by finding the closest **available degree in the current EDO mode** to these just-intonation-derived interval targets. A bounded 32-bit pseudo-random function makes small melodic alterations while leaving important cadence notes intact. This is intentionally seed-dependent and **runs only when clicked**, not every audio block or every playhead step. Other lane data (gate, velocity, CC, glide, step lengths) remains unchanged.

## MIDI implementation and limitations

PNORDE's Pitch data is stored in the existing `[-120, +120]` tenths-of-semitone format to avoid changing preset layout. The **sounding pitch** is computed at higher precision by quantising onto the selected EDO's *exact step* and sending the appropriate MIDI note plus **14-bit channel pitch bend**. That eliminates the otherwise audible ~5-cent rounding limitation of the visual pitch lane. This uses PNORDE's existing monophonic voice; it is **not** per-note MPE and cannot independently retune simultaneous notes on one channel.

For EDO, Rast, Bayati and Free (0.1), PNORDE automatically uses an effective +/-2-semitone pitch-bend range. The saved range for standard scales is preserved. Set the receiving synthesizer to +/-2 as well (MPE sends an RPN request, but not every host or synth honours it). MIDI note channels and routing remain as before. Hosts or synths that filter/ignore pitch bend will **not** play the intended EDO intervals. In Live 12, MPE-capable instruments and Ableton's Microtuner/Scale systems are alternatives if polyphonic microtonal output is needed.

## Research and listening references

- Xenharmonic Wiki 31-EDO interval chart (notably the 5/4 major third and 7/4 seventh): https://en.xen.wiki/w/31edo
- Xenharmonic Reference 22-EDO scale and interval structure: https://xenreference.com/w/22edo
- 19-EDO interval and meantone notes: https://microtonali.app/tunings/19-edo/
- 24-EDO interval chart: https://microtonali.app/tunings/24-edo/
- 17-EDO interval reference: https://en.xen.wiki/w/17-ET
- William Sethares, *Tuning Timbre Spectrum Scale*: the perceived consonance/roughness of microtones depends strongly on timbre: https://sethares.engr.wisc.edu/ttss.html
- Ableton Live 12 Tuning Systems and MPE setup: https://help.ableton.com/hc/en-us/articles/11535414344476-Tuning-Systems-FAQ and https://help.ableton.com/hc/en-us/articles/360019144999-MPE-in-Live-FAQ

The interval/ratio relationships are researched; **emotional mood labels and generated melodic motifs are PNORDE design choices**. No traditional maqam, performance practice or fixed cultural emotional association is claimed.

### v0.5.9 output clarification

All curated EDO modes and the Free (0.1), Rast and Bayati modes now use an effective +/-2 semitone pitch-bend range, without mutating the older saved host parameter for standard scales. SLIDE is **not bypassed**; its 0–100% values specify the tempo-relative time to approach the precise microtonal pitch. MPE mode is experimental, monophonic, member channel 2, and sends an RPN requesting +/-2. Poly Aftertouch is an independently opt-in MIDI Poly Key Pressure event. Ableton / instrument may still filter Pitch Wheel, so check its MIDI Monitor output and configure the receiving synth to +/-2.
