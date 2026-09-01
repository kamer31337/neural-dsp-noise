#pragma once

#include "gui_theme.h"
#include <vector>
#include <string>

class SpectrumVisualizer
{
public:
    Gdiplus::RectF bounds;

    SpectrumVisualizer() = default;
    explicit SpectrumVisualizer(const Gdiplus::RectF& rect);

    void draw(Gdiplus::Graphics& g, const std::vector<float>& inSpectrum, const std::vector<float>& outSpectrum, float sampleRate = 48000.0f);

private:
    std::vector<float> smoothIn;
    std::vector<float> smoothOut;
};

class NeuralMaskVisualizer
{
public:
    Gdiplus::RectF bounds;

    NeuralMaskVisualizer() = default;
    explicit NeuralMaskVisualizer(const Gdiplus::RectF& rect);

    void draw(Gdiplus::Graphics& g, const std::vector<float>& maskBins, float sampleRate = 48000.0f);

private:
    std::vector<float> smoothMask;
};

class OscilloscopeVisualizer
{
public:
    Gdiplus::RectF bounds;

    OscilloscopeVisualizer() = default;
    explicit OscilloscopeVisualizer(const Gdiplus::RectF& rect);

    void draw(Gdiplus::Graphics& g, const std::vector<float>& inWave, const std::vector<float>& outWave);
};

class VUMeterControl
{
public:
    Gdiplus::RectF bounds;
    std::wstring label;

    VUMeterControl() = default;
    VUMeterControl(const std::wstring& lbl, const Gdiplus::RectF& rect);

    void draw(Gdiplus::Graphics& g, float peakLinear, float rmsLinear, float gainReductionDb = 0.0f);

private:
    float smoothPeak = 0.0f;
    float peakHold = 0.0f;
    int peakHoldTimer = 0;
};
