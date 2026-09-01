#include "dsp_pipeline.h"
#include <cmath>
#include <algorithm>

DSPPipeline::DSPPipeline()
{
    init(48000.0f, 512, 128);
}

void DSPPipeline::init(float sr, size_t frameSize, size_t hopSize)
{
    sampleRate = sr;
    stft.init(frameSize, hopSize, WindowType::Hann);
    spectralGate.init(stft.getNumBins());
    enhancer.init(sampleRate);
    limiter.init(sampleRate, 4.0f);
    rawInSpectrum.assign(stft.getNumBins(), 0.001f);
    rawOutSpectrum.assign(stft.getNumBins(), 0.001f);
    lastMaskSpectrum.assign(stft.getNumBins(), 1.0f);
    waveInHistory.assign(512, 0.0f);
    waveOutHistory.assign(512, 0.0f);
    currentMeters.inSpectrum.assign(stft.getNumBins(), 0.001f);
    currentMeters.outSpectrum.assign(stft.getNumBins(), 0.001f);
    currentMeters.neuralMask.assign(stft.getNumBins(), 1.0f);
    currentMeters.inWaveform.assign(512, 0.0f);
    currentMeters.outWaveform.assign(512, 0.0f);
    reset();
}

void DSPPipeline::reset()
{
    stft.reset();
    neuralDenoiser.reset();
    spectralGate.reset();
    enhancer.reset();
    limiter.reset();
    waveWritePos = 0;
}

void DSPPipeline::processBlock(const float* inSamples, float* outSamples, size_t numSamples)
{
    MasterDSPParams p;
    {
        std::lock_guard<std::mutex> lock(paramsMutex);
        p = params;
    }
    neuralDenoiser.setParams(p.denoiser);
    spectralGate.setParams(p.gate);
    enhancer.setParams(p.enhancer);
    limiter.setParams(p.limiter);
    float inGainLin = std::pow(10.0f, p.inputGainDb / 20.0f);
    float outGainLin = std::pow(10.0f, p.outputGainDb / 20.0f);
    float inPeakAcc = 0.0f;
    float inRmsAcc = 0.0f;
    float outPeakAcc = 0.0f;
    float outRmsAcc = 0.0f;
    std::vector<float> gainAdjustedIn(numSamples);
    for (size_t i = 0; i < numSamples; ++i)
    {
        float val = inSamples[i] * inGainLin;
        gainAdjustedIn[i] = val;
        float absVal = std::abs(val);
        if (absVal > inPeakAcc) inPeakAcc = absVal;
        inRmsAcc += val * val;
    }
    if (p.bypass)
    {
        for (size_t i = 0; i < numSamples; ++i)
        {
            float val = gainAdjustedIn[i] * outGainLin;
            outSamples[i] = val;
            float absVal = std::abs(val);
            if (absVal > outPeakAcc) outPeakAcc = absVal;
            outRmsAcc += val * val;
        }
    }
    else
    {
        std::vector<float> stftDenoised(numSamples);
        stft.processBlock(gainAdjustedIn.data(), stftDenoised.data(), numSamples, [&](std::vector<float>& mag, std::vector<float>& phase)
        {
            for (size_t k = 0; k < mag.size(); ++k) rawInSpectrum[k] = 0.8f * rawInSpectrum[k] + 0.2f * mag[k];
            spectralGate.process(mag);
            std::vector<float> neuralMag(mag.size());
            neuralDenoiser.processSpectrum(mag.data(), phase.data(), neuralMag.data(), lastMaskSpectrum.data(), static_cast<int>(mag.size()));
            for (size_t k = 0; k < mag.size(); ++k)
            {
                mag[k] = neuralMag[k];
                rawOutSpectrum[k] = 0.8f * rawOutSpectrum[k] + 0.2f * mag[k];
            }
        });
        std::vector<float> enhanced(numSamples);
        enhancer.processBlock(stftDenoised.data(), enhanced.data(), numSamples);
        std::vector<float> mixed(numSamples);
        for (size_t i = 0; i < numSamples; ++i)
        {
            mixed[i] = (1.0f - p.wetDryMix) * gainAdjustedIn[i] + p.wetDryMix * enhanced[i];
            mixed[i] *= outGainLin;
        }
        limiter.processBlock(mixed.data(), outSamples, numSamples);
        for (size_t i = 0; i < numSamples; ++i)
        {
            float val = outSamples[i];
            float absVal = std::abs(val);
            if (absVal > outPeakAcc) outPeakAcc = absVal;
            outRmsAcc += val * val;
        }
    }
    {
        std::lock_guard<std::mutex> lock(meterMutex);
        currentMeters.inputPeak = inPeakAcc;
        currentMeters.inputRms = (numSamples > 0) ? std::sqrt(inRmsAcc / static_cast<float>(numSamples)) : 0.0f;
        currentMeters.outputPeak = outPeakAcc;
        currentMeters.outputRms = (numSamples > 0) ? std::sqrt(outRmsAcc / static_cast<float>(numSamples)) : 0.0f;
        currentMeters.limiterGainReductionDb = limiter.getGainReductionDb();
        currentMeters.inSpectrum = rawInSpectrum;
        currentMeters.outSpectrum = rawOutSpectrum;
        currentMeters.neuralMask = lastMaskSpectrum;
        for (size_t i = 0; i < numSamples; ++i)
        {
            waveInHistory[waveWritePos] = gainAdjustedIn[i];
            waveOutHistory[waveWritePos] = outSamples[i];
            waveWritePos = (waveWritePos + 1) % waveInHistory.size();
        }
        currentMeters.inWaveform = waveInHistory;
        currentMeters.outWaveform = waveOutHistory;
    }
}

void DSPPipeline::setParams(const MasterDSPParams& p)
{
    std::lock_guard<std::mutex> lock(paramsMutex);
    params = p;
}

MasterDSPParams DSPPipeline::getParams() const
{
    std::lock_guard<std::mutex> lock(paramsMutex);
    return params;
}

void DSPPipeline::getMeteringData(MeteringData& data)
{
    std::lock_guard<std::mutex> lock(meterMutex);
    data = currentMeters;
}

void DSPPipeline::startNoiseLearning()
{
    spectralGate.startLearningProfile();
}

void DSPPipeline::stopNoiseLearning()
{
    spectralGate.stopLearningProfile();
}

void DSPPipeline::clearNoiseProfile()
{
    spectralGate.clearNoiseProfile();
}

const ModelProfileStats& DSPPipeline::getModelStats() const
{
    return neuralDenoiser.getModelStats();
}

float DSPPipeline::getSampleRate() const
{
    return sampleRate;
}
