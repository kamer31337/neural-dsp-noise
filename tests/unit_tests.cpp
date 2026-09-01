#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <string>
#include <chrono>
#include "../src/neural/quantized_tensor.h"
#include "../src/neural/quantized_layers.h"
#include "../src/neural/neural_denoiser.h"
#include "../src/dsp/fft.h"
#include "../src/dsp/stft_engine.h"
#include "../src/dsp/spectral_gate.h"
#include "../src/dsp/sound_enhancer.h"
#include "../src/dsp/limiter.h"
#include "../src/dsp/dsp_pipeline.h"
#include "../src/audio/wav_file.h"

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            g_testsPassed++; \
        } else { \
            g_testsFailed++; \
            std::cerr << "  [FAIL] " << msg << " (Line " << __LINE__ << ")" << std::endl; \
        } \
    } while(0)

void testQuantizedTensor()
{
    std::cout << "[TEST] Running QuantizedTensor & Fixed-Point Tests..." << std::endl;
    std::vector<float> original = {-1.0f, -0.5f, 0.0f, 0.25f, 0.75f, 1.0f};
    QuantizedTensor tensor({6}, 1.0f / 127.0f, 0);
    tensor.quantize(original.data(), original.size());
    std::vector<float> restored(6);
    tensor.dequantize(restored.data(), restored.size());
    for (size_t i = 0; i < original.size(); ++i)
    {
        float err = std::abs(original[i] - restored[i]);
        TEST_ASSERT(err < 0.015f, "Quantization round-trip error must be < 0.015");
    }
    std::vector<int8_t> vecA = {10, -20, 30, 40, -50, 60, 70, -80};
    std::vector<int8_t> vecB = {5, 10, -15, 20, 25, -30, 35, 40};
    int32_t expectedDot = 0;
    for (size_t i = 0; i < vecA.size(); ++i) expectedDot += static_cast<int32_t>(vecA[i]) * static_cast<int32_t>(vecB[i]);
    int32_t calcDot = QuantizedTensor::dotProduct(vecA.data(), vecB.data(), vecA.size());
    TEST_ASSERT(calcDot == expectedDot, "INT8 Dot product must match expected value");
    const ActivationLUT& lut = ActivationLUT::instance();
    for (float x = -4.0f; x <= 4.0f; x += 0.5f)
    {
        int8_t qX = QuantizedTensor::quantizeValue(x, 1.0f / 16.0f, 0);
        int8_t qSig = lut.evaluateSigmoid(qX, 1.0f / 16.0f, 0, 1.0f / 127.0f, 0);
        float fSig = QuantizedTensor::dequantizeValue(qSig, 1.0f / 127.0f, 0);
        float expectedSig = 1.0f / (1.0f + std::exp(-x));
        TEST_ASSERT(std::abs(fSig - expectedSig) < 0.05f, "Sigmoid LUT value must match float sigmoid within tolerance");
    }
}

void testQuantizedLayers()
{
    std::cout << "[TEST] Running Quantized Neural Network Layers Tests..." << std::endl;
    QuantizedDenseLayer dense(16, 8, ActivationType::ReLU6);
    dense.initRandom(0.5f);
    QuantizedTensor input({16}, 1.0f / 127.0f, 0);
    input.fill(32);
    QuantizedTensor output;
    dense.forward(input, output);
    TEST_ASSERT(output.data.size() == 8, "Dense layer output size must match outFeatures");
    QuantizedConv1DLayer conv(1, 1, 3, 1, 1, ActivationType::Tanh);
    conv.initRandom(0.5f);
    QuantizedTensor convOut;
    conv.forward(input, convOut);
    TEST_ASSERT(convOut.data.size() == 16, "Conv1D layer with same padding must preserve length");
    QuantizedGRULayer gru(16, 12);
    gru.initRandom(0.3f);
    QuantizedTensor gruOut1, gruOut2;
    gru.forward(input, gruOut1);
    gru.forward(input, gruOut2);
    TEST_ASSERT(gruOut1.data.size() == 12, "GRU output size must match hidden size");
    gru.resetState();
    QuantizedSpectralMaskNet maskNet;
    std::vector<float> mags(QuantizedSpectralMaskNet::NUM_BINS, 0.5f);
    std::vector<float> mask(QuantizedSpectralMaskNet::NUM_BINS, 0.0f);
    maskNet.forward(mags.data(), mask.data(), 1.0f);
    bool allValid = true;
    for (int i = 0; i < QuantizedSpectralMaskNet::NUM_BINS; ++i)
    {
        if (mask[i] < 0.0f || mask[i] > 1.0f || std::isnan(mask[i])) allValid = false;
    }
    TEST_ASSERT(allValid, "Neural Spectral Mask values must be strictly in [0.0, 1.0]");
}

void testDSPFFT()
{
    std::cout << "[TEST] Running DSP FFT & STFT Tests..." << std::endl;
    size_t fftSize = 512;
    FFT fft(fftSize);
    std::vector<float> input(fftSize, 0.0f);
    for (size_t i = 0; i < fftSize; ++i) input[i] = std::sin(2.0f * 3.14159265f * 10.0f * static_cast<float>(i) / static_cast<float>(fftSize));
    std::vector<std::complex<float>> freq;
    fft.forward(input, freq);
    std::vector<float> reconstructed;
    fft.inverse(freq, reconstructed);
    float maxErr = 0.0f;
    for (size_t i = 0; i < fftSize; ++i)
    {
        float err = std::abs(input[i] - reconstructed[i]);
        if (err > maxErr) maxErr = err;
    }
    TEST_ASSERT(maxErr < 1e-4f, "FFT -> IFFT round-trip max error must be < 1e-4");
    STFTEngine stft(512, 128, WindowType::Hann);
    std::vector<float> inBlock(1024);
    for (size_t i = 0; i < inBlock.size(); ++i) inBlock[i] = std::sin(static_cast<float>(i) * 0.1f);
    std::vector<float> outBlock(1024, 0.0f);
    stft.processBlock(inBlock.data(), outBlock.data(), inBlock.size(), [](std::vector<float>&, std::vector<float>&) {});
    float maxOlaErr = 0.0f;
    for (size_t i = 512; i < 900; ++i)
    {
        float err = std::abs(inBlock[i - 512] - outBlock[i]);
        if (err > maxOlaErr) maxOlaErr = err;
    }
    TEST_ASSERT(maxOlaErr < 0.05f, "STFT Overlap-Add pass-through reconstruction must preserve audio");
}

void testEnhancerAndLimiter()
{
    std::cout << "[TEST] Running Sound Enhancer & Lookahead Limiter Tests..." << std::endl;
    SoundEnhancer enhancer;
    enhancer.init(48000.0f);
    std::vector<float> testBlock(512, 0.5f);
    std::vector<float> enhOut(512, 0.0f);
    enhancer.processBlock(testBlock.data(), enhOut.data(), testBlock.size());
    bool noNan = true;
    for (float v : enhOut) if (std::isnan(v) || std::isinf(v)) noNan = false;
    TEST_ASSERT(noNan, "Sound Enhancer must produce clean, finite outputs without NaN or Inf");
    LookaheadLimiter limiter;
    limiter.init(48000.0f, 4.0f);
    LimiterParams lp;
    lp.ceilingDb = -0.5f;
    limiter.setParams(lp);
    float ceilingLin = std::pow(10.0f, -0.5f / 20.0f);
    std::vector<float> hotSignal(1024);
    for (size_t i = 0; i < hotSignal.size(); ++i) hotSignal[i] = 2.5f * std::sin(static_cast<float>(i) * 0.05f);
    std::vector<float> limitedSignal(1024);
    limiter.processBlock(hotSignal.data(), limitedSignal.data(), hotSignal.size());
    bool strictlyLimited = true;
    for (size_t i = 200; i < limitedSignal.size(); ++i)
    {
        if (std::abs(limitedSignal[i]) > ceilingLin + 1e-4f) strictlyLimited = false;
    }
    TEST_ASSERT(strictlyLimited, "Lookahead limiter must strictly clamp peaks below ceiling");
}

void testWavFileIO()
{
    std::cout << "[TEST] Running WAV File I/O Tests..." << std::endl;
    std::wstring testPath = L"test_audio_temp.wav";
    std::vector<float> writeData(48000);
    for (size_t i = 0; i < writeData.size(); ++i) writeData[i] = 0.5f * std::sin(2.0f * 3.14159265f * 440.0f * static_cast<float>(i) / 48000.0f);
    bool saved = WavFile::save(testPath, writeData, 48000, 1, 16);
    TEST_ASSERT(saved, "WAV file save must succeed");
    std::vector<float> readData;
    uint32_t sr = 0;
    uint16_t ch = 0;
    bool loaded = WavFile::load(testPath, readData, sr, ch);
    TEST_ASSERT(loaded, "WAV file load must succeed");
    TEST_ASSERT(sr == 48000, "Sample rate must be 48000");
    TEST_ASSERT(ch == 1, "Channel count must be 1");
    TEST_ASSERT(readData.size() == writeData.size(), "Sample count must match");
    _wremove(testPath.c_str());
}

int main()
{
    std::cout << "=========================================================" << std::endl;
    std::cout << " Neural DSP: Automated Unit Test & Verification Suite   " << std::endl;
    std::cout << "=========================================================" << std::endl;
    testQuantizedTensor();
    testQuantizedLayers();
    testDSPFFT();
    testEnhancerAndLimiter();
    testWavFileIO();
    std::cout << "=========================================================" << std::endl;
    std::cout << " Tests Passed: " << g_testsPassed << std::endl;
    std::cout << " Tests Failed: " << g_testsFailed << std::endl;
    std::cout << "=========================================================" << std::endl;
    return (g_testsFailed == 0) ? 0 : 1;
}
