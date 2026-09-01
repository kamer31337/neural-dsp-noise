#pragma once

#include "quantized_layers.h"
#include <vector>

struct DenoiserParams
{
    float suppressionStrength = 1.0f;
    float spectralFloor = 0.05f;
    float maskSmoothing = 0.65f;
    float transientPreserve = 0.4f;
    bool enabled = true;
};

class NeuralDenoiser
{
public:
    NeuralDenoiser();
    void reset();
    void processSpectrum(const float* inMag, const float* inPhase, float* outMag, float* outMask, int numBins);
    void setParams(const DenoiserParams& p);
    const DenoiserParams& getParams() const;
    const ModelProfileStats& getModelStats() const;
    const std::vector<float>& getLastMask() const;

private:
    QuantizedSpectralMaskNet net;
    DenoiserParams params;
    std::vector<float> prevMag;
    std::vector<float> smoothedMask;
    std::vector<float> lastComputedMask;
};
