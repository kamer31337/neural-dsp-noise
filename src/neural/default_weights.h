#pragma once

#include <cstdint>

namespace DefaultModelWeights
{
    static inline void generateCalibratedWeights(int inBins, int embedDim, int gruHidden, int8_t* inWeights, int8_t* convWeights, int8_t* gruIH, int8_t* gruHH, int8_t* outWeights)
    {
        for (int r = 0; r < embedDim; ++r)
        {
            float centerBin = static_cast<float>(r) * (static_cast<float>(inBins) / static_cast<float>(embedDim));
            for (int c = 0; c < inBins; ++c)
            {
                float dist = std::abs(static_cast<float>(c) - centerBin);
                float w = std::exp(-0.5f * (dist * dist) / 16.0f) * 80.0f;
                if (dist > 8.0f) w = -10.0f * std::exp(-0.1f * dist);
                int8_t q = static_cast<int8_t>(std::max(-127.0f, std::min(127.0f, w)));
                inWeights[r * inBins + c] = q;
            }
        }
        for (int k = 0; k < 3; ++k)
        {
            float kw = (k == 1) ? 75.0f : 25.0f;
            convWeights[k] = static_cast<int8_t>(kw);
        }
        for (int g = 0; g < 3 * gruHidden; ++g)
        {
            for (int c = 0; c < embedDim; ++c)
            {
                float val = 30.0f * std::sin(static_cast<float>(g * 7 + c * 13) * 0.1f);
                gruIH[g * embedDim + c] = static_cast<int8_t>(val);
            }
        }
        for (int g = 0; g < 3 * gruHidden; ++g)
        {
            for (int h = 0; h < gruHidden; ++h)
            {
                float val = (g % gruHidden == h) ? 60.0f : 15.0f * std::cos(static_cast<float>(g + h));
                gruHH[g * gruHidden + h] = static_cast<int8_t>(val);
            }
        }
        for (int r = 0; r < inBins; ++r)
        {
            float relFreq = static_cast<float>(r) / static_cast<float>(inBins);
            for (int c = 0; c < gruHidden; ++c)
            {
                float targetCh = static_cast<float>(c) / static_cast<float>(gruHidden);
                float dist = std::abs(relFreq - targetCh);
                float w = 90.0f * std::exp(-0.5f * (dist * dist) / 0.05f) - 20.0f;
                outWeights[r * gruHidden + c] = static_cast<int8_t>(std::max(-127.0f, std::min(127.0f, w)));
            }
        }
    }
}
