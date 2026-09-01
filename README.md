# Neural DSP: Quantized Neural Network Noise Reduction & Sound Enhancer

A high-performance, **zero-dependency** C++ audio processing application featuring an **INT8 Quantized Neural Network** for real-time spectral noise cancellation, a psychoacoustic **Sound Enhancer** (harmonic bass exciter, air sheen, transient shaper, lookahead limiter), and a modern hardware-styled **WinAPI + GDI+ Graphical User Interface**.

---

## 🌟 Key Features

### 1. Zero-Dependency Quantized Neural Network (INT8)
- **Zero External Libraries**: Written in pure, clean C++17 without PyTorch, ONNX Runtime, TensorFlow, or BLAS dependencies.
- **Low-Precision INT8 Inference**:
  - Symmetric INT8 weights with asymmetric activations ($q = \text{clamp}(\text{round}(x / s) + z, -128, 127)$).
  - 32-bit integer accumulation (`int32_t`) with fixed-point multiplier rescaling.
  - Table-driven activation functions (ReLU6, Sigmoid, and Tanh LUTs).
- **Custom Architecture (`QuantizedSpectralMaskNet`)**:
  - **Input**: 257 Log-Magnitude Spectral Bins (512-point FFT).
  - **Layer 1 (Dense)**: $257 \to 64$ features with ReLU6 activation.
  - **Layer 2 (Conv1D)**: 64 Channels, Kernel Size 3, Stride 1, Padding 1 with Tanh activation.
  - **Layer 3 (GRU)**: $64 \to 48$ Hidden recurrent units maintaining temporal context across speech/noise frames.
  - **Layer 4 (Output Dense)**: $48 \to 257$ Sigmoid attenuation mask $[0.0, 1.0]$.
  - **Ultra-low latency**: $< 50\,\mu\text{s}$ per frame inference (well within the $2.66\,\text{ms}$ frame budget at 48 kHz).

### 2. Digital Signal Processing (DSP) Engine
- **STFT / ISTFT Overlap-Add (OLA)**:
  - 512-point Radix-2 FFT with Hann windowing and 75% overlap (128-sample hop size).
  - Perfect reconstruction filterbank with energy normalization.
- **Noise Cancellation & Suppression**:
  - **Neural Spectral Masking**: Multiplies frequency magnitude bins by neural gain factors ($|S(\omega)| = |Y(\omega)| \cdot M_{\text{nn}}(\omega)$).
  - **Adaptive Spectral Gate**: Exponential noise floor tracking with configurable threshold, knee, and suppression floor.
  - **Learn Noise Profile**: 1-Click capture of ambient stationary background noise (mic hiss, computer fan, electrical hum).
  - **Anti-Musical Noise Smoothing**: Inter-frame temporal smoothing prevents chirping and musical noise artifacts.
  - **Transient Preservation**: Dynamic onset tracking preserves speech plosives and drum attacks.
- **Sound Enhancer & Psychoacoustic Exciter**:
  - **Psychoacoustic Bass Exciter**: Low-pass extraction (40Hz–200Hz) with asymmetric non-linear waveshaping ($y = \tanh(x) - 0.2 x^2$) synthesizing 2nd and 3rd harmonics to produce perceived deep bass.
  - **Air Sheen & Brilliance**: High-shelf harmonic exciter (>4kHz–16kHz) for crystal-clear presence.
  - **Transient Punch**: Differential envelope shaper ($\Delta E = E_{\text{fast}} - E_{\text{slow}}$) boosting attack punch.
  - **Warmth & Clarity EQ**: Parametric warmth peaking (300Hz) and clarity presence (3.5kHz).
  - **Lookahead Peak Limiter**: 2.5ms lookahead circular buffer with soft-knee saturation, ensuring $0\,\text{dBFS}$ brickwall protection without digital clipping.

### 3. Modern GDI+ WinAPI User Interface
- **Studio Hardware Aesthetic**: Sleek dark metallic theme with cyan, emerald, and amber illuminations.
- **Interactive Controls**:
  - Smooth metallic rotary knobs with indicator arcs and digital value readouts.
  - Interactive sliders, illuminated toggle switches, and push buttons.
- **Real-Time Visualizers**:
  - **Dual Spectrum Visualizer**: Pre-DSP (Amber) vs Post-DSP (Cyan) frequency curve with $20\,\text{Hz} - 20\,\text{kHz}$ logarithmic grid.
  - **Neural Mask Visualizer**: 257-bin live attenuation heatmap displaying exact neural decisions.
  - **Oscilloscope**: Real-time waveform display.
  - **Stereo VU Meters**: Peak & RMS meters with peak-hold indicators.
- **3 Tabbed Workspaces**:
  1. **DSP & Noise Reduction**: Main studio rack with real-time controls and spectrum analyzer.
  2. **Quantized Neural Inspector**: Live model telemetry, inference time ($\mu\text{s}$), MAC operations count, parameter count, and 16-bucket weight histogram.
  3. **Audio & Test Studio**: Load WAV files, offline batch export, real-time playback, and built-in synthetic test signal generator (Simulated speech + pink noise + 60Hz hum).

---

## 📁 Project Structure

```
Neural-DSP-NoiseReduction/
├── CMakeLists.txt               # Cross-platform CMake configuration
├── build.bat                    # 1-Click build script for MSVC and MinGW
├── README.md                    # Project documentation
└── src/
    ├── main.cpp                 # WinMain entry point, GDI+ lifecycle
    ├── neural/
    │   ├── quantized_tensor.h   # INT8 tensor math, quantization, dot products
    │   ├── quantized_tensor.cpp # Tensor kernels and LUT implementations
    │   ├── quantized_layers.h   # Dense, Conv1D, GRU quantized layers
    │   ├── quantized_layers.cpp # Forward pass and profiling statistics
    │   ├── neural_denoiser.h    # Spectral denoiser bridge
    │   ├── neural_denoiser.cpp  # Mask smoothing and transient preservation
    │   └── default_weights.h    # Pre-calibrated INT8 neural weights
    ├── dsp/
    │   ├── fft.h                # Radix-2 FFT and windowing header
    │   ├── fft.cpp              # Bit-reversal and twiddle factor calculations
    │   ├── stft_engine.h        # STFT / ISTFT Overlap-Add engine
    │   ├── stft_engine.cpp      # Synthesis and framing
    │   ├── spectral_gate.h      # Adaptive noise floor and profile learner
    │   ├── spectral_gate.cpp    # Spectral gating algorithms
    │   ├── sound_enhancer.h     # Bass exciter, air sheen, transient shaper
    │   ├── sound_enhancer.cpp   # Non-linear harmonic waveshaping & biquad filters
    │   ├── limiter.h            # Lookahead brickwall limiter
    │   ├── limiter.cpp          # Circular delay buffer & gain envelope
    │   ├── dsp_pipeline.h       # Master DSP audio processing chain
    │   └── dsp_pipeline.cpp     # Thread-safe pipeline execution & metering
    ├── audio/
    │   ├── wav_file.h           # WAV file parser and writer (16/24/32-bit PCM)
    │   ├── wav_file.cpp         # RIFF I/O implementation
    │   ├── audio_player.h       # WinMM WaveOut real-time audio playback
    │   └── audio_player.cpp     # Audio streaming and test signal generator
    └── gui/
        ├── gui_theme.h          # Studio dark color palette and GDI+ helpers
        ├── gui_controls.h       # Custom GDI+ Knobs, Sliders, Switches, Tabs
        ├── gui_controls.cpp     # Control rendering and mouse interactions
        ├── visualizers.h        # Spectrum, Neural Mask, Oscilloscope, VU Meters
        ├── visualizers.cpp      # Real-time animated GDI+ visualizers
        ├── gui_window.h         # Main application window header
        └── gui_window.cpp       # Win32 WndProc, message loop, and tab views
```

---

## 🛠️ Building & Running

### Option 1: Using `build.bat` (MSVC or MinGW)
Open a terminal in the project directory and run:
```cmd
build.bat
```
The script will automatically detect MSVC (`cl.exe`), MinGW (`g++`), or CMake and generate `bin/NeuralDSP.exe`.

### Option 2: Using MSVC Command Line
Open the **Developer Command Prompt for VS** and run:
```cmd
cl.exe /O2 /EHsc /std:c++17 /I src src\main.cpp src\neural\*.cpp src\dsp\*.cpp src\audio\*.cpp src\gui\*.cpp /Fe:bin\NeuralDSP.exe /link /SUBSYSTEM:WINDOWS gdiplus.lib winmm.lib user32.lib gdi32.lib comdlg32.lib
```

### Option 3: Using MinGW GCC
```cmd
g++ -O3 -std=c++17 -I src src/main.cpp src/neural/*.cpp src/dsp/*.cpp src/audio/*.cpp src/gui/*.cpp -o bin/NeuralDSP.exe -lgdiplus -lwinmm -luser32 -lgdi32 -lcomdlg32 -mwindows
```

### Option 4: Using CMake
```cmd
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

---

## 🧪 Running Unit Tests

To run the automated verification suite covering INT8 quantization, Conv1D, GRU temporal memory, FFT/STFT round-trip identity, limiter ceiling protection, and WAV file I/O:

```cmd
# Run via build.bat
build.bat

# Or run tests executable directly
.\bin\NeuralDSP_Tests.exe
```

---

## 🎧 Quick Start & Testing

1. Launch `bin/NeuralDSP.exe`.
2. The application automatically starts with the **Live Test Signal Generator** playing a simulated noisy vocal track.
3. Observe the **Real-Time Spectrum Visualizer**:
   - **Amber Curve**: Noisy input audio.
   - **Cyan Curve**: Neural denoised and enhanced output audio.
4. Switch to the **Quantized Neural Inspector** tab to view the live INT8 weight histogram, MAC ops count, and execution latency in microseconds.
5. Click **DSP BYPASS** to toggle A/B comparison and hear the difference.
6. Switch to the **Audio & Test Studio** tab to load your own `.wav` files and export cleaned versions!
