@echo off
setlocal

echo =====================================================================
echo  Neural DSP: Quantized Noise Reduction and Sound Enhancer Build Script
echo =====================================================================

if not exist bin mkdir bin

where cl.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 goto build_msvc

where g++.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 goto build_mingw

where cmake.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 goto build_cmake

echo [WARNING] No MSVC, MinGW, or CMake found in active PATH.
echo To compile with MSVC, launch from "Developer Command Prompt for VS" and run build.bat.
goto done

:build_msvc
echo [INFO] Detected MSVC (cl.exe). Compiling Release binary and Unit Tests...
cl.exe /nologo /O2 /EHsc /std:c++17 /I src ^
    src\main.cpp ^
    src\neural\quantized_tensor.cpp ^
    src\neural\quantized_layers.cpp ^
    src\neural\neural_denoiser.cpp ^
    src\dsp\fft.cpp ^
    src\dsp\stft_engine.cpp ^
    src\dsp\spectral_gate.cpp ^
    src\dsp\sound_enhancer.cpp ^
    src\dsp\limiter.cpp ^
    src\dsp\dsp_pipeline.cpp ^
    src\audio\wav_file.cpp ^
    src\audio\audio_player.cpp ^
    src\gui\gui_controls.cpp ^
    src\gui\visualizers.cpp ^
    src\gui\gui_window.cpp ^
    /Fe:bin\NeuralDSP.exe ^
    /link /SUBSYSTEM:WINDOWS gdiplus.lib winmm.lib user32.lib gdi32.lib comdlg32.lib

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Main app built: bin\NeuralDSP.exe
) else (
    echo [ERROR] Main app compilation failed.
    goto done
)

cl.exe /nologo /O2 /EHsc /std:c++17 /I src ^
    tests\unit_tests.cpp ^
    src\neural\quantized_tensor.cpp ^
    src\neural\quantized_layers.cpp ^
    src\neural\neural_denoiser.cpp ^
    src\dsp\fft.cpp ^
    src\dsp\stft_engine.cpp ^
    src\dsp\spectral_gate.cpp ^
    src\dsp\sound_enhancer.cpp ^
    src\dsp\limiter.cpp ^
    src\dsp\dsp_pipeline.cpp ^
    src\audio\wav_file.cpp ^
    src\audio\audio_player.cpp ^
    /Fe:bin\NeuralDSP_Tests.exe ^
    /link /SUBSYSTEM:CONSOLE winmm.lib user32.lib

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Unit tests built: bin\NeuralDSP_Tests.exe
    echo [RUN] Running Unit Tests...
    bin\NeuralDSP_Tests.exe
)
goto done

:build_mingw
echo [INFO] Detected MinGW (g++). Compiling Release binary and Unit Tests...
g++ -O3 -std=c++17 -I src ^
    src/main.cpp ^
    src/neural/quantized_tensor.cpp ^
    src/neural/quantized_layers.cpp ^
    src/neural/neural_denoiser.cpp ^
    src/dsp/fft.cpp ^
    src/dsp/stft_engine.cpp ^
    src/dsp/spectral_gate.cpp ^
    src/dsp/sound_enhancer.cpp ^
    src/dsp/limiter.cpp ^
    src/dsp/dsp_pipeline.cpp ^
    src/audio/wav_file.cpp ^
    src/audio/audio_player.cpp ^
    src/gui/gui_controls.cpp ^
    src/gui/visualizers.cpp ^
    src/gui/gui_window.cpp ^
    -o bin/NeuralDSP.exe ^
    -lgdiplus -lwinmm -luser32 -lgdi32 -lcomdlg32 -mwindows

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Main app built: bin\NeuralDSP.exe
) else (
    echo [ERROR] Main app compilation failed.
    goto done
)

g++ -O3 -std=c++17 -I src ^
    tests/unit_tests.cpp ^
    src/neural/quantized_tensor.cpp ^
    src/neural/quantized_layers.cpp ^
    src/neural/neural_denoiser.cpp ^
    src/dsp/fft.cpp ^
    src/dsp/stft_engine.cpp ^
    src/dsp/spectral_gate.cpp ^
    src/dsp/sound_enhancer.cpp ^
    src/dsp/limiter.cpp ^
    src/dsp/dsp_pipeline.cpp ^
    src/audio/wav_file.cpp ^
    src/audio/audio_player.cpp ^
    -o bin/NeuralDSP_Tests.exe ^
    -lwinmm -luser32

if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] Unit tests built: bin\NeuralDSP_Tests.exe
    echo [RUN] Running Unit Tests...
    bin\NeuralDSP_Tests.exe
)
goto done

:build_cmake
echo [INFO] Detected CMake. Generating and building...
if not exist build mkdir build
cd build
cmake ..
cmake --build . --config Release
cd ..
goto done

:done
echo =====================================================================
