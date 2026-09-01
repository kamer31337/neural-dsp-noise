#pragma once

#include "gui_theme.h"
#include <string>
#include <vector>
#include <functional>

class KnobControl
{
public:
    std::wstring label;
    std::wstring unit;
    float minVal = 0.0f;
    float maxVal = 1.0f;
    float defaultVal = 0.5f;
    float currentVal = 0.5f;
    Gdiplus::RectF bounds;
    Gdiplus::Color accentColor = UITheme::CyanAccent;
    bool isDragging = false;
    int lastMouseY = 0;
    std::function<void(float)> onValueChanged;

    KnobControl() = default;
    KnobControl(const std::wstring& lbl, const std::wstring& u, float minV, float maxV, float defV, const Gdiplus::RectF& rect, const Gdiplus::Color& accent = UITheme::CyanAccent);

    void draw(Gdiplus::Graphics& g);
    bool onMouseDown(int x, int y);
    bool onMouseMove(int x, int y, bool shiftDown);
    void onMouseUp();
    bool onDoubleClick(int x, int y);
    void setValue(float val);
    float getValue() const;
};

class SliderControl
{
public:
    std::wstring label;
    std::wstring unit;
    float minVal = 0.0f;
    float maxVal = 1.0f;
    float defaultVal = 0.5f;
    float currentVal = 0.5f;
    Gdiplus::RectF bounds;
    Gdiplus::Color accentColor = UITheme::CyanAccent;
    bool isHorizontal = true;
    bool isDragging = false;
    std::function<void(float)> onValueChanged;

    SliderControl() = default;
    SliderControl(const std::wstring& lbl, const std::wstring& u, float minV, float maxV, float defV, const Gdiplus::RectF& rect, bool horizontal = true, const Gdiplus::Color& accent = UITheme::CyanAccent);

    void draw(Gdiplus::Graphics& g);
    bool onMouseDown(int x, int y);
    bool onMouseMove(int x, int y);
    void onMouseUp();
    void setValue(float val);
    float getValue() const;
};

class SwitchControl
{
public:
    std::wstring label;
    bool state = false;
    Gdiplus::RectF bounds;
    Gdiplus::Color accentColor = UITheme::EmeraldAccent;
    std::function<void(bool)> onStateChanged;

    SwitchControl() = default;
    SwitchControl(const std::wstring& lbl, bool defState, const Gdiplus::RectF& rect, const Gdiplus::Color& accent = UITheme::EmeraldAccent);

    void draw(Gdiplus::Graphics& g);
    bool onMouseDown(int x, int y);
    void setState(bool s);
    bool getState() const;
};

class ButtonControl
{
public:
    std::wstring label;
    Gdiplus::RectF bounds;
    Gdiplus::Color accentColor = UITheme::CyanAccent;
    bool isHovered = false;
    bool isPressed = false;
    bool isToggle = false;
    bool isToggled = false;
    std::function<void()> onClick;

    ButtonControl() = default;
    ButtonControl(const std::wstring& lbl, const Gdiplus::RectF& rect, const Gdiplus::Color& accent = UITheme::CyanAccent, bool toggle = false);

    void draw(Gdiplus::Graphics& g);
    bool onMouseDown(int x, int y);
    void onMouseMove(int x, int y);
    bool onMouseUp(int x, int y);
};

class TabBarControl
{
public:
    std::vector<std::wstring> tabs;
    int selectedTab = 0;
    Gdiplus::RectF bounds;
    std::function<void(int)> onTabChanged;

    TabBarControl() = default;
    TabBarControl(const std::vector<std::wstring>& tabNames, const Gdiplus::RectF& rect);

    void draw(Gdiplus::Graphics& g);
    bool onMouseDown(int x, int y);
};
