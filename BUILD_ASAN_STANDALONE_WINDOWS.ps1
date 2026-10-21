$ErrorActionPreference = 'Stop'
$project = $PSScriptRoot
$build = Join-Path $project 'build-asan-v061'
Write-Host 'Configuring separate PNORDE AddressSanitizer Debug Standalone...' -ForegroundColor Cyan
# /RTC1 must not be enabled alongside MSVC AddressSanitizer.
& cmake -S "$project" -B "$build" -G 'Visual Studio 18 2026' -A x64 `
    '-DCMAKE_CXX_FLAGS_DEBUG=/Od /Zi /MDd /D_DEBUG /fsanitize=address' `
    '-DCMAKE_C_FLAGS_DEBUG=/Od /Zi /MDd /D_DEBUG /fsanitize=address' `
    '-DCMAKE_EXE_LINKER_FLAGS_DEBUG=/INCREMENTAL:NO'
if ($LASTEXITCODE -ne 0) { throw 'ASan configure failed; test build not available.' }
& cmake --build "$build" --config Debug --target PnordeDreamSequencer_Standalone --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'ASan build failed; do not run or install.' }
Write-Host 'ASan Standalone compiled. Run its Debug executable and test UI interactions.' -ForegroundColor Green
Write-Host 'DO NOT install a Debug/ASan build into your production VST3 folder.'
