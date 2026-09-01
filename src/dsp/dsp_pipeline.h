#pragma once

#include "stft_engine.h"
#include "spectral_gate.h"
#include "sound_enhancer.h"
#include "limiter.h"
#include "../neural/neural_denoiser.h"
#include <vector>
#include <mutex>
#include <atomic>

struct MasterDSPParams
{
    float inputGainDb = 0.0f;
    float outputGainDb = 0.0f;
    float wetDryMix = 1.0f;
    bool bypass = false;
    DenoiserParams denoiser;
    SpectralGateParams gate;
    EnhancerParams enhancer;
    LimiterParams limiter;
};

struct MeteringData
{
    float inputPeak = 0.0f;
    float inputRms = 0.0f;
    float outputPeak = 0.0f;
    float outputRms = 0.0f;
    float limiterGainReductionDb = 0.0f;
    std::vector<float> inSpectrum;
    std::vector<float> outSpectrum;
    std::vector<float> neuralMask;
    std::vector<float> inWaveform;
    std::vector<float> outWaveform;
};

class DSPPipeline
{
public:
    DSPPipeline();
    void init(float sampleRate = 48000.0f, size_t frameSize = 512, size_t hopSize = 128);
    void reset();
    void processBlock(const float* inSamples, float* outSamples, size_t numSamples);
    void setParams(const MasterDSPParams& p);
    MasterDSPParams getParams() const;
    void getMeteringData(MeteringData& data);
    void startNoiseLearning();
    void stopNoiseLearning();
    void clearNoiseProfile();
    const ModelProfileStats& getModelStats() const;
    float getSampleRate() const;

private:
    float sampleRate = 48000.0f;
    mutable std::mutex paramsMutex;
    mutable std::mutex meterMutex;
    MasterDSPParams params;
    STFTEngine stft;
    NeuralDenoiser neuralDenoiser;
    SpectralGate spectralGate;
    SoundEnhancer enhancer;
    LookaheadLimiter limiter;
    MeteringData currentMeters;
    std::vector<float> rawInSpectrum;
    std::vector<float> rawOutSpectrum;
    std::vector<float> lastMaskSpectrum;
    std::vector<float> waveInHistory;
    std::vector<float> waveOutHistory;
    size_t waveWritePos = 0;
};
