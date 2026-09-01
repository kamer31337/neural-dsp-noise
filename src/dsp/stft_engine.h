#pragma once

#include "fft.h"
#include <vector>
#include <functional>

class STFTEngine
{
public:
    STFTEngine() = default;
    STFTEngine(size_t frameSize, size_t hopSize, WindowType winType = WindowType::Hann);

    void init(size_t frameSize, size_t hopSize, WindowType winType = WindowType::Hann);
    void reset();
    void processSample(float inSample, float& outSample, const std::function<void(std::vector<float>& mag, std::vector<float>& phase)>& spectralProcessor);
    void processBlock(const float* inSamples, float* outSamples, size_t count, const std::function<void(std::vector<float>& mag, std::vector<float>& phase)>& spectralProcessor);

    size_t getFrameSize() const;
    size_t getHopSize() const;
    size_t getNumBins() const;
    const std::vector<float>& getAnalysisWindow() const;

private:
    size_t frameSize = 512;
    size_t hopSize = 128;
    size_t numBins = 257;
    float olaNormalization = 1.0f;
    FFT fft;
    std::vector<float> window;
    std::vector<float> inBuffer;
    std::vector<float> outBuffer;
    std::vector<float> timeFrame;
    std::vector<std::complex<float>> freqFrame;
    std::vector<float> magBins;
    std::vector<float> phaseBins;
    size_t inWritePos = 0;
    size_t outReadPos = 0;
    size_t hopCounter = 0;
};
