#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <vector>
#include <memory>
#include "gui_theme.h"
#include "gui_controls.h"
#include "visualizers.h"
#include "../dsp/dsp_pipeline.h"
#include "../audio/audio_player.h"

class MainWindow
{
public:
    MainWindow(HINSTANCE hInstance, DSPPipeline& dsp, AudioPlayer& player);
    ~MainWindow();

    bool create(int width = 1180, int height = 820, const wchar_t* title = L"Neural DSP - Quantized Noise Reduction & Sound Enhancer");
    void show(int nCmdShow);
    HWND getHwnd() const;

private:
    HINSTANCE hInst;
    HWND hWnd = nullptr;
    DSPPipeline& dspEngine;
    AudioPlayer& audioPlayer;

    int clientWidth = 1180;
    int clientHeight = 820;
    int currentTab = 0;

    TabBarControl tabControl;
    SpectrumVisualizer spectrumVis;
    NeuralMaskVisualizer neuralMaskVis;
    OscilloscopeVisualizer oscVis;
    VUMeterControl inMeter;
    VUMeterControl outMeter;

    KnobControl knobDenoiseStrength;
    KnobControl knobSpectralFloor;
    KnobControl knobMaskSmoothing;
    KnobControl knobTransientPreserve;
    KnobControl knobGateThreshold;
    SwitchControl switchDenoiseEnable;
    ButtonControl btnLearnNoise;
    ButtonControl btnClearNoise;

    KnobControl knobBassDrive;
    KnobControl knobBassFreq;
    KnobControl knobAirDrive;
    KnobControl knobAirFreq;
    KnobControl knobTransientAttack;
    KnobControl knobWarmth;
    KnobControl knobClarity;
    SwitchControl switchEnhanceEnable;

    KnobControl knobInputGain;
    KnobControl knobOutputGain;
    KnobControl knobWetDry;
    KnobControl knobLimiterCeiling;
    SwitchControl switchBypass;

    ButtonControl btnLoadWav;
    ButtonControl btnProcessWav;
    ButtonControl btnPlay;
    ButtonControl btnPause;
    ButtonControl btnStop;
    ButtonControl btnLoop;
    ButtonControl btnSrcVoice;
    ButtonControl btnSrcPink;
    ButtonControl btnSrcWhite;
    ButtonControl btnSrcHum;
    ButtonControl btnSrcSweep;
    SliderControl sliderScrub;

    std::wstring loadedWavPath;
    std::wstring statusMessage;
    int statusTimer = 0;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void initControls();
    void layoutControls();
    void updateDSPFromControls();
    void render(HDC hdc);
    void renderTabDSP(Gdiplus::Graphics& g);
    void renderTabNeural(Gdiplus::Graphics& g);
    void renderTabAudio(Gdiplus::Graphics& g);
    void onOpenFile();
    void onProcessFile();
    void setStatus(const std::wstring& msg);
};
