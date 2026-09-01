cmake_minimum_required(VERSION 3.15)
project(NeuralDSPNoiseReduction CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORE_SOURCES
    src/neural/quantized_tensor.cpp
    src/neural/quantized_layers.cpp
    src/neural/neural_denoiser.cpp
    src/dsp/fft.cpp
    src/dsp/stft_engine.cpp
    src/dsp/spectral_gate.cpp
    src/dsp/sound_enhancer.cpp
    src/dsp/limiter.cpp
    src/dsp/dsp_pipeline.cpp
    src/audio/wav_file.cpp
    src/audio/audio_player.cpp
)

set(GUI_SOURCES
    src/gui/gui_controls.cpp
    src/gui/visualizers.cpp
    src/gui/gui_window.cpp
)

add_executable(NeuralDSP WIN32 src/main.cpp ${CORE_SOURCES} ${GUI_SOURCES})
target_include_directories(NeuralDSP PRIVATE src)

if(MSVC)
    target_compile_options(NeuralDSP PRIVATE /O2 /EHsc /W4 /permissive-)
    target_link_libraries(NeuralDSP PRIVATE gdiplus.lib winmm.lib user32.lib gdi32.lib comdlg32.lib)
else()
    target_compile_options(NeuralDSP PRIVATE -O3 -Wall -Wextra)
    target_link_libraries(NeuralDSP PRIVATE -lgdiplus -lwinmm -luser32 -lgdi32 -lcomdlg32 -mwindows)
endif()

add_executable(NeuralDSP_Tests tests/unit_tests.cpp ${CORE_SOURCES})
target_include_directories(NeuralDSP_Tests PRIVATE src)

if(MSVC)
    target_compile_options(NeuralDSP_Tests PRIVATE /O2 /EHsc /W4)
    target_link_libraries(NeuralDSP_Tests PRIVATE winmm.lib user32.lib)
else()
    target_compile_options(NeuralDSP_Tests PRIVATE -O3 -Wall -Wextra)
    target_link_libraries(NeuralDSP_Tests PRIVATE -lwinmm -luser32)
endif()
