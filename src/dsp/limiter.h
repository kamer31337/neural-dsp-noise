#pragma once

#include <vector>

struct LimiterParams
{
    float ceilingDb = -0.3f;
    float releaseMs = 80.0f;
    float lookaheadMs = 2.5f;
    bool enabled = true;
};

class LookaheadLimiter
{
public:
    LookaheadLimiter();
    void init(float sampleRate, float maxLookaheadMs = 5.0f);
    void reset();
    float processSample(float in);
    void processBlock(const float* in, float* out, size_t count);
    void setParams(const LimiterParams& p);
    const LimiterParams& getParams() const;
    float getGainReductionDb() const;

private:
    float sampleRate = 48000.0f;
    LimiterParams params;
    std::vector<float> delayBuffer;
    size_t writePos = 0;
    size_t lookaheadSamples = 120;
    float currentGain = 1.0f;
    float releaseCoeff = 0.999f;
    float currentReductionDb = 0.0f;
    void updateParameters();
};
