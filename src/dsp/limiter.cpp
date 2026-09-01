#include "limiter.h"
#include <cmath>
#include <algorithm>

LookaheadLimiter::LookaheadLimiter()
{
    init(48000.0f, 5.0f);
}

void LookaheadLimiter::init(float sr, float maxLookaheadMs)
{
    sampleRate = sr;
    size_t bufSize = static_cast<size_t>(sampleRate * (maxLookaheadMs / 1000.0f)) + 16;
    delayBuffer.assign(bufSize, 0.0f);
    writePos = 0;
    updateParameters();
    reset();
}

void LookaheadLimiter::reset()
{
    std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
    writePos = 0;
    currentGain = 1.0f;
    currentReductionDb = 0.0f;
}

void LookaheadLimiter::updateParameters()
{
    lookaheadSamples = static_cast<size_t>(sampleRate * (params.lookaheadMs / 1000.0f));
    if (lookaheadSamples >= delayBuffer.size()) lookaheadSamples = delayBuffer.size() - 1;
    if (lookaheadSamples < 1) lookaheadSamples = 1;
    float releaseSec = std::max(0.005f, params.releaseMs / 1000.0f);
    releaseCoeff = std::exp(-1.0f / (releaseSec * sampleRate));
}

float LookaheadLimiter::processSample(float in)
{
    if (!params.enabled) return in;
    delayBuffer[writePos] = in;
    size_t readPos = (writePos + delayBuffer.size() - lookaheadSamples) % delayBuffer.size();
    float delayedSample = delayBuffer[readPos];
    writePos = (writePos + 1) % delayBuffer.size();
    float absIn = std::abs(in);
    float ceilingLin = std::pow(10.0f, params.ceilingDb / 20.0f);
    float targetGain = 1.0f;
    if (absIn > ceilingLin) targetGain = ceilingLin / (absIn + 1e-6f);
    if (targetGain < currentGain) currentGain = targetGain;
    else currentGain = releaseCoeff * currentGain + (1.0f - releaseCoeff) * targetGain;
    float limited = delayedSample * currentGain;
    limited = std::max(-ceilingLin, std::min(ceilingLin, limited));
    currentReductionDb = 20.0f * std::log10(std::max(0.0001f, currentGain));
    return limited;
}

void LookaheadLimiter::processBlock(const float* in, float* out, size_t count)
{
    for (size_t i = 0; i < count; ++i) out[i] = processSample(in[i]);
}

void LookaheadLimiter::setParams(const LimiterParams& p)
{
    params = p;
    updateParameters();
}

const LimiterParams& LookaheadLimiter::getParams() const
{
    return params;
}

float LookaheadLimiter::getGainReductionDb() const
{
    return currentReductionDb;
}
