#include "visualizers.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

SpectrumVisualizer::SpectrumVisualizer(const Gdiplus::RectF& rect)
{
    bounds = rect;
    smoothIn.assign(257, 0.001f);
    smoothOut.assign(257, 0.001f);
}

void SpectrumVisualizer::draw(Gdiplus::Graphics& g, const std::vector<float>& inSpectrum, const std::vector<float>& outSpectrum, float sampleRate)
{
    Gdiplus::SolidBrush bgBrush(UITheme::BgPanel);
    UITheme::fillRoundedRect(g, bgBrush, bounds.X, bounds.Y, bounds.Width, bounds.Height, 8.0f);
    Gdiplus::Pen borderPen(UITheme::BorderDark, 1.0f);
    UITheme::drawRoundedRect(g, borderPen, bounds.X, bounds.Y, bounds.Width, bounds.Height, 8.0f);
    Gdiplus::Pen gridPen(Gdiplus::Color(255, 36, 42, 54), 1.0f);
    Gdiplus::FontFamily fontFam(L"Segoe UI");
    Gdiplus::Font gridFont(&fontFam, 8.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush gridTextBrush(UITheme::TextMuted);
    Gdiplus::StringFormat strFmt;
    strFmt.SetAlignment(Gdiplus::StringAlignmentCenter);
    float freqs[] = {50.0f, 100.0f, 250.0f, 500.0f, 1000.0f, 2500.0f, 5000.0f, 10000.0f, 20000.0f};
    const wchar_t* freqLabels[] = {L"50", L"100", L"250", L"500", L"1k", L"2.5k", L"5k", L"10k", L"20k"};
    float minF = 20.0f;
    float maxF = 20000.0f;
    float logMin = std::log10(minF);
    float logMax = std::log10(maxF);
    for (int i = 0; i < 9; ++i)
    {
        float f = freqs[i];
        float normX = (std::log10(f) - logMin) / (logMax - logMin);
        float gx = bounds.X + normX * bounds.Width;
        g.DrawLine(&gridPen, gx, bounds.Y, gx, bounds.Y + bounds.Height);
        Gdiplus::RectF txtRect(gx - 15.0f, bounds.Y + bounds.Height - 12.0f, 30.0f, 12.0f);
        g.DrawString(freqLabels[i], -1, &gridFont, txtRect, &strFmt, &gridTextBrush);
    }
    float dbs[] = {0.0f, -12.0f, -24.0f, -36.0f, -48.0f, -60.0f};
    for (int i = 0; i < 6; ++i)
    {
        float normY = (dbs[i] - 6.0f) / (-66.0f);
        float gy = bounds.Y + normY * bounds.Height;
        g.DrawLine(&gridPen, bounds.X, gy, bounds.X + bounds.Width, gy);
    }
    if (inSpectrum.empty() || outSpectrum.empty()) return;
    size_t numBins = inSpectrum.size();
    if (smoothIn.size() != numBins) smoothIn.resize(numBins, 0.001f);
    if (smoothOut.size() != numBins) smoothOut.resize(numBins, 0.001f);
    for (size_t i = 0; i < numBins; ++i)
    {
        smoothIn[i] = 0.65f * smoothIn[i] + 0.35f * inSpectrum[i];
        smoothOut[i] = 0.65f * smoothOut[i] + 0.35f * outSpectrum[i];
    }
    std::vector<Gdiplus::PointF> inPoints;
    std::vector<Gdiplus::PointF> outPoints;
    float nyquist = sampleRate * 0.5f;
    for (size_t i = 1; i < numBins; ++i)
    {
        float binFreq = (static_cast<float>(i) / static_cast<float>(numBins - 1)) * nyquist;
        if (binFreq < minF || binFreq > maxF) continue;
        float normX = (std::log10(binFreq) - logMin) / (logMax - logMin);
        float px = bounds.X + normX * bounds.Width;
        float inDb = 20.0f * std::log10(std::max(0.00001f, smoothIn[i]));
        float outDb = 20.0f * std::log10(std::max(0.00001f, smoothOut[i]));
        float normInY = (inDb - 6.0f) / (-66.0f);
        float normOutY = (outDb - 6.0f) / (-66.0f);
        float pyIn = bounds.Y + std::max(0.0f, std::min(1.0f, normInY)) * bounds.Height;
        float pyOut = bounds.Y + std::max(0.0f, std::min(1.0f, normOutY)) * bounds.Height;
        inPoints.push_back(Gdiplus::PointF(px, pyIn));
        outPoints.push_back(Gdiplus::PointF(px, pyOut));
    }
    if (inPoints.size() > 2)
    {
        Gdiplus::Pen inPen(Gdiplus::Color(180, 255, 179, 0), 1.4f);
        g.DrawCurve(&inPen, inPoints.data(), static_cast<INT>(inPoints.size()), 0.5f);
    }
    if (outPoints.size() > 2)
    {
        Gdiplus::GraphicsPath fillPath;
        fillPath.AddLine(bounds.X, bounds.Y + bounds.Height, outPoints.front().X, outPoints.front().Y);
        for (size_t i = 1; i < outPoints.size(); ++i) fillPath.AddLine(outPoints[i - 1], outPoints[i]);
        fillPath.AddLine(outPoints.back().X, outPoints.back().Y, outPoints.back().X, bounds.Y + bounds.Height);
        fillPath.CloseFigure();
        Gdiplus::LinearGradientBrush fillGrad(Gdiplus::PointF(bounds.X, bounds.Y), Gdiplus::PointF(bounds.X, bounds.Y + bounds.Height), Gdiplus::Color(70, 0, 210, 255), Gdiplus::Color(5, 0, 210, 255));
        g.FillPath(&fillGrad, &fillPath);
        Gdiplus::Pen outPen(UITheme::CyanAccent, 2.0f);
        g.DrawCurve(&outPen, outPoints.data(), static_cast<INT>(outPoints.size()), 0.5f);
    }
    Gdiplus::Font legendFont(&fontFam, 9.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush amberBrush(UITheme::AmberAccent);
    Gdiplus::SolidBrush cyanBrush(UITheme::CyanAccent);
    g.DrawString(L"- - Input (Pre-DSP)", -1, &legendFont, Gdiplus::PointF(bounds.X + 10.0f, bounds.Y + 8.0f), &amberBrush);
    g.DrawString(L"— Output (Post-DSP)", -1, &legendFont, Gdiplus::PointF(bounds.X + 120.0f, bounds.Y + 8.0f), &cyanBrush);
}

NeuralMaskVisualizer::NeuralMaskVisualizer(const Gdiplus::RectF& rect)
{
    bounds = rect;
    smoothMask.assign(257, 1.0f);
}

void NeuralMaskVisualizer::draw(Gdiplus::Graphics& g, const std::vector<float>& maskBins, float sampleRate)
{
    (void)sampleRate;
    Gdiplus::SolidBrush bgBrush(UITheme::BgPanel);
    UITheme::fillRoundedRect(g, bgBrush, bounds.X, bounds.Y, bounds.Width, bounds.Height, 8.0f);
    Gdiplus::Pen borderPen(UITheme::BorderDark, 1.0f);
    UITheme::drawRoundedRect(g, borderPen, bounds.X, bounds.Y, bounds.Width, bounds.Height, 8.0f);
    Gdiplus::FontFamily fontFam(L"Segoe UI");
    Gdiplus::Font font(&fontFam, 9.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush titleBrush(UITheme::TextSecondary);
    g.DrawString(L"INT8 QUANTIZED NEURAL SPECTRAL ATTENUATION MASK (257 BINS)", -1, &font, Gdiplus::PointF(bounds.X + 10.0f, bounds.Y + 8.0f), &titleBrush);
    if (maskBins.empty()) return;
    size_t numBins = maskBins.size();
    if (smoothMask.size() != numBins) smoothMask.resize(numBins, 1.0f);
    for (size_t i = 0; i < numBins; ++i) smoothMask[i] = 0.7f * smoothMask[i] + 0.3f * maskBins[i];
    float barAreaY = bounds.Y + 28.0f;
    float barAreaH = bounds.Height - 36.0f;
    float barW = (bounds.Width - 16.0f) / static_cast<float>(numBins);
    for (size_t i = 0; i < numBins; ++i)
    {
        float m = std::max(0.0f, std::min(1.0f, smoothMask[i]));
        float barH = barAreaH * m;
        float bx = bounds.X + 8.0f + static_cast<float>(i) * barW;
        float by = barAreaY + (barAreaH - barH);
        Gdiplus::Color barCol = (m > 0.8f) ? UITheme::EmeraldAccent : (m > 0.4f ? UITheme::AmberAccent : UITheme::PurpleAccent);
        Gdiplus::SolidBrush barBrush(barCol);
        g.FillRectangle(&barBrush, bx, by, std::max(1.0f, barW - 0.5f), barH);
    }
}

OscilloscopeVisualizer::OscilloscopeVisualizer(const Gdiplus::RectF& rect)
{
    bounds = rect;
}

void OscilloscopeVisualizer::draw(Gdiplus::Graphics& g, const std::vector<float>& inWave, const std::vector<float>& outWave)
{
    Gdiplus::SolidBrush bgBrush(UITheme::BgPanel);
    UITheme::fillRoundedRect(g, bgBrush, bounds.X, bounds.Y, bounds.Width, bounds.Height, 8.0f);
    Gdiplus::Pen borderPen(UITheme::BorderDark, 1.0f);
    UITheme::drawRoundedRect(g, borderPen, bounds.X, bounds.Y, bounds.Width, bounds.Height, 8.0f);
    Gdiplus::Pen centerPen(Gdiplus::Color(255, 36, 42, 54), 1.0f);
    float cy = bounds.Y + bounds.Height * 0.5f;
    g.DrawLine(&centerPen, bounds.X, cy, bounds.X + bounds.Width, cy);
    if (!inWave.empty())
    {
        size_t countIn = inWave.size();
        std::vector<Gdiplus::PointF> inPoints(countIn);
        float stepInX = bounds.Width / static_cast<float>(countIn - 1);
        for (size_t i = 0; i < countIn; ++i)
        {
            float s = std::max(-1.0f, std::min(1.0f, inWave[i]));
            inPoints[i] = Gdiplus::PointF(bounds.X + static_cast<float>(i) * stepInX, cy - s * (bounds.Height * 0.45f));
        }
        Gdiplus::Pen inWavePen(Gdiplus::Color(120, 255, 179, 0), 1.0f);
        g.DrawLines(&inWavePen, inPoints.data(), static_cast<INT>(inPoints.size()));
    }
    if (!outWave.empty())
    {
        size_t count = outWave.size();
        std::vector<Gdiplus::PointF> points(count);
        float stepX = bounds.Width / static_cast<float>(count - 1);
        for (size_t i = 0; i < count; ++i)
        {
            float s = std::max(-1.0f, std::min(1.0f, outWave[i]));
            points[i] = Gdiplus::PointF(bounds.X + static_cast<float>(i) * stepX, cy - s * (bounds.Height * 0.45f));
        }
        Gdiplus::Pen wavePen(UITheme::EmeraldAccent, 1.5f);
        g.DrawLines(&wavePen, points.data(), static_cast<INT>(points.size()));
    }
    Gdiplus::FontFamily fontFam(L"Segoe UI");
    Gdiplus::Font font(&fontFam, 9.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush textBrush(UITheme::EmeraldAccent);
    g.DrawString(L"REAL-TIME OSCILLOSCOPE", -1, &font, Gdiplus::PointF(bounds.X + 10.0f, bounds.Y + 8.0f), &textBrush);
}

VUMeterControl::VUMeterControl(const std::wstring& lbl, const Gdiplus::RectF& rect)
{
    label = lbl;
    bounds = rect;
}

void VUMeterControl::draw(Gdiplus::Graphics& g, float peakLinear, float rmsLinear, float gainReductionDb)
{
    (void)rmsLinear;
    (void)gainReductionDb;
    Gdiplus::SolidBrush bgBrush(UITheme::BgControl);
    UITheme::fillRoundedRect(g, bgBrush, bounds.X, bounds.Y, bounds.Width, bounds.Height, 4.0f);
    float peakDb = 20.0f * std::log10(std::max(0.0001f, peakLinear));
    float normPeak = (peakDb + 60.0f) / 66.0f;
    normPeak = std::max(0.0f, std::min(1.0f, normPeak));
    smoothPeak = 0.6f * smoothPeak + 0.4f * normPeak;
    if (smoothPeak > peakHold)
    {
        peakHold = smoothPeak;
        peakHoldTimer = 25;
    }
    else
    {
        if (peakHoldTimer > 0) peakHoldTimer--;
        else peakHold = std::max(0.0f, peakHold - 0.015f);
    }
    float fillW = (bounds.Width - 4.0f) * smoothPeak;
    if (fillW > 1.0f)
    {
        Gdiplus::Color col = (smoothPeak > 0.9f) ? UITheme::CoralAccent : ((smoothPeak > 0.7f) ? UITheme::AmberAccent : UITheme::EmeraldAccent);
        Gdiplus::SolidBrush fillBrush(col);
        g.FillRectangle(&fillBrush, bounds.X + 2.0f, bounds.Y + 2.0f, fillW, bounds.Height - 4.0f);
    }
    if (peakHold > 0.05f)
    {
        float hx = bounds.X + 2.0f + (bounds.Width - 4.0f) * peakHold;
        Gdiplus::SolidBrush holdBrush(UITheme::TextPrimary);
        g.FillRectangle(&holdBrush, hx - 1.0f, bounds.Y + 1.0f, 2.0f, bounds.Height - 2.0f);
    }
    Gdiplus::FontFamily fontFam(L"Segoe UI");
    Gdiplus::Font font(&fontFam, 8.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::StringFormat strFmt;
    strFmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    Gdiplus::SolidBrush textBrush(UITheme::TextPrimary);
    Gdiplus::RectF lblRect(bounds.X + 6.0f, bounds.Y, bounds.Width - 12.0f, bounds.Height);
    g.DrawString(label.c_str(), -1, &font, lblRect, &strFmt, &textBrush);
}
