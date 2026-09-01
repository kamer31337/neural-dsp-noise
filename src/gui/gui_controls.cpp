#include "gui_controls.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

KnobControl::KnobControl(const std::wstring& lbl, const std::wstring& u, float minV, float maxV, float defV, const Gdiplus::RectF& rect, const Gdiplus::Color& accent)
{
    label = lbl;
    unit = u;
    minVal = minV;
    maxVal = maxV;
    defaultVal = defV;
    currentVal = defV;
    bounds = rect;
    accentColor = accent;
}

void KnobControl::draw(Gdiplus::Graphics& g)
{
    float cx = bounds.X + bounds.Width * 0.5f;
    float cy = bounds.Y + bounds.Height * 0.42f;
    float radius = std::min(bounds.Width, bounds.Height) * 0.34f;
    Gdiplus::SolidBrush bgBrush(UITheme::BgControl);
    g.FillEllipse(&bgBrush, cx - radius, cy - radius, 2.0f * radius, 2.0f * radius);
    Gdiplus::Pen ringPen(UITheme::BorderDark, 3.5f);
    g.DrawArc(&ringPen, cx - radius, cy - radius, 2.0f * radius, 2.0f * radius, 135.0f, 270.0f);
    float normVal = (currentVal - minVal) / (maxVal - minVal + 1e-6f);
    normVal = std::max(0.0f, std::min(1.0f, normVal));
    float sweepAngle = normVal * 270.0f;
    if (sweepAngle > 0.5f)
    {
        Gdiplus::Pen activePen(accentColor, 4.0f);
        g.DrawArc(&activePen, cx - radius, cy - radius, 2.0f * radius, 2.0f * radius, 135.0f, sweepAngle);
    }
    float innerR = radius * 0.72f;
    Gdiplus::SolidBrush innerBrush(UITheme::BgPanel);
    g.FillEllipse(&innerBrush, cx - innerR, cy - innerR, 2.0f * innerR, 2.0f * innerR);
    float indicatorAngle = (135.0f + sweepAngle) * 3.14159265f / 180.0f;
    float dotX = cx + std::cos(indicatorAngle) * (radius * 0.55f);
    float dotY = cy + std::sin(indicatorAngle) * (radius * 0.55f);
    Gdiplus::SolidBrush dotBrush(accentColor);
    g.FillEllipse(&dotBrush, dotX - 2.5f, dotY - 2.5f, 5.0f, 5.0f);
    Gdiplus::FontFamily fontFam(L"Segoe UI");
    Gdiplus::Font labelFont(&fontFam, 9.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::Font valFont(&fontFam, 10.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::StringFormat strFmt;
    strFmt.SetAlignment(Gdiplus::StringAlignmentCenter);
    strFmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    std::wstringstream ss;
    if (std::abs(currentVal) < 10.0f) ss << std::fixed << std::setprecision(1) << currentVal;
    else ss << std::fixed << std::setprecision(0) << currentVal;
    ss << L" " << unit;
    std::wstring valStr = ss.str();
    Gdiplus::SolidBrush valBrush(UITheme::TextPrimary);
    Gdiplus::RectF valRect(bounds.X, cy - 6.0f, bounds.Width, 12.0f);
    g.DrawString(valStr.c_str(), -1, &valFont, valRect, &strFmt, &valBrush);
    Gdiplus::SolidBrush textBrush(UITheme::TextSecondary);
    Gdiplus::RectF lblRect(bounds.X, bounds.Y + bounds.Height - 16.0f, bounds.Width, 16.0f);
    g.DrawString(label.c_str(), -1, &labelFont, lblRect, &strFmt, &textBrush);
}

bool KnobControl::onMouseDown(int x, int y)
{
    if (bounds.Contains(static_cast<float>(x), static_cast<float>(y)))
    {
        isDragging = true;
        lastMouseY = y;
        return true;
    }
    return false;
}

bool KnobControl::onMouseMove(int x, int y, bool shiftDown)
{
    (void)x;
    if (!isDragging) return false;
    int dy = lastMouseY - y;
    lastMouseY = y;
    float range = maxVal - minVal;
    float step = range / (shiftDown ? 600.0f : 150.0f);
    currentVal = std::max(minVal, std::min(maxVal, currentVal + static_cast<float>(dy) * step));
    if (onValueChanged) onValueChanged(currentVal);
    return true;
}

void KnobControl::onMouseUp()
{
    isDragging = false;
}

bool KnobControl::onDoubleClick(int x, int y)
{
    if (bounds.Contains(static_cast<float>(x), static_cast<float>(y)))
    {
        currentVal = defaultVal;
        if (onValueChanged) onValueChanged(currentVal);
        return true;
    }
    return false;
}

void KnobControl::setValue(float val)
{
    currentVal = std::max(minVal, std::min(maxVal, val));
}

float KnobControl::getValue() const
{
    return currentVal;
}

SliderControl::SliderControl(const std::wstring& lbl, const std::wstring& u, float minV, float maxV, float defV, const Gdiplus::RectF& rect, bool horizontal, const Gdiplus::Color& accent)
{
    label = lbl;
    unit = u;
    minVal = minV;
    maxVal = maxV;
    defaultVal = defV;
    currentVal = defV;
    bounds = rect;
    isHorizontal = horizontal;
    accentColor = accent;
}

void SliderControl::draw(Gdiplus::Graphics& g)
{
    Gdiplus::FontFamily fontFam(L"Segoe UI");
    Gdiplus::Font font(&fontFam, 9.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::Font valFont(&fontFam, 9.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::StringFormat strFmt;
    strFmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    Gdiplus::SolidBrush lblBrush(UITheme::TextSecondary);
    Gdiplus::RectF lblRect(bounds.X, bounds.Y, bounds.Width * 0.45f, 14.0f);
    g.DrawString(label.c_str(), -1, &font, lblRect, &strFmt, &lblBrush);
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(1) << currentVal << L" " << unit;
    std::wstring valStr = ss.str();
    Gdiplus::SolidBrush valBrush(UITheme::TextPrimary);
    strFmt.SetAlignment(Gdiplus::StringAlignmentFar);
    Gdiplus::RectF valRect(bounds.X + bounds.Width * 0.45f, bounds.Y, bounds.Width * 0.55f, 14.0f);
    g.DrawString(valStr.c_str(), -1, &valFont, valRect, &strFmt, &valBrush);
    float trackY = bounds.Y + 18.0f;
    float trackH = 6.0f;
    Gdiplus::SolidBrush trackBg(UITheme::BgControl);
    UITheme::fillRoundedRect(g, trackBg, bounds.X, trackY, bounds.Width, trackH, 3.0f);
    float normVal = (currentVal - minVal) / (maxVal - minVal + 1e-6f);
    normVal = std::max(0.0f, std::min(1.0f, normVal));
    float fillW = bounds.Width * normVal;
    if (fillW > 3.0f)
    {
        Gdiplus::SolidBrush fillBrush(accentColor);
        UITheme::fillRoundedRect(g, fillBrush, bounds.X, trackY, fillW, trackH, 3.0f);
    }
    float thumbX = bounds.X + fillW;
    Gdiplus::SolidBrush thumbBrush(UITheme::TextPrimary);
    g.FillEllipse(&thumbBrush, thumbX - 5.0f, trackY - 2.0f, 10.0f, 10.0f);
}

bool SliderControl::onMouseDown(int x, int y)
{
    if (bounds.Contains(static_cast<float>(x), static_cast<float>(y)))
    {
        isDragging = true;
        onMouseMove(x, y);
        return true;
    }
    return false;
}

bool SliderControl::onMouseMove(int x, int y)
{
    (void)y;
    if (!isDragging) return false;
    float norm = (static_cast<float>(x) - bounds.X) / bounds.Width;
    norm = std::max(0.0f, std::min(1.0f, norm));
    currentVal = minVal + norm * (maxVal - minVal);
    if (onValueChanged) onValueChanged(currentVal);
    return true;
}

void SliderControl::onMouseUp()
{
    isDragging = false;
}

void SliderControl::setValue(float val)
{
    currentVal = std::max(minVal, std::min(maxVal, val));
}

float SliderControl::getValue() const
{
    return currentVal;
}

SwitchControl::SwitchControl(const std::wstring& lbl, bool defState, const Gdiplus::RectF& rect, const Gdiplus::Color& accent)
{
    label = lbl;
    state = defState;
    bounds = rect;
    accentColor = accent;
}

void SwitchControl::draw(Gdiplus::Graphics& g)
{
    float pillW = 34.0f;
    float pillH = 18.0f;
    float pillX = bounds.X;
    float pillY = bounds.Y + (bounds.Height - pillH) * 0.5f;
    Gdiplus::SolidBrush pillBg(state ? accentColor : UITheme::BgControl);
    UITheme::fillRoundedRect(g, pillBg, pillX, pillY, pillW, pillH, 8.0f);
    float thumbX = state ? (pillX + pillW - 16.0f) : (pillX + 2.0f);
    Gdiplus::SolidBrush thumbBrush(UITheme::TextPrimary);
    g.FillEllipse(&thumbBrush, thumbX, pillY + 2.0f, 14.0f, 14.0f);
    Gdiplus::FontFamily fontFam(L"Segoe UI");
    Gdiplus::Font font(&fontFam, 10.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::StringFormat strFmt;
    strFmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    Gdiplus::SolidBrush textBrush(state ? UITheme::TextPrimary : UITheme::TextSecondary);
    Gdiplus::RectF textRect(pillX + pillW + 8.0f, bounds.Y, bounds.Width - pillW - 8.0f, bounds.Height);
    g.DrawString(label.c_str(), -1, &font, textRect, &strFmt, &textBrush);
}

bool SwitchControl::onMouseDown(int x, int y)
{
    if (bounds.Contains(static_cast<float>(x), static_cast<float>(y)))
    {
        state = !state;
        if (onStateChanged) onStateChanged(state);
        return true;
    }
    return false;
}

void SwitchControl::setState(bool s)
{
    state = s;
}

bool SwitchControl::getState() const
{
    return state;
}

ButtonControl::ButtonControl(const std::wstring& lbl, const Gdiplus::RectF& rect, const Gdiplus::Color& accent, bool toggle)
{
    label = lbl;
    bounds = rect;
    accentColor = accent;
    isToggle = toggle;
}

void ButtonControl::draw(Gdiplus::Graphics& g)
{
    Gdiplus::Color bg = UITheme::BgPanelLight;
    if (isToggled) bg = accentColor;
    else if (isPressed) bg = UITheme::BgControl;
    else if (isHovered) bg = UITheme::BgControl;
    Gdiplus::SolidBrush fillBrush(bg);
    UITheme::fillRoundedRect(g, fillBrush, bounds.X, bounds.Y, bounds.Width, bounds.Height, 5.0f);
    Gdiplus::Pen borderPen((isHovered || isToggled) ? accentColor : UITheme::BorderDark, 1.2f);
    UITheme::drawRoundedRect(g, borderPen, bounds.X, bounds.Y, bounds.Width, bounds.Height, 5.0f);
    Gdiplus::FontFamily fontFam(L"Segoe UI");
    Gdiplus::Font font(&fontFam, 10.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::StringFormat strFmt;
    strFmt.SetAlignment(Gdiplus::StringAlignmentCenter);
    strFmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    Gdiplus::SolidBrush textBrush((isToggled) ? UITheme::BgDark : (isHovered ? UITheme::TextPrimary : UITheme::TextSecondary));
    g.DrawString(label.c_str(), -1, &font, bounds, &strFmt, &textBrush);
}

bool ButtonControl::onMouseDown(int x, int y)
{
    if (bounds.Contains(static_cast<float>(x), static_cast<float>(y)))
    {
        isPressed = true;
        return true;
    }
    return false;
}

void ButtonControl::onMouseMove(int x, int y)
{
    isHovered = bounds.Contains(static_cast<float>(x), static_cast<float>(y));
}

bool ButtonControl::onMouseUp(int x, int y)
{
    bool wasPressed = isPressed;
    isPressed = false;
    if (wasPressed && bounds.Contains(static_cast<float>(x), static_cast<float>(y)))
    {
        if (isToggle) isToggled = !isToggled;
        if (onClick) onClick();
        return true;
    }
    return false;
}

TabBarControl::TabBarControl(const std::vector<std::wstring>& tabNames, const Gdiplus::RectF& rect)
{
    tabs = tabNames;
    bounds = rect;
    selectedTab = 0;
}

void TabBarControl::draw(Gdiplus::Graphics& g)
{
    if (tabs.empty()) return;
    float tabW = bounds.Width / static_cast<float>(tabs.size());
    Gdiplus::FontFamily fontFam(L"Segoe UI");
    Gdiplus::Font font(&fontFam, 11.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::StringFormat strFmt;
    strFmt.SetAlignment(Gdiplus::StringAlignmentCenter);
    strFmt.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    for (size_t i = 0; i < tabs.size(); ++i)
    {
        float tx = bounds.X + static_cast<float>(i) * tabW;
        Gdiplus::RectF tabRect(tx, bounds.Y, tabW, bounds.Height);
        bool isSel = (static_cast<int>(i) == selectedTab);
        if (isSel)
        {
            Gdiplus::SolidBrush selBrush(UITheme::BgPanel);
            UITheme::fillRoundedRect(g, selBrush, tx + 2.0f, bounds.Y + 2.0f, tabW - 4.0f, bounds.Height - 2.0f, 6.0f);
            Gdiplus::SolidBrush barBrush(UITheme::CyanAccent);
            g.FillRectangle(&barBrush, tx + 12.0f, bounds.Y + bounds.Height - 3.0f, tabW - 24.0f, 3.0f);
        }
        Gdiplus::SolidBrush textBrush(isSel ? UITheme::CyanAccent : UITheme::TextMuted);
        g.DrawString(tabs[i].c_str(), -1, &font, tabRect, &strFmt, &textBrush);
    }
}

bool TabBarControl::onMouseDown(int x, int y)
{
    if (bounds.Contains(static_cast<float>(x), static_cast<float>(y)))
    {
        float tabW = bounds.Width / static_cast<float>(tabs.size());
        int idx = static_cast<int>((static_cast<float>(x) - bounds.X) / tabW);
        if (idx >= 0 && idx < static_cast<int>(tabs.size()))
        {
            selectedTab = idx;
            if (onTabChanged) onTabChanged(selectedTab);
            return true;
        }
    }
    return false;
}
