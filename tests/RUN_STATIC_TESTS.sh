#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 tests/V0621IntegrationChecks.py
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

# v0.6.0 factory patterns and forward/reverse/ping-pong stress checks.
"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeNewFeaturesTests.cpp -o /tmp/pnorde_new_features_tests
/tmp/pnorde_new_features_tests
printf 'v0.6.0 new-feature sanitizer tests passed\n'

"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeRetainedTransportTests.cpp -o /tmp/pnorde_retained_transport_tests
/tmp/pnorde_retained_transport_tests
printf 'v0.6.1 retained-clock sanitizer tests passed\n'

# Superseded by V0612IntegrationChecks (older expected v0.6.9 paths).
"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeStabilityTests.cpp -o /tmp/pnorde_stability_tests
/tmp/pnorde_stability_tests

# Superseded by V0612IntegrationChecks (legacy saved-build hardcoding).
"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeProjectVersionTests.cpp -o /tmp/pnorde_project_version_tests
/tmp/pnorde_project_version_tests
printf 'v0.6.7 project-state migration tests passed\n'

# Superseded by V0612IntegrationChecks (16/12/8/4 legacy widths).

# Superseded by V0612IntegrationChecks (legacy v0.6.9 version assertions).

"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeLaneStartTests.cpp -o /tmp/pnorde_lane_start_tests
/tmp/pnorde_lane_start_tests
printf 'v0.6.12 cycle start + reverse/ping-pong sanitizer tests passed\n'

"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeGlobalReverseTests.cpp -o /tmp/pnorde_global_reverse_tests
/tmp/pnorde_global_reverse_tests
printf "v0.6.13 global reverse transition sanitizer tests passed\n"

"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeAftertouchPatternsTests.cpp -o /tmp/pnorde_aftertouch_patterns_v0616
/tmp/pnorde_aftertouch_patterns_v0616

"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer tests/PnordeProgramLaneTests.cpp -o /tmp/pnorde_program_lane_v0616
/tmp/pnorde_program_lane_v0616

"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeProgramRequestTests.cpp -o /tmp/pnorde_program_requests_v0617
/tmp/pnorde_program_requests_v0617
printf 'v0.6.17 PROGRAM request arbitration sanitizer tests passed\n'

"$cxx" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/PnordeCompositionEngineTests.cpp -o /tmp/pnorde_v0621_composition_tests
/tmp/pnorde_v0621_composition_tests
printf 'v0.6.21 composition theory, EDO, serial and generation sanitizer tests passed\n'
