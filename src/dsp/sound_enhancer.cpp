#include "sound_enhancer.h"
#include <cmath>
#include <algorithm>

constexpr float M_PI_F = 3.14159265358979323846f;

BiquadFilter::BiquadFilter()
{
    reset();
}

void BiquadFilter::reset()
{
    z1 = 0.0f;
    z2 = 0.0f;
}

void BiquadFilter::configure(Type type, float sampleRate, float cutoffHz, float q, float gainDb)
{
    if (cutoffHz <= 10.0f) cutoffHz = 10.0f;
    if (cutoffHz >= sampleRate * 0.49f) cutoffHz = sampleRate * 0.49f;
    if (q <= 0.01f) q = 0.01f;
    float w0 = 2.0f * M_PI_F * cutoffHz / sampleRate;
    float cosw0 = std::cos(w0);
    float sinw0 = std::sin(w0);
    float alpha = sinw0 / (2.0f * q);
    float A = std::pow(10.0f, gainDb / 40.0f);
    float a0 = 1.0f;
    if (type == Type::LowPass)
    {
        b0 = (1.0f - cosw0) * 0.5f;
        b1 = 1.0f - cosw0;
        b2 = (1.0f - cosw0) * 0.5f;
        a0 = 1.0f + alpha;
        a1 = -2.0f * cosw0;
        a2 = 1.0f - alpha;
    }
    else if (type == Type::HighPass)
    {
        b0 = (1.0f + cosw0) * 0.5f;
        b1 = -(1.0f + cosw0);
        b2 = (1.0f + cosw0) * 0.5f;
        a0 = 1.0f + alpha;
        a1 = -2.0f * cosw0;
        a2 = 1.0f - alpha;
    }
    else if (type == Type::Peaking)
    {
        b0 = 1.0f + alpha * A;
        b1 = -2.0f * cosw0;
        b2 = 1.0f - alpha * A;
        a0 = 1.0f + alpha / A;
        a1 = -2.0f * cosw0;
        a2 = 1.0f - alpha / A;
    }
    else if (type == Type::HighShelf)
    {
        float sqrtA = std::sqrt(A);
        b0 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha);
        b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0);
        b2 = A * ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha);
        a0 = (A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha;
        a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0);
        a2 = (A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha;
    }
    else
    {
        b0 = 1.0f;
        b1 = 0.0f;
        b2 = 0.0f;
        a0 = 1.0f;
        a1 = 0.0f;
        a2 = 0.0f;
    }
    float invA0 = 1.0f / a0;
    b0 *= invA0;
    b1 *= invA0;
    b2 *= invA0;
    a1 *= invA0;
    a2 *= invA0;
}

float BiquadFilter::process(float in)
{
    float out = b0 * in + z1;
    z1 = b1 * in - a1 * out + z2;
    z2 = b2 * in - a2 * out;
    return out;
}

SoundEnhancer::SoundEnhancer()
{
    init(48000.0f);
}

void SoundEnhancer::init(float sr)
{
    sampleRate = sr;
    updateFilters();
    reset();
}

void SoundEnhancer::reset()
{
    bassLpf.reset();
    bassHpf.reset();
    airHpf.reset();
    warmthFilter.reset();
    clarityFilter.reset();
    fastEnv = 0.0f;
    slowEnv = 0.0f;
}

void SoundEnhancer::updateFilters()
{
    bassLpf.configure(BiquadFilter::Type::LowPass, sampleRate, params.bassFreqHz, 0.707f);
    bassHpf.configure(BiquadFilter::Type::HighPass, sampleRate, params.bassFreqHz * 1.5f, 0.707f);
    airHpf.configure(BiquadFilter::Type::HighShelf, sampleRate, params.airFreqHz, 0.707f, params.airDrive * 12.0f);
    warmthFilter.configure(BiquadFilter::Type::Peaking, sampleRate, 300.0f, 1.2f, params.warmthDrive * 6.0f);
    clarityFilter.configure(BiquadFilter::Type::Peaking, sampleRate, 3500.0f, 1.5f, params.clarityBoost * 8.0f);
}

float SoundEnhancer::processSample(float in)
{
    if (!params.enabled) return in;
    float absIn = std::abs(in);
    float fastAlpha = 0.05f;
    float slowAlpha = 0.002f;
    fastEnv = fastAlpha * absIn + (1.0f - fastAlpha) * fastEnv;
    slowEnv = slowAlpha * absIn + (1.0f - slowAlpha) * slowEnv;
    float diff = fastEnv - slowEnv;
    float transientMod = 1.0f;
    if (diff > 0.0f) transientMod += diff * params.transientAttack * 3.0f;
    else transientMod += diff * params.transientSustain * 1.5f;
    transientMod = std::max(0.2f, std::min(2.5f, transientMod));
    float shapedIn = in * transientMod;
    float bassLow = bassLpf.process(shapedIn);
    float clippedBass = std::tanh(bassLow * (1.0f + params.bassDrive * 3.0f));
    float satBass = clippedBass - 0.2f * clippedBass * clippedBass;
    float bassHarmonics = bassHpf.process(satBass) * params.bassDrive * 1.5f;
    float warmed = warmthFilter.process(shapedIn);
    float clear = clarityFilter.process(warmed);
    float aired = airHpf.process(clear);
    float out = aired + bassHarmonics;
    return out;
}

void SoundEnhancer::processBlock(const float* in, float* out, size_t count)
{
    for (size_t i = 0; i < count; ++i) out[i] = processSample(in[i]);
}

void SoundEnhancer::setParams(const EnhancerParams& p)
{
    params = p;
    updateFilters();
}

const EnhancerParams& SoundEnhancer::getParams() const
{
    return params;
}
