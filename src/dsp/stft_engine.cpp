#include "stft_engine.h"
#include <numeric>

STFTEngine::STFTEngine(size_t frameSz, size_t hopSz, WindowType winType)
{
    init(frameSz, hopSz, winType);
}

void STFTEngine::init(size_t frameSz, size_t hopSz, WindowType winType)
{
    frameSize = frameSz;
    hopSize = hopSz;
    numBins = frameSize / 2 + 1;
    fft.init(frameSize);
    window.resize(frameSize);
    FFT::generateWindow(window, winType);
    float factor = static_cast<float>(frameSize) / static_cast<float>(hopSize);
    float winEnergy = 0.0f;
    for (float w : window) winEnergy += w * w;
    olaNormalization = 1.0f / (winEnergy * factor / static_cast<float>(frameSize));
    inBuffer.assign(frameSize, 0.0f);
    outBuffer.assign(frameSize * 2, 0.0f);
    timeFrame.assign(frameSize, 0.0f);
    magBins.assign(numBins, 0.0f);
    phaseBins.assign(numBins, 0.0f);
    inWritePos = 0;
    outReadPos = 0;
    hopCounter = 0;
}

void STFTEngine::reset()
{
    std::fill(inBuffer.begin(), inBuffer.end(), 0.0f);
    std::fill(outBuffer.begin(), outBuffer.end(), 0.0f);
    std::fill(timeFrame.begin(), timeFrame.end(), 0.0f);
    std::fill(magBins.begin(), magBins.end(), 0.0f);
    std::fill(phaseBins.begin(), phaseBins.end(), 0.0f);
    inWritePos = 0;
    outReadPos = 0;
    hopCounter = 0;
}

void STFTEngine::processSample(float inSample, float& outSample, const std::function<void(std::vector<float>& mag, std::vector<float>& phase)>& spectralProcessor)
{
    inBuffer[inWritePos] = inSample;
    inWritePos = (inWritePos + 1) % frameSize;
    outSample = outBuffer[outReadPos];
    outBuffer[outReadPos] = 0.0f;
    outReadPos = (outReadPos + 1) % outBuffer.size();
    hopCounter++;
    if (hopCounter >= hopSize)
    {
        hopCounter = 0;
        for (size_t i = 0; i < frameSize; ++i)
        {
            size_t readIdx = (inWritePos + i) % frameSize;
            timeFrame[i] = inBuffer[readIdx] * window[i];
        }
        fft.forward(timeFrame, freqFrame);
        fft.computeMagnitudeAndPhase(freqFrame, magBins, phaseBins);
        if (spectralProcessor) spectralProcessor(magBins, phaseBins);
        fft.reconstructFromMagnitudeAndPhase(magBins, phaseBins, freqFrame);
        fft.inverse(freqFrame, timeFrame);
        for (size_t i = 0; i < frameSize; ++i)
        {
            float synSample = timeFrame[i] * window[i] * olaNormalization;
            size_t targetIdx = (outReadPos + i) % outBuffer.size();
            outBuffer[targetIdx] += synSample;
        }
    }
}

void STFTEngine::processBlock(const float* inSamples, float* outSamples, size_t count, const std::function<void(std::vector<float>& mag, std::vector<float>& phase)>& spectralProcessor)
{
    for (size_t i = 0; i < count; ++i)
    {
        processSample(inSamples[i], outSamples[i], spectralProcessor);
    }
}

size_t STFTEngine::getFrameSize() const
{
    return frameSize;
}

size_t STFTEngine::getHopSize() const
{
    return hopSize;
}

size_t STFTEngine::getNumBins() const
{
    return numBins;
}

const std::vector<float>& STFTEngine::getAnalysisWindow() const
{
    return window;
}
