#include "wav_file.h"
#include <cstdio>
#include <cmath>
#include <cstring>
#include <algorithm>

bool WavFile::load(const std::wstring& path, std::vector<float>& samples, uint32_t& sampleRate, uint16_t& channels)
{
    FILE* file = _wfopen(path.c_str(), L"rb");
    if (!file) return false;
    char riff[4];
    if (fread(riff, 1, 4, file) != 4 || std::memcmp(riff, "RIFF", 4) != 0)
    {
        fclose(file);
        return false;
    }
    uint32_t fileSize = 0;
    if (fread(&fileSize, 1, 4, file) != 4)
    {
        fclose(file);
        return false;
    }
    char wave[4];
    if (fread(wave, 1, 4, file) != 4 || std::memcmp(wave, "WAVE", 4) != 0)
    {
        fclose(file);
        return false;
    }
    uint16_t formatTag = 1;
    uint16_t numCh = 1;
    uint32_t sRate = 44100;
    uint16_t bitsPerSamp = 16;
    bool foundFmt = false;
    bool foundData = false;
    uint32_t dataBytes = 0;
    long dataPos = 0;
    while (!feof(file) && (!foundFmt || !foundData))
    {
        char chunkId[4];
        uint32_t chunkSize = 0;
        if (fread(chunkId, 1, 4, file) != 4) break;
        if (fread(&chunkSize, 1, 4, file) != 4) break;
        if (std::memcmp(chunkId, "fmt ", 4) == 0)
        {
            if (fread(&formatTag, 1, 2, file) != 2) break;
            if (fread(&numCh, 1, 2, file) != 2) break;
            if (fread(&sRate, 1, 4, file) != 4) break;
            uint32_t byteRate = 0;
            if (fread(&byteRate, 1, 4, file) != 4) break;
            uint16_t blockAlign = 0;
            if (fread(&blockAlign, 1, 2, file) != 2) break;
            if (fread(&bitsPerSamp, 1, 2, file) != 2) break;
            if (chunkSize > 16) fseek(file, static_cast<long>(chunkSize - 16), SEEK_CUR);
            foundFmt = true;
        }
        else if (std::memcmp(chunkId, "data", 4) == 0)
        {
            dataBytes = chunkSize;
            dataPos = ftell(file);
            foundData = true;
            fseek(file, static_cast<long>(chunkSize), SEEK_CUR);
        }
        else
        {
            fseek(file, static_cast<long>(chunkSize), SEEK_CUR);
        }
    }
    if (!foundFmt || !foundData)
    {
        fclose(file);
        return false;
    }
    sampleRate = sRate;
    channels = numCh;
    fseek(file, dataPos, SEEK_SET);
    size_t numSamples = (bitsPerSamp > 0) ? (dataBytes / (bitsPerSamp / 8)) : 0;
    samples.resize(numSamples);
    if (formatTag == 1 && bitsPerSamp == 16)
    {
        std::vector<int16_t> raw(numSamples);
        fread(raw.data(), 1, dataBytes, file);
        for (size_t i = 0; i < numSamples; ++i) samples[i] = static_cast<float>(raw[i]) / 32768.0f;
    }
    else if (formatTag == 1 && bitsPerSamp == 24)
    {
        std::vector<uint8_t> raw(dataBytes);
        fread(raw.data(), 1, dataBytes, file);
        for (size_t i = 0; i < numSamples; ++i)
        {
            int32_t val = (raw[i * 3 + 2] << 24) | (raw[i * 3 + 1] << 16) | (raw[i * 3] << 8);
            val >>= 8;
            samples[i] = static_cast<float>(val) / 8388608.0f;
        }
    }
    else if (formatTag == 3 && bitsPerSamp == 32)
    {
        fread(samples.data(), 1, dataBytes, file);
    }
    else
    {
        fclose(file);
        return false;
    }
    fclose(file);
    return true;
}

bool WavFile::save(const std::wstring& path, const std::vector<float>& samples, uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample)
{
    FILE* file = _wfopen(path.c_str(), L"wb");
    if (!file) return false;
    WavHeader hdr;
    hdr.numChannels = channels;
    hdr.sampleRate = sampleRate;
    hdr.bitsPerSample = bitsPerSample;
    hdr.blockAlign = channels * (bitsPerSample / 8);
    hdr.byteRate = sampleRate * hdr.blockAlign;
    uint32_t dataBytes = static_cast<uint32_t>(samples.size() * (bitsPerSample / 8));
    hdr.dataSize = dataBytes;
    hdr.riffSize = 36 + dataBytes;
    hdr.audioFormat = (bitsPerSample == 32) ? 3 : 1;
    fwrite(&hdr, sizeof(WavHeader), 1, file);
    if (bitsPerSample == 16)
    {
        std::vector<int16_t> pcm(samples.size());
        for (size_t i = 0; i < samples.size(); ++i)
        {
            float s = std::max(-1.0f, std::min(1.0f, samples[i]));
            pcm[i] = static_cast<int16_t>(std::round(s * 32767.0f));
        }
        fwrite(pcm.data(), 1, dataBytes, file);
    }
    else if (bitsPerSample == 32)
    {
        fwrite(samples.data(), 1, dataBytes, file);
    }
    fclose(file);
    return true;
}
