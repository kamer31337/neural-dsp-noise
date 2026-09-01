#pragma once

#include <windows.h>
#include <gdiplus.h>
#ifdef _MSC_VER
#pragma comment(lib, "gdiplus.lib")
#endif

namespace UITheme
{
    const Gdiplus::Color BgDark(255, 18, 20, 26);
    const Gdiplus::Color BgPanel(255, 26, 30, 39);
    const Gdiplus::Color BgPanelLight(255, 34, 40, 52);
    const Gdiplus::Color BgControl(255, 42, 49, 64);
    const Gdiplus::Color BorderDark(255, 48, 56, 73);
    const Gdiplus::Color BorderGlow(255, 0, 210, 255);
    const Gdiplus::Color CyanAccent(255, 0, 210, 255);
    const Gdiplus::Color EmeraldAccent(255, 0, 230, 118);
    const Gdiplus::Color AmberAccent(255, 255, 179, 0);
    const Gdiplus::Color CoralAccent(255, 255, 82, 82);
    const Gdiplus::Color PurpleAccent(255, 186, 104, 255);
    const Gdiplus::Color TextPrimary(255, 240, 244, 248);
    const Gdiplus::Color TextSecondary(255, 148, 163, 184);
    const Gdiplus::Color TextMuted(255, 100, 116, 139);

    static inline void drawRoundedRect(Gdiplus::Graphics& g, const Gdiplus::Pen& pen, float x, float y, float w, float h, float r)
    {
        Gdiplus::GraphicsPath path;
        path.AddArc(x, y, 2.0f * r, 2.0f * r, 180.0f, 90.0f);
        path.AddArc(x + w - 2.0f * r, y, 2.0f * r, 2.0f * r, 270.0f, 90.0f);
        path.AddArc(x + w - 2.0f * r, y + h - 2.0f * r, 2.0f * r, 2.0f * r, 0.0f, 90.0f);
        path.AddArc(x, y + h - 2.0f * r, 2.0f * r, 2.0f * r, 90.0f, 90.0f);
        path.CloseFigure();
        g.DrawPath(&pen, &path);
    }

    static inline void fillRoundedRect(Gdiplus::Graphics& g, const Gdiplus::Brush& brush, float x, float y, float w, float h, float r)
    {
        Gdiplus::GraphicsPath path;
        path.AddArc(x, y, 2.0f * r, 2.0f * r, 180.0f, 90.0f);
        path.AddArc(x + w - 2.0f * r, y, 2.0f * r, 2.0f * r, 270.0f, 90.0f);
        path.AddArc(x + w - 2.0f * r, y + h - 2.0f * r, 2.0f * r, 2.0f * r, 0.0f, 90.0f);
        path.AddArc(x, y + h - 2.0f * r, 2.0f * r, 2.0f * r, 90.0f, 90.0f);
        path.CloseFigure();
        g.FillPath(&brush, &path);
    }
}
