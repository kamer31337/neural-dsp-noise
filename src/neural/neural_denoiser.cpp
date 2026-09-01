#include "neural_denoiser.h"
#include <cmath>
#include <algorithm>

NeuralDenoiser::NeuralDenoiser()
{
    prevMag.resize(QuantizedSpectralMaskNet::NUM_BINS, 0.0f);
    smoothedMask.resize(QuantizedSpectralMaskNet::NUM_BINS, 1.0f);
    lastComputedMask.resize(QuantizedSpectralMaskNet::NUM_BINS, 1.0f);
}

void NeuralDenoiser::reset()
{
    net.resetState();
    std::fill(prevMag.begin(), prevMag.end(), 0.0f);
    std::fill(smoothedMask.begin(), smoothedMask.end(), 1.0f);
    std::fill(lastComputedMask.begin(), lastComputedMask.end(), 1.0f);
}

void NeuralDenoiser::processSpectrum(const float* inMag, const float* inPhase, float* outMag, float* outMask, int numBins)
{
    (void)inPhase;
    int n = std::min(numBins, QuantizedSpectralMaskNet::NUM_BINS);
    if (!params.enabled)
    {
        for (int i = 0; i < n; ++i)
        {
            outMag[i] = inMag[i];
            if (outMask) outMask[i] = 1.0f;
            lastComputedMask[i] = 1.0f;
        }
        return;
    }
    std::vector<float> rawMask(QuantizedSpectralMaskNet::NUM_BINS, 1.0f);
    net.forward(inMag, rawMask.data(), params.suppressionStrength);
    for (int i = 0; i < n; ++i)
    {
        float deltaMag = inMag[i] - prevMag[i];
        float transientBonus = (deltaMag > 0.05f) ? (deltaMag * params.transientPreserve * 2.0f) : 0.0f;
        float targetMask = std::min(1.0f, rawMask[i] + transientBonus);
        targetMask = std::pow(targetMask, params.suppressionStrength);
        targetMask = std::max(params.spectralFloor, targetMask);
        float alpha = (targetMask > smoothedMask[i]) ? 0.3f : params.maskSmoothing;
        smoothedMask[i] = alpha * smoothedMask[i] + (1.0f - alpha) * targetMask;
        lastComputedMask[i] = smoothedMask[i];
        outMag[i] = inMag[i] * smoothedMask[i];
        if (outMask) outMask[i] = smoothedMask[i];
        prevMag[i] = inMag[i];
    }
}

void NeuralDenoiser::setParams(const DenoiserParams& p)
{
    params = p;
}

const DenoiserParams& NeuralDenoiser::getParams() const
{
    return params;
}

const ModelProfileStats& NeuralDenoiser::getModelStats() const
{
    return net.stats;
}

const std::vector<float>& NeuralDenoiser::getLastMask() const
{
    return lastComputedMask;
}
