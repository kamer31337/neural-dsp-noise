#include "fft.h"
#include <algorithm>

constexpr float PI = 3.14159265358979323846f;

FFT::FFT(size_t fftSize)
{
    init(fftSize);
}

void FFT::init(size_t fftSize)
{
    size = 1;
    while (size < fftSize) size <<= 1;
    computeBitReversal();
    computeTwiddles();
}

size_t FFT::getSize() const
{
    return size;
}

void FFT::computeBitReversal()
{
    bitReverseTable.resize(size);
    size_t bits = 0;
    while ((1ULL << bits) < size) bits++;
    for (size_t i = 0; i < size; ++i)
    {
        size_t rev = 0;
        for (size_t b = 0; b < bits; ++b)
        {
            if (i & (1ULL << b)) rev |= (1ULL << (bits - 1 - b));
        }
        bitReverseTable[i] = rev;
    }
}

void FFT::computeTwiddles()
{
    twiddleFactors.resize(size / 2);
    for (size_t i = 0; i < size / 2; ++i)
    {
        float angle = -2.0f * PI * static_cast<float>(i) / static_cast<float>(size);
        twiddleFactors[i] = std::complex<float>(std::cos(angle), std::sin(angle));
    }
}

void FFT::forward(const std::vector<float>& realInput, std::vector<std::complex<float>>& complexOutput) const
{
    complexOutput.resize(size);
    for (size_t i = 0; i < size; ++i)
    {
        float val = (i < realInput.size()) ? realInput[i] : 0.0f;
        complexOutput[bitReverseTable[i]] = std::complex<float>(val, 0.0f);
    }
    for (size_t len = 2; len <= size; len <<= 1)
    {
        size_t halfLen = len >> 1;
        size_t step = size / len;
        for (size_t i = 0; i < size; i += len)
        {
            for (size_t j = 0; j < halfLen; ++j)
            {
                std::complex<float> u = complexOutput[i + j];
                std::complex<float> v = complexOutput[i + j + halfLen] * twiddleFactors[j * step];
                complexOutput[i + j] = u + v;
                complexOutput[i + j + halfLen] = u - v;
            }
        }
    }
}

void FFT::inverse(const std::vector<std::complex<float>>& complexInput, std::vector<float>& realOutput) const
{
    std::vector<std::complex<float>> temp(size);
    for (size_t i = 0; i < size; ++i)
    {
        std::complex<float> val = (i < complexInput.size()) ? complexInput[i] : std::complex<float>(0.0f, 0.0f);
        temp[bitReverseTable[i]] = std::conj(val);
    }
    for (size_t len = 2; len <= size; len <<= 1)
    {
        size_t halfLen = len >> 1;
        size_t step = size / len;
        for (size_t i = 0; i < size; i += len)
        {
            for (size_t j = 0; j < halfLen; ++j)
            {
                std::complex<float> u = temp[i + j];
                std::complex<float> v = temp[i + j + halfLen] * twiddleFactors[j * step];
                temp[i + j] = u + v;
                temp[i + j + halfLen] = u - v;
            }
        }
    }
    realOutput.resize(size);
    float invSize = 1.0f / static_cast<float>(size);
    for (size_t i = 0; i < size; ++i) realOutput[i] = temp[i].real() * invSize;
}

void FFT::computeMagnitudeAndPhase(const std::vector<std::complex<float>>& complexInput, std::vector<float>& mag, std::vector<float>& phase) const
{
    size_t numBins = size / 2 + 1;
    mag.resize(numBins);
    phase.resize(numBins);
    for (size_t i = 0; i < numBins; ++i)
    {
        mag[i] = std::abs(complexInput[i]);
        phase[i] = std::arg(complexInput[i]);
    }
}

void FFT::reconstructFromMagnitudeAndPhase(const std::vector<float>& mag, const std::vector<float>& phase, std::vector<std::complex<float>>& complexOutput) const
{
    complexOutput.resize(size);
    size_t numBins = size / 2 + 1;
    for (size_t i = 0; i < numBins; ++i) complexOutput[i] = std::polar(mag[i], phase[i]);
    for (size_t i = numBins; i < size; ++i) complexOutput[i] = std::conj(complexOutput[size - i]);
}

void FFT::generateWindow(std::vector<float>& win, WindowType type)
{
    size_t n = win.size();
    if (n == 0) return;
    for (size_t i = 0; i < n; ++i)
    {
        float frac = static_cast<float>(i) / static_cast<float>(n - 1);
        if (type == WindowType::Hann) win[i] = 0.5f * (1.0f - std::cos(2.0f * PI * frac));
        else if (type == WindowType::Hamming) win[i] = 0.54f - 0.46f * std::cos(2.0f * PI * frac);
        else if (type == WindowType::BlackmanHarris) win[i] = 0.35875f - 0.48829f * std::cos(2.0f * PI * frac) + 0.14128f * std::cos(4.0f * PI * frac) - 0.01168f * std::cos(6.0f * PI * frac);
        else win[i] = 1.0f;
    }
}
