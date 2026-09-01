#include "spectral_gate.h"
#include <cmath>
#include <algorithm>

SpectralGate::SpectralGate(size_t bins)
{
    init(bins);
}

void SpectralGate::init(size_t bins)
{
    numBins = bins;
    noiseFloor.assign(numBins, 0.001f);
    learnedProfile.assign(numBins, 0.001f);
    smoothedGain.assign(numBins, 1.0f);
    learnFrameCount = 0;
    isLearningActive = false;
}

void SpectralGate::reset()
{
    std::fill(noiseFloor.begin(), noiseFloor.end(), 0.001f);
    std::fill(smoothedGain.begin(), smoothedGain.end(), 1.0f);
}

void SpectralGate::startLearningProfile()
{
    isLearningActive = true;
    learnFrameCount = 0;
    std::fill(learnedProfile.begin(), learnedProfile.end(), 0.0f);
}

void SpectralGate::stopLearningProfile()
{
    isLearningActive = false;
}

void SpectralGate::clearNoiseProfile()
{
    std::fill(learnedProfile.begin(), learnedProfile.end(), 0.001f);
    isLearningActive = false;
    learnFrameCount = 0;
}

void SpectralGate::process(std::vector<float>& magBins)
{
    if (magBins.size() != numBins) return;
    if (isLearningActive)
    {
        for (size_t i = 0; i < numBins; ++i)
        {
            learnedProfile[i] = (learnedProfile[i] * static_cast<float>(learnFrameCount) + magBins[i]) / static_cast<float>(learnFrameCount + 1);
        }
        learnFrameCount++;
        if (learnFrameCount >= 80) isLearningActive = false;
    }
    for (size_t i = 0; i < numBins; ++i)
    {
        float m = magBins[i];
        if (m < noiseFloor[i]) noiseFloor[i] = 0.98f * noiseFloor[i] + 0.02f * m;
        else noiseFloor[i] = 0.999f * noiseFloor[i] + 0.001f * m;
    }
    if (!params.enabled) return;
    float threshLinear = std::pow(10.0f, params.thresholdDb / 20.0f);
    float minGain = std::pow(10.0f, params.reductionDb / 20.0f);
    float alphaAttack = 0.2f;
    float alphaRelease = 0.85f;
    for (size_t i = 0; i < numBins; ++i)
    {
        float m = magBins[i];
        float refNoise = std::max(noiseFloor[i], learnedProfile[i]);
        float snr = m / (refNoise + 1e-6f);
        float targetGain = 1.0f;
        if (snr < threshLinear * 20.0f)
        {
            float diff = (threshLinear * 20.0f - snr) / (threshLinear * 20.0f + 1e-6f);
            targetGain = std::max(minGain, 1.0f - diff);
        }
        float alpha = (targetGain > smoothedGain[i]) ? alphaAttack : alphaRelease;
        smoothedGain[i] = alpha * targetGain + (1.0f - alpha) * smoothedGain[i];
        magBins[i] = m * smoothedGain[i];
    }
}

void SpectralGate::setParams(const SpectralGateParams& p)
{
    params = p;
}

const SpectralGateParams& SpectralGate::getParams() const
{
    return params;
}

const std::vector<float>& SpectralGate::getNoiseFloor() const
{
    return noiseFloor;
}

const std::vector<float>& SpectralGate::getLearnedProfile() const
{
    return learnedProfile;
}

bool SpectralGate::isLearning() const
{
    return isLearningActive;
}
