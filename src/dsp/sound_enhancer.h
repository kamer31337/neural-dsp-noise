#pragma once

#include <vector>

struct EnhancerParams
{
    float bassDrive = 0.4f;
    float bassFreqHz = 90.0f;
    float airDrive = 0.5f;
    float airFreqHz = 8000.0f;
    float transientAttack = 0.3f;
    float transientSustain = 0.1f;
    float warmthDrive = 0.25f;
    float clarityBoost = 0.35f;
    bool enabled = true;
};

class BiquadFilter
{
public:
    enum class Type { LowPass, HighPass, BandPass, Peaking, HighShelf, LowShelf };
    BiquadFilter();
    void configure(Type type, float sampleRate, float cutoffHz, float q, float gainDb = 0.0f);
    float process(float in);
    void reset();

private:
    float b0 = 1.0f;
    float b1 = 0.0f;
    float b2 = 0.0f;
    float a1 = 0.0f;
    float a2 = 0.0f;
    float z1 = 0.0f;
    float z2 = 0.0f;
};

class SoundEnhancer
{
public:
    SoundEnhancer();
    void init(float sampleRate);
    void reset();
    float processSample(float in);
    void processBlock(const float* in, float* out, size_t count);
    void setParams(const EnhancerParams& p);
    const EnhancerParams& getParams() const;

private:
    float sampleRate = 48000.0f;
    EnhancerParams params;
    BiquadFilter bassLpf;
    BiquadFilter bassHpf;
    BiquadFilter airHpf;
    BiquadFilter warmthFilter;
    BiquadFilter clarityFilter;
    float fastEnv = 0.0f;
    float slowEnv = 0.0f;
    void updateFilters();
};
