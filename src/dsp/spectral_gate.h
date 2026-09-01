#pragma once

#include <vector>

struct SpectralGateParams
{
    float thresholdDb = -45.0f;
    float reductionDb = -24.0f;
    float attackMs = 10.0f;
    float releaseMs = 50.0f;
    float learnRate = 0.05f;
    bool enabled = true;
    bool learnNoiseProfile = false;
};

class SpectralGate
{
public:
    SpectralGate() = default;
    explicit SpectralGate(size_t numBins);

    void init(size_t numBins);
    void reset();
    void process(std::vector<float>& magBins);
    void startLearningProfile();
    void stopLearningProfile();
    void clearNoiseProfile();
    void setParams(const SpectralGateParams& p);
    const SpectralGateParams& getParams() const;
    const std::vector<float>& getNoiseFloor() const;
    const std::vector<float>& getLearnedProfile() const;
    bool isLearning() const;

private:
    size_t numBins = 257;
    SpectralGateParams params;
    std::vector<float> noiseFloor;
    std::vector<float> learnedProfile;
    std::vector<float> smoothedGain;
    int learnFrameCount = 0;
    bool isLearningActive = false;
};
