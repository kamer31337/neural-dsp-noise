#pragma once

#include <vector>
#include <complex>
#include <cmath>

enum class WindowType
{
    Rectangular = 0,
    Hann,
    Hamming,
    BlackmanHarris
};

class FFT
{
public:
    FFT() = default;
    explicit FFT(size_t fftSize);

    void init(size_t fftSize);
    void forward(const std::vector<float>& realInput, std::vector<std::complex<float>>& complexOutput) const;
    void inverse(const std::vector<std::complex<float>>& complexInput, std::vector<float>& realOutput) const;
    void computeMagnitudeAndPhase(const std::vector<std::complex<float>>& complexInput, std::vector<float>& mag, std::vector<float>& phase) const;
    void reconstructFromMagnitudeAndPhase(const std::vector<float>& mag, const std::vector<float>& phase, std::vector<std::complex<float>>& complexOutput) const;

    static void generateWindow(std::vector<float>& win, WindowType type);
    size_t getSize() const;

private:
    size_t size = 0;
    std::vector<size_t> bitReverseTable;
    std::vector<std::complex<float>> twiddleFactors;
    void computeBitReversal();
    void computeTwiddles();
};
