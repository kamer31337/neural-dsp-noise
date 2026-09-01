#pragma once

#include <vector>
#include <string>
#include <cstdint>

struct WavHeader
{
    char riffTag[4] = {'R', 'I', 'F', 'F'};
    uint32_t riffSize = 0;
    char waveTag[4] = {'W', 'A', 'V', 'E'};
    char fmtTag[4] = {'f', 'm', 't', ' '};
    uint32_t fmtSize = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = 1;
    uint32_t sampleRate = 48000;
    uint32_t byteRate = 96000;
    uint16_t blockAlign = 2;
    uint16_t bitsPerSample = 16;
    char dataTag[4] = {'d', 'a', 't', 'a'};
    uint32_t dataSize = 0;
};

class WavFile
{
public:
    static bool load(const std::wstring& path, std::vector<float>& samples, uint32_t& sampleRate, uint16_t& channels);
    static bool save(const std::wstring& path, const std::vector<float>& samples, uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample = 16);
};
