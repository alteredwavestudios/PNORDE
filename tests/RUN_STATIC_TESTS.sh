#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 tests/SourceRegressionTests.py
cxx="${CXX:-clang++}"
"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeSafetyTests.cpp -o /tmp/pnorde_safety_tests
/tmp/pnorde_safety_tests

"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeVisualOrderTests.cpp -o /tmp/pnorde_visual_tests
/tmp/pnorde_visual_tests
"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeAftertouchGlideTests.cpp -o /tmp/pnorde_aftertouch_tests
/tmp/pnorde_aftertouch_tests

"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeTempoGlideTests.cpp -o /tmp/pnorde_tempo_glide_tests
/tmp/pnorde_tempo_glide_tests

"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeScaleTests.cpp -o /tmp/pnorde_scale_tests
/tmp/pnorde_scale_tests

"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeManualTempoTests.cpp -o /tmp/pnorde_manual_tempo_tests
/tmp/pnorde_manual_tempo_tests

"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeEDOTests.cpp -o /tmp/pnorde_edo_tests
/tmp/pnorde_edo_tests

"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeMicroOutputTests.cpp -o /tmp/pnorde_micro_output_tests
/tmp/pnorde_micro_output_tests
"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordePatternLibraryTests.cpp -o /tmp/pnorde_pattern_library_tests
/tmp/pnorde_pattern_library_tests
