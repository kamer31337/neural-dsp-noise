#include "gui_window.h"
#include <commdlg.h>
#include <sstream>
#include <iomanip>

MainWindow::MainWindow(HINSTANCE hInstance, DSPPipeline& dsp, AudioPlayer& player) : hInst(hInstance), dspEngine(dsp), audioPlayer(player)
{
    initControls();
}

MainWindow::~MainWindow()
{
    if (hWnd) DestroyWindow(hWnd);
}

void MainWindow::initControls()
{
    tabControl = TabBarControl({L"DSP & NOISE REDUCTION", L"QUANTIZED NEURAL INSPECTOR", L"AUDIO & TEST STUDIO"}, Gdiplus::RectF(20.0f, 60.0f, 1140.0f, 36.0f));
    tabControl.onTabChanged = [this](int tab) { currentTab = tab; };
    knobDenoiseStrength = KnobControl(L"INT8 Mask", L"x", 0.0f, 2.0f, 1.0f, Gdiplus::RectF(), UITheme::CyanAccent);
    knobSpectralFloor = KnobControl(L"Floor", L"dB", -60.0f, -6.0f, -26.0f, Gdiplus::RectF(), UITheme::CyanAccent);
    knobMaskSmoothing = KnobControl(L"Smoothing", L"%", 0.0f, 100.0f, 65.0f, Gdiplus::RectF(), UITheme::CyanAccent);
    knobTransientPreserve = KnobControl(L"Transient", L"%", 0.0f, 100.0f, 40.0f, Gdiplus::RectF(), UITheme::CyanAccent);
    knobGateThreshold = KnobControl(L"Gate Thresh", L"dB", -60.0f, 0.0f, -42.0f, Gdiplus::RectF(), UITheme::CyanAccent);
    switchDenoiseEnable = SwitchControl(L"Denoise Active", true, Gdiplus::RectF(), UITheme::CyanAccent);
    btnLearnNoise = ButtonControl(L"Learn Profile", Gdiplus::RectF(), UITheme::AmberAccent);
    btnClearNoise = ButtonControl(L"Clear Profile", Gdiplus::RectF(), UITheme::TextSecondary);
    knobBassDrive = KnobControl(L"Bass Exciter", L"%", 0.0f, 100.0f, 40.0f, Gdiplus::RectF(), UITheme::EmeraldAccent);
    knobBassFreq = KnobControl(L"Bass Freq", L"Hz", 40.0f, 200.0f, 90.0f, Gdiplus::RectF(), UITheme::EmeraldAccent);
    knobAirDrive = KnobControl(L"Air Sheen", L"%", 0.0f, 100.0f, 50.0f, Gdiplus::RectF(), UITheme::EmeraldAccent);
    knobAirFreq = KnobControl(L"Air Freq", L"Hz", 4000.0f, 16000.0f, 8000.0f, Gdiplus::RectF(), UITheme::EmeraldAccent);
    knobTransientAttack = KnobControl(L"Attack Punch", L"%", -100.0f, 100.0f, 30.0f, Gdiplus::RectF(), UITheme::EmeraldAccent);
    knobWarmth = KnobControl(L"Warmth", L"%", 0.0f, 100.0f, 25.0f, Gdiplus::RectF(), UITheme::EmeraldAccent);
    knobClarity = KnobControl(L"Clarity", L"%", 0.0f, 100.0f, 35.0f, Gdiplus::RectF(), UITheme::EmeraldAccent);
    switchEnhanceEnable = SwitchControl(L"Enhancer Active", true, Gdiplus::RectF(), UITheme::EmeraldAccent);
    knobInputGain = KnobControl(L"In Gain", L"dB", -24.0f, 24.0f, 0.0f, Gdiplus::RectF(), UITheme::PurpleAccent);
    knobOutputGain = KnobControl(L"Out Gain", L"dB", -24.0f, 24.0f, 0.0f, Gdiplus::RectF(), UITheme::PurpleAccent);
    knobWetDry = KnobControl(L"Mix", L"%", 0.0f, 100.0f, 100.0f, Gdiplus::RectF(), UITheme::PurpleAccent);
    knobLimiterCeiling = KnobControl(L"Ceiling", L"dB", -12.0f, 0.0f, -0.3f, Gdiplus::RectF(), UITheme::PurpleAccent);
    switchBypass = SwitchControl(L"DSP BYPASS", false, Gdiplus::RectF(), UITheme::CoralAccent);
    btnLoadWav = ButtonControl(L"Open WAV File", Gdiplus::RectF(), UITheme::CyanAccent);
    btnProcessWav = ButtonControl(L"Clean & Export WAV", Gdiplus::RectF(), UITheme::EmeraldAccent);
    btnPlay = ButtonControl(L"Play", Gdiplus::RectF(), UITheme::EmeraldAccent);
    btnPause = ButtonControl(L"Pause", Gdiplus::RectF(), UITheme::AmberAccent);
    btnStop = ButtonControl(L"Stop", Gdiplus::RectF(), UITheme::CoralAccent);
    btnLoop = ButtonControl(L"Loop: ON", Gdiplus::RectF(), UITheme::CyanAccent, true);
    btnLoop.isToggled = true;
    btnSrcVoice = ButtonControl(L"Simulated Speech + Noise", Gdiplus::RectF(), UITheme::CyanAccent, true);
    btnSrcVoice.isToggled = true;
    btnSrcPink = ButtonControl(L"Pink Noise Test", Gdiplus::RectF(), UITheme::AmberAccent, true);
    btnSrcWhite = ButtonControl(L"White Noise Test", Gdiplus::RectF(), UITheme::AmberAccent, true);
    btnSrcHum = ButtonControl(L"60Hz AC Hum Test", Gdiplus::RectF(), UITheme::AmberAccent, true);
    btnSrcSweep = ButtonControl(L"Sine Sweep 20Hz-20kHz", Gdiplus::RectF(), UITheme::PurpleAccent, true);
    sliderScrub = SliderControl(L"Timeline Position", L"%", 0.0f, 100.0f, 0.0f, Gdiplus::RectF(), true, UITheme::CyanAccent);
    auto bindKnob = [this](KnobControl& k) { k.onValueChanged = [this](float) { updateDSPFromControls(); }; };
    bindKnob(knobDenoiseStrength);
    bindKnob(knobSpectralFloor);
    bindKnob(knobMaskSmoothing);
    bindKnob(knobTransientPreserve);
    bindKnob(knobGateThreshold);
    bindKnob(knobBassDrive);
    bindKnob(knobBassFreq);
    bindKnob(knobAirDrive);
    bindKnob(knobAirFreq);
    bindKnob(knobTransientAttack);
    bindKnob(knobWarmth);
    bindKnob(knobClarity);
    bindKnob(knobInputGain);
    bindKnob(knobOutputGain);
    bindKnob(knobWetDry);
    bindKnob(knobLimiterCeiling);
    switchDenoiseEnable.onStateChanged = [this](bool) { updateDSPFromControls(); };
    switchEnhanceEnable.onStateChanged = [this](bool) { updateDSPFromControls(); };
    switchBypass.onStateChanged = [this](bool) { updateDSPFromControls(); };
    btnLearnNoise.onClick = [this]()
    {
        dspEngine.startNoiseLearning();
        setStatus(L"Learning background noise profile...");
    };
    btnClearNoise.onClick = [this]()
    {
        dspEngine.clearNoiseProfile();
        setStatus(L"Noise profile reset.");
    };
    btnLoadWav.onClick = [this]() { onOpenFile(); };
    btnProcessWav.onClick = [this]() { onProcessFile(); };
    btnPlay.onClick = [this]() { audioPlayer.play(); setStatus(L"Audio Playback Active."); };
    btnPause.onClick = [this]() { audioPlayer.pause(); setStatus(L"Audio Paused."); };
    btnStop.onClick = [this]() { audioPlayer.stop(); setStatus(L"Audio Stopped."); };
    btnLoop.onClick = [this]() { audioPlayer.setLoop(btnLoop.isToggled); };
    auto clearSourceToggles = [this]()
    {
        btnSrcVoice.isToggled = false;
        btnSrcPink.isToggled = false;
        btnSrcWhite.isToggled = false;
        btnSrcHum.isToggled = false;
        btnSrcSweep.isToggled = false;
    };
    btnSrcVoice.onClick = [this, clearSourceToggles]() { clearSourceToggles(); btnSrcVoice.isToggled = true; audioPlayer.setSignalSource(SignalSource::TestVoiceWithNoise); setStatus(L"Source: Simulated Speech + Noise"); };
    btnSrcPink.onClick = [this, clearSourceToggles]() { clearSourceToggles(); btnSrcPink.isToggled = true; audioPlayer.setSignalSource(SignalSource::TestPinkNoise); setStatus(L"Source: Pink Noise"); };
    btnSrcWhite.onClick = [this, clearSourceToggles]() { clearSourceToggles(); btnSrcWhite.isToggled = true; audioPlayer.setSignalSource(SignalSource::TestWhiteNoise); setStatus(L"Source: White Noise"); };
    btnSrcHum.onClick = [this, clearSourceToggles]() { clearSourceToggles(); btnSrcHum.isToggled = true; audioPlayer.setSignalSource(SignalSource::TestHum60Hz); setStatus(L"Source: 60Hz Hum"); };
    btnSrcSweep.onClick = [this, clearSourceToggles]() { clearSourceToggles(); btnSrcSweep.isToggled = true; audioPlayer.setSignalSource(SignalSource::TestSineSweep); setStatus(L"Source: Sine Sweep"); };
    sliderScrub.onValueChanged = [this](float val) { audioPlayer.seekTo(val / 100.0f); };
    layoutControls();
    updateDSPFromControls();
}

void MainWindow::layoutControls()
{
    tabControl.bounds = Gdiplus::RectF(20.0f, 60.0f, static_cast<float>(clientWidth - 40), 34.0f);
    spectrumVis.bounds = Gdiplus::RectF(20.0f, 105.0f, static_cast<float>(clientWidth - 40), 220.0f);
    neuralMaskVis.bounds = Gdiplus::RectF(20.0f, 335.0f, static_cast<float>(clientWidth - 40), 160.0f);
    oscVis.bounds = Gdiplus::RectF(20.0f, 505.0f, static_cast<float>(clientWidth - 40), 120.0f);
    inMeter.bounds = Gdiplus::RectF(static_cast<float>(clientWidth - 200), 18.0f, 80.0f, 18.0f);
    outMeter.bounds = Gdiplus::RectF(static_cast<float>(clientWidth - 110), 18.0f, 80.0f, 18.0f);
    float panY = 340.0f;
    float colW = (static_cast<float>(clientWidth) - 80.0f) / 3.0f;
    float col1X = 20.0f;
    switchDenoiseEnable.bounds = Gdiplus::RectF(col1X + 16.0f, panY + 40.0f, 140.0f, 22.0f);
    btnLearnNoise.bounds = Gdiplus::RectF(col1X + 160.0f, panY + 38.0f, 95.0f, 24.0f);
    btnClearNoise.bounds = Gdiplus::RectF(col1X + 260.0f, panY + 38.0f, 85.0f, 24.0f);
    knobDenoiseStrength.bounds = Gdiplus::RectF(col1X + 15.0f, panY + 75.0f, 75.0f, 80.0f);
    knobSpectralFloor.bounds = Gdiplus::RectF(col1X + 100.0f, panY + 75.0f, 75.0f, 80.0f);
    knobMaskSmoothing.bounds = Gdiplus::RectF(col1X + 185.0f, panY + 75.0f, 75.0f, 80.0f);
    knobTransientPreserve.bounds = Gdiplus::RectF(col1X + 270.0f, panY + 75.0f, 75.0f, 80.0f);
    knobGateThreshold.bounds = Gdiplus::RectF(col1X + 15.0f, panY + 165.0f, 75.0f, 80.0f);
    float col2X = col1X + colW + 20.0f;
    switchEnhanceEnable.bounds = Gdiplus::RectF(col2X + 16.0f, panY + 40.0f, 160.0f, 22.0f);
    knobBassDrive.bounds = Gdiplus::RectF(col2X + 15.0f, panY + 75.0f, 75.0f, 80.0f);
    knobBassFreq.bounds = Gdiplus::RectF(col2X + 100.0f, panY + 75.0f, 75.0f, 80.0f);
    knobAirDrive.bounds = Gdiplus::RectF(col2X + 185.0f, panY + 75.0f, 75.0f, 80.0f);
    knobAirFreq.bounds = Gdiplus::RectF(col2X + 270.0f, panY + 75.0f, 75.0f, 80.0f);
    knobTransientAttack.bounds = Gdiplus::RectF(col2X + 15.0f, panY + 165.0f, 75.0f, 80.0f);
    knobWarmth.bounds = Gdiplus::RectF(col2X + 100.0f, panY + 165.0f, 75.0f, 80.0f);
    knobClarity.bounds = Gdiplus::RectF(col2X + 185.0f, panY + 165.0f, 75.0f, 80.0f);
    float col3X = col2X + colW + 20.0f;
    switchBypass.bounds = Gdiplus::RectF(col3X + 16.0f, panY + 40.0f, 140.0f, 22.0f);
    knobInputGain.bounds = Gdiplus::RectF(col3X + 15.0f, panY + 75.0f, 75.0f, 80.0f);
    knobOutputGain.bounds = Gdiplus::RectF(col3X + 100.0f, panY + 75.0f, 75.0f, 80.0f);
    knobWetDry.bounds = Gdiplus::RectF(col3X + 185.0f, panY + 75.0f, 75.0f, 80.0f);
    knobLimiterCeiling.bounds = Gdiplus::RectF(col3X + 270.0f, panY + 75.0f, 75.0f, 80.0f);
    float audY = 110.0f;
    btnLoadWav.bounds = Gdiplus::RectF(40.0f, audY + 50.0f, 150.0f, 32.0f);
    btnProcessWav.bounds = Gdiplus::RectF(205.0f, audY + 50.0f, 170.0f, 32.0f);
    sliderScrub.bounds = Gdiplus::RectF(40.0f, audY + 100.0f, static_cast<float>(clientWidth - 80), 30.0f);
    btnPlay.bounds = Gdiplus::RectF(40.0f, audY + 150.0f, 80.0f, 32.0f);
    btnPause.bounds = Gdiplus::RectF(130.0f, audY + 150.0f, 80.0f, 32.0f);
    btnStop.bounds = Gdiplus::RectF(220.0f, audY + 150.0f, 80.0f, 32.0f);
    btnLoop.bounds = Gdiplus::RectF(310.0f, audY + 150.0f, 90.0f, 32.0f);
    float sigY = audY + 220.0f;
    btnSrcVoice.bounds = Gdiplus::RectF(40.0f, sigY + 45.0f, 210.0f, 32.0f);
    btnSrcPink.bounds = Gdiplus::RectF(260.0f, sigY + 45.0f, 150.0f, 32.0f);
    btnSrcWhite.bounds = Gdiplus::RectF(420.0f, sigY + 45.0f, 150.0f, 32.0f);
    btnSrcHum.bounds = Gdiplus::RectF(580.0f, sigY + 45.0f, 150.0f, 32.0f);
    btnSrcSweep.bounds = Gdiplus::RectF(740.0f, sigY + 45.0f, 190.0f, 32.0f);
}

void MainWindow::updateDSPFromControls()
{
    MasterDSPParams p;
    p.inputGainDb = knobInputGain.getValue();
    p.outputGainDb = knobOutputGain.getValue();
    p.wetDryMix = knobWetDry.getValue() / 100.0f;
    p.bypass = switchBypass.getState();
    p.denoiser.suppressionStrength = knobDenoiseStrength.getValue();
    p.denoiser.spectralFloor = std::pow(10.0f, knobSpectralFloor.getValue() / 20.0f);
    p.denoiser.maskSmoothing = knobMaskSmoothing.getValue() / 100.0f;
    p.denoiser.transientPreserve = knobTransientPreserve.getValue() / 100.0f;
    p.denoiser.enabled = switchDenoiseEnable.getState();
    p.gate.thresholdDb = knobGateThreshold.getValue();
    p.gate.enabled = switchDenoiseEnable.getState();
    p.enhancer.bassDrive = knobBassDrive.getValue() / 100.0f;
    p.enhancer.bassFreqHz = knobBassFreq.getValue();
    p.enhancer.airDrive = knobAirDrive.getValue() / 100.0f;
    p.enhancer.airFreqHz = knobAirFreq.getValue();
    p.enhancer.transientAttack = knobTransientAttack.getValue() / 100.0f;
    p.enhancer.warmthDrive = knobWarmth.getValue() / 100.0f;
    p.enhancer.clarityBoost = knobClarity.getValue() / 100.0f;
    p.enhancer.enabled = switchEnhanceEnable.getState();
    p.limiter.ceilingDb = knobLimiterCeiling.getValue();
    p.limiter.enabled = true;
    dspEngine.setParams(p);
}

void MainWindow::setStatus(const std::wstring& msg)
{
    statusMessage = msg;
    statusTimer = 180;
}

void MainWindow::onOpenFile()
{
    wchar_t filename[MAX_PATH] = L"";
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"WAV Audio Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameW(&ofn))
    {
        if (audioPlayer.loadWav(filename))
        {
            loadedWavPath = filename;
            setStatus(L"Loaded WAV: " + loadedWavPath);
            btnSrcVoice.isToggled = false;
            btnSrcPink.isToggled = false;
            btnSrcWhite.isToggled = false;
            btnSrcHum.isToggled = false;
            btnSrcSweep.isToggled = false;
        }
        else
        {
            setStatus(L"Failed to load WAV file.");
        }
    }
}

void MainWindow::onProcessFile()
{
    if (loadedWavPath.empty())
    {
        setStatus(L"Please load an input WAV file first.");
        return;
    }
    wchar_t filename[MAX_PATH] = L"cleaned_output.wav";
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"WAV Audio Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT;
    if (GetSaveFileNameW(&ofn))
    {
        setStatus(L"Processing & Exporting cleaned WAV...");
        bool ok = audioPlayer.processAndSaveWav(loadedWavPath, filename);
        if (ok) setStatus(L"Cleaned WAV successfully exported!");
        else setStatus(L"Export failed.");
    }
}

bool MainWindow::create(int width, int height, const wchar_t* title)
{
    clientWidth = width;
    clientHeight = height;
    if (!hInst) hInst = GetModuleHandleW(nullptr);
    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = MainWindow::WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = L"NeuralDSPNoiseReductionWndClass";
    RegisterClassExW(&wc);
    RECT wr = {0, 0, clientWidth, clientHeight};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    int winW = wr.right - wr.left;
    int winH = wr.bottom - wr.top;
    hWnd = CreateWindowExW(WS_EX_APPWINDOW, L"NeuralDSPNoiseReductionWndClass", title, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, winW, winH, nullptr, nullptr, hInst, this);
    if (!hWnd) return false;
    SetTimer(hWnd, 1, 16, nullptr);
    audioPlayer.init(48000);
    audioPlayer.play();
    return true;
}

void MainWindow::show(int nCmdShow)
{
    if (nCmdShow <= 0 || nCmdShow == SW_HIDE) nCmdShow = SW_SHOWNORMAL;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    SetForegroundWindow(hWnd);
}

HWND MainWindow::getHwnd() const
{
    return hWnd;
}

LRESULT CALLBACK MainWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MainWindow* pWnd = nullptr;
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pWnd = reinterpret_cast<MainWindow*>(pCreate->lpCreateParams);
        if (pWnd)
        {
            pWnd->hWnd = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
        }
    }
    else
    {
        pWnd = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
    if (pWnd) return pWnd->handleMessage(hwnd, msg, wParam, lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT MainWindow::handleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_TIMER:
    {
        if (statusTimer > 0) statusTimer--;
        if (!sliderScrub.isDragging && audioPlayer.getSignalSource() == SignalSource::WavPlayback)
        {
            sliderScrub.setValue(audioPlayer.getPlaybackProgress() * 100.0f);
        }
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
    }
    case WM_SIZE:
    {
        clientWidth = LOWORD(lParam);
        clientHeight = HIWORD(lParam);
        layoutControls();
        InvalidateRect(hWnd, nullptr, FALSE);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        SetCapture(hWnd);
        if (tabControl.onMouseDown(x, y)) return 0;
        if (currentTab == 0)
        {
            if (switchDenoiseEnable.onMouseDown(x, y)) return 0;
            if (btnLearnNoise.onMouseDown(x, y)) return 0;
            if (btnClearNoise.onMouseDown(x, y)) return 0;
            if (knobDenoiseStrength.onMouseDown(x, y)) return 0;
            if (knobSpectralFloor.onMouseDown(x, y)) return 0;
            if (knobMaskSmoothing.onMouseDown(x, y)) return 0;
            if (knobTransientPreserve.onMouseDown(x, y)) return 0;
            if (knobGateThreshold.onMouseDown(x, y)) return 0;
            if (switchEnhanceEnable.onMouseDown(x, y)) return 0;
            if (knobBassDrive.onMouseDown(x, y)) return 0;
            if (knobBassFreq.onMouseDown(x, y)) return 0;
            if (knobAirDrive.onMouseDown(x, y)) return 0;
            if (knobAirFreq.onMouseDown(x, y)) return 0;
            if (knobTransientAttack.onMouseDown(x, y)) return 0;
            if (knobWarmth.onMouseDown(x, y)) return 0;
            if (knobClarity.onMouseDown(x, y)) return 0;
            if (switchBypass.onMouseDown(x, y)) return 0;
            if (knobInputGain.onMouseDown(x, y)) return 0;
            if (knobOutputGain.onMouseDown(x, y)) return 0;
            if (knobWetDry.onMouseDown(x, y)) return 0;
            if (knobLimiterCeiling.onMouseDown(x, y)) return 0;
        }
        else if (currentTab == 2)
        {
            if (btnLoadWav.onMouseDown(x, y)) return 0;
            if (btnProcessWav.onMouseDown(x, y)) return 0;
            if (btnPlay.onMouseDown(x, y)) return 0;
            if (btnPause.onMouseDown(x, y)) return 0;
            if (btnStop.onMouseDown(x, y)) return 0;
            if (btnLoop.onMouseDown(x, y)) return 0;
            if (btnSrcVoice.onMouseDown(x, y)) return 0;
            if (btnSrcPink.onMouseDown(x, y)) return 0;
            if (btnSrcWhite.onMouseDown(x, y)) return 0;
            if (btnSrcHum.onMouseDown(x, y)) return 0;
            if (btnSrcSweep.onMouseDown(x, y)) return 0;
            if (sliderScrub.onMouseDown(x, y)) return 0;
        }
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        bool shiftDown = (wParam & MK_SHIFT) != 0;
        if (currentTab == 0)
        {
            knobDenoiseStrength.onMouseMove(x, y, shiftDown);
            knobSpectralFloor.onMouseMove(x, y, shiftDown);
            knobMaskSmoothing.onMouseMove(x, y, shiftDown);
            knobTransientPreserve.onMouseMove(x, y, shiftDown);
            knobGateThreshold.onMouseMove(x, y, shiftDown);
            knobBassDrive.onMouseMove(x, y, shiftDown);
            knobBassFreq.onMouseMove(x, y, shiftDown);
            knobAirDrive.onMouseMove(x, y, shiftDown);
            knobAirFreq.onMouseMove(x, y, shiftDown);
            knobTransientAttack.onMouseMove(x, y, shiftDown);
            knobWarmth.onMouseMove(x, y, shiftDown);
            knobClarity.onMouseMove(x, y, shiftDown);
            knobInputGain.onMouseMove(x, y, shiftDown);
            knobOutputGain.onMouseMove(x, y, shiftDown);
            knobWetDry.onMouseMove(x, y, shiftDown);
            knobLimiterCeiling.onMouseMove(x, y, shiftDown);
            btnLearnNoise.onMouseMove(x, y);
            btnClearNoise.onMouseMove(x, y);
        }
        else if (currentTab == 2)
        {
            btnLoadWav.onMouseMove(x, y);
            btnProcessWav.onMouseMove(x, y);
            btnPlay.onMouseMove(x, y);
            btnPause.onMouseMove(x, y);
            btnStop.onMouseMove(x, y);
            btnLoop.onMouseMove(x, y);
            btnSrcVoice.onMouseMove(x, y);
            btnSrcPink.onMouseMove(x, y);
            btnSrcWhite.onMouseMove(x, y);
            btnSrcHum.onMouseMove(x, y);
            btnSrcSweep.onMouseMove(x, y);
            sliderScrub.onMouseMove(x, y);
        }
        return 0;
    }
    case WM_LBUTTONUP:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        ReleaseCapture();
        knobDenoiseStrength.onMouseUp();
        knobSpectralFloor.onMouseUp();
        knobMaskSmoothing.onMouseUp();
        knobTransientPreserve.onMouseUp();
        knobGateThreshold.onMouseUp();
        knobBassDrive.onMouseUp();
        knobBassFreq.onMouseUp();
        knobAirDrive.onMouseUp();
        knobAirFreq.onMouseUp();
        knobTransientAttack.onMouseUp();
        knobWarmth.onMouseUp();
        knobClarity.onMouseUp();
        knobInputGain.onMouseUp();
        knobOutputGain.onMouseUp();
        knobWetDry.onMouseUp();
        knobLimiterCeiling.onMouseUp();
        sliderScrub.onMouseUp();
        if (currentTab == 0)
        {
            btnLearnNoise.onMouseUp(x, y);
            btnClearNoise.onMouseUp(x, y);
        }
        else if (currentTab == 2)
        {
            btnLoadWav.onMouseUp(x, y);
            btnProcessWav.onMouseUp(x, y);
            btnPlay.onMouseUp(x, y);
            btnPause.onMouseUp(x, y);
            btnStop.onMouseUp(x, y);
            btnLoop.onMouseUp(x, y);
            btnSrcVoice.onMouseUp(x, y);
            btnSrcPink.onMouseUp(x, y);
            btnSrcWhite.onMouseUp(x, y);
            btnSrcHum.onMouseUp(x, y);
            btnSrcSweep.onMouseUp(x, y);
        }
        return 0;
    }
    case WM_LBUTTONDBLCLK:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        if (currentTab == 0)
        {
            knobDenoiseStrength.onDoubleClick(x, y);
            knobSpectralFloor.onDoubleClick(x, y);
            knobMaskSmoothing.onDoubleClick(x, y);
            knobTransientPreserve.onDoubleClick(x, y);
            knobGateThreshold.onDoubleClick(x, y);
            knobBassDrive.onDoubleClick(x, y);
            knobBassFreq.onDoubleClick(x, y);
            knobAirDrive.onDoubleClick(x, y);
            knobAirFreq.onDoubleClick(x, y);
            knobTransientAttack.onDoubleClick(x, y);
            knobWarmth.onDoubleClick(x, y);
            knobClarity.onDoubleClick(x, y);
            knobInputGain.onDoubleClick(x, y);
            knobOutputGain.onDoubleClick(x, y);
            knobWetDry.onDoubleClick(x, y);
            knobLimiterCeiling.onDoubleClick(x, y);
        }
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        render(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
    {
        audioPlayer.shutdown();
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void MainWindow::render(HDC hdc)
{
    if (clientWidth <= 0 || clientHeight <= 0) return;
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, clientWidth, clientHeight);
    HGDIOBJ oldBmp = SelectObject(memDC, memBmp);
    Gdiplus::Graphics g(memDC);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
    Gdiplus::SolidBrush bgBrush(UITheme::BgDark);
    g.FillRectangle(&bgBrush, 0, 0, clientWidth, clientHeight);
    Gdiplus::FontFamily fontFam(L"Segoe UI");
    Gdiplus::Font titleFont(&fontFam, 14.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::Font subFont(&fontFam, 9.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush titleBrush(UITheme::TextPrimary);
    Gdiplus::SolidBrush subBrush(UITheme::CyanAccent);
    g.DrawString(L"NEURAL DSP // ZERO-DEPENDENCY QUANTIZED NOISE REDUCTION & ENHANCER", -1, &titleFont, Gdiplus::PointF(20.0f, 16.0f), &titleBrush);
    g.DrawString(L"INT8 QUANTIZED INFERENCE • STFT MASKING • HARMONIC EXCITER • LOOKAHEAD LIMITER", -1, &subFont, Gdiplus::PointF(20.0f, 36.0f), &subBrush);
    MeteringData meters;
    dspEngine.getMeteringData(meters);
    inMeter.draw(g, meters.inputPeak, meters.inputRms);
    outMeter.draw(g, meters.outputPeak, meters.outputRms, meters.limiterGainReductionDb);
    tabControl.draw(g);
    if (currentTab == 0) renderTabDSP(g);
    else if (currentTab == 1) renderTabNeural(g);
    else if (currentTab == 2) renderTabAudio(g);
    if (!statusMessage.empty() && statusTimer > 0)
    {
        Gdiplus::SolidBrush statusBg(Gdiplus::Color(220, 26, 30, 39));
        UITheme::fillRoundedRect(g, statusBg, 20.0f, static_cast<float>(clientHeight - 34), static_cast<float>(clientWidth - 40), 24.0f, 4.0f);
        Gdiplus::Font statusFont(&fontFam, 9.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::SolidBrush statusBrush(UITheme::EmeraldAccent);
        g.DrawString(statusMessage.c_str(), -1, &statusFont, Gdiplus::PointF(30.0f, static_cast<float>(clientHeight - 29)), &statusBrush);
    }
    BitBlt(hdc, 0, 0, clientWidth, clientHeight, memDC, 0, 0, SRCCOPY);
    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
}

void MainWindow::renderTabDSP(Gdiplus::Graphics& g)
{
    MeteringData meters;
    dspEngine.getMeteringData(meters);
    spectrumVis.draw(g, meters.inSpectrum, meters.outSpectrum, dspEngine.getSampleRate());
    float panY = 335.0f;
    float panH = static_cast<float>(clientHeight) - panY - 45.0f;
    float colW = (static_cast<float>(clientWidth) - 80.0f) / 3.0f;
    float col1X = 20.0f;
    Gdiplus::SolidBrush panelBg(UITheme::BgPanel);
    Gdiplus::Pen panelPen(UITheme::BorderDark, 1.0f);
    UITheme::fillRoundedRect(g, panelBg, col1X, panY, colW, panH, 8.0f);
    UITheme::drawRoundedRect(g, panelPen, col1X, panY, colW, panH, 8.0f);
    Gdiplus::FontFamily fontFam(L"Segoe UI");
    Gdiplus::Font headerFont(&fontFam, 11.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush cyanHdr(UITheme::CyanAccent);
    g.DrawString(L"1. QUANTIZED NOISE REDUCTION", -1, &headerFont, Gdiplus::PointF(col1X + 16.0f, panY + 14.0f), &cyanHdr);
    switchDenoiseEnable.draw(g);
    btnLearnNoise.draw(g);
    btnClearNoise.draw(g);
    knobDenoiseStrength.draw(g);
    knobSpectralFloor.draw(g);
    knobMaskSmoothing.draw(g);
    knobTransientPreserve.draw(g);
    knobGateThreshold.draw(g);
    float col2X = col1X + colW + 20.0f;
    UITheme::fillRoundedRect(g, panelBg, col2X, panY, colW, panH, 8.0f);
    UITheme::drawRoundedRect(g, panelPen, col2X, panY, colW, panH, 8.0f);
    Gdiplus::SolidBrush emeraldHdr(UITheme::EmeraldAccent);
    g.DrawString(L"2. SOUND ENHANCER & EXCITER", -1, &headerFont, Gdiplus::PointF(col2X + 16.0f, panY + 14.0f), &emeraldHdr);
    switchEnhanceEnable.draw(g);
    knobBassDrive.draw(g);
    knobBassFreq.draw(g);
    knobAirDrive.draw(g);
    knobAirFreq.draw(g);
    knobTransientAttack.draw(g);
    knobWarmth.draw(g);
    knobClarity.draw(g);
    float col3X = col2X + colW + 20.0f;
    UITheme::fillRoundedRect(g, panelBg, col3X, panY, colW, panH, 8.0f);
    UITheme::drawRoundedRect(g, panelPen, col3X, panY, colW, panH, 8.0f);
    Gdiplus::SolidBrush purpleHdr(UITheme::PurpleAccent);
    g.DrawString(L"3. MASTER & LOOKAHEAD LIMITER", -1, &headerFont, Gdiplus::PointF(col3X + 16.0f, panY + 14.0f), &purpleHdr);
    switchBypass.draw(g);
    knobInputGain.draw(g);
    knobOutputGain.draw(g);
    knobWetDry.draw(g);
    knobLimiterCeiling.draw(g);
    float miniOscY = panY + 175.0f;
    float miniOscW = colW - 32.0f;
    float miniOscH = panH - 190.0f;
    if (miniOscH > 40.0f)
    {
        OscilloscopeVisualizer miniOsc(Gdiplus::RectF(col3X + 16.0f, miniOscY, miniOscW, miniOscH));
        miniOsc.draw(g, meters.inWaveform, meters.outWaveform);
    }
}

void MainWindow::renderTabNeural(Gdiplus::Graphics& g)
{
    MeteringData meters;
    dspEngine.getMeteringData(meters);
    neuralMaskVis.draw(g, meters.neuralMask, dspEngine.getSampleRate());
    const ModelProfileStats& stats = dspEngine.getModelStats();
    float infoY = 510.0f;
    float colW = (static_cast<float>(clientWidth) - 60.0f) / 2.0f;
    Gdiplus::SolidBrush panelBg(UITheme::BgPanel);
    Gdiplus::Pen panelPen(UITheme::BorderDark, 1.0f);
    UITheme::fillRoundedRect(g, panelBg, 20.0f, infoY, colW, 230.0f, 8.0f);
    UITheme::drawRoundedRect(g, panelPen, 20.0f, infoY, colW, 230.0f, 8.0f);
    Gdiplus::FontFamily fontFam(L"Segoe UI");
    Gdiplus::Font hdrFont(&fontFam, 11.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::Font bodyFont(&fontFam, 10.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::Font boldFont(&fontFam, 10.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush cyanBrush(UITheme::CyanAccent);
    Gdiplus::SolidBrush textBrush(UITheme::TextPrimary);
    Gdiplus::SolidBrush mutedBrush(UITheme::TextMuted);
    g.DrawString(L"QUANTIZED INT8 ARCHITECTURE & PROFILE", -1, &hdrFont, Gdiplus::PointF(35.0f, infoY + 15.0f), &cyanBrush);
    std::wstringstream ss;
    ss << L"• Input Features: 257 Log-Magnitude Spectral Bins\n"
       << L"• Layer 1 (Dense): 257 -> 64 | INT8 Weights | ReLU6 Table LUT\n"
       << L"• Layer 2 (Conv1D): 64 Channels, K=3, S=1 | INT8 Weights | Tanh LUT\n"
       << L"• Layer 3 (GRU): 64 -> 48 Recurrent State | INT8 Weights | Temporal Memory\n"
       << L"• Layer 4 (Output): 48 -> 257 Spectral Mask [0.0 - 1.0] | Sigmoid LUT\n"
       << L"• Quantization Scheme: Asymmetric INT8 (Scale + ZeroPoint)\n"
       << L"• Total Weights: " << stats.totalWeights << L" INT8 parameters (" << (stats.totalWeights / 1024) << L" KB)\n"
       << L"• Sparsity: " << std::fixed << std::setprecision(1) << stats.sparsityPercent << L"%\n"
       << L"• Multiply-Accumulate Ops (MACs): " << stats.totalMacs << L" ops / frame";
    std::wstring archStr = ss.str();
    g.DrawString(archStr.c_str(), -1, &bodyFont, Gdiplus::RectF(35.0f, infoY + 40.0f, colW - 30.0f, 180.0f), nullptr, &textBrush);
    float histX = 40.0f + colW;
    UITheme::fillRoundedRect(g, panelBg, histX, infoY, colW, 230.0f, 8.0f);
    UITheme::drawRoundedRect(g, panelPen, histX, infoY, colW, 230.0f, 8.0f);
    Gdiplus::SolidBrush emeraldBrush(UITheme::EmeraldAccent);
    g.DrawString(L"INT8 WEIGHT DISTRIBUTION HISTOGRAM & LATENCY", -1, &hdrFont, Gdiplus::PointF(histX + 15.0f, infoY + 15.0f), &emeraldBrush);
    std::wstringstream latSS;
    latSS << L"Last Inference Latency: " << std::fixed << std::setprecision(1) << stats.lastInferenceTimeUs << L" µs | Avg: " << stats.avgInferenceTimeUs << L" µs (Real-time Budget: 2660 µs)";
    std::wstring latStr = latSS.str();
    g.DrawString(latStr.c_str(), -1, &bodyFont, Gdiplus::PointF(histX + 15.0f, infoY + 40.0f), &textBrush);
    float histBarW = (colW - 50.0f) / 16.0f;
    float histBaseY = infoY + 200.0f;
    float maxBucket = 1.0f;
    for (int i = 0; i < 16; ++i) if (stats.weightHistogram[i] > maxBucket) maxBucket = static_cast<float>(stats.weightHistogram[i]);
    for (int i = 0; i < 16; ++i)
    {
        float normH = (static_cast<float>(stats.weightHistogram[i]) / maxBucket) * 110.0f;
        float bx = histX + 25.0f + static_cast<float>(i) * histBarW;
        float by = histBaseY - normH;
        Gdiplus::SolidBrush barBrush(UITheme::CyanAccent);
        g.FillRectangle(&barBrush, bx, by, histBarW - 2.0f, normH);
    }
}

void MainWindow::renderTabAudio(Gdiplus::Graphics& g)
{
    float audY = 110.0f;
    float panW = static_cast<float>(clientWidth - 40);
    Gdiplus::SolidBrush panelBg(UITheme::BgPanel);
    Gdiplus::Pen panelPen(UITheme::BorderDark, 1.0f);
    UITheme::fillRoundedRect(g, panelBg, 20.0f, audY, panW, 190.0f, 8.0f);
    UITheme::drawRoundedRect(g, panelPen, 20.0f, audY, panW, 190.0f, 8.0f);
    Gdiplus::FontFamily fontFam(L"Segoe UI");
    Gdiplus::Font hdrFont(&fontFam, 11.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::Font bodyFont(&fontFam, 10.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush cyanHdr(UITheme::CyanAccent);
    Gdiplus::SolidBrush textBrush(UITheme::TextPrimary);
    Gdiplus::SolidBrush mutedBrush(UITheme::TextMuted);
    g.DrawString(L"WAV AUDIO FILE PROCESSOR", -1, &hdrFont, Gdiplus::PointF(40.0f, audY + 16.0f), &cyanHdr);
    std::wstring fileStatus = loadedWavPath.empty() ? L"No WAV file loaded. Click 'Open WAV File' to load." : (L"File: " + loadedWavPath + L" (" + std::to_wstring(static_cast<int>(audioPlayer.getDurationSeconds())) + L" sec)");
    g.DrawString(fileStatus.c_str(), -1, &bodyFont, Gdiplus::PointF(40.0f, audY + 90.0f), &textBrush);
    btnLoadWav.draw(g);
    btnProcessWav.draw(g);
    sliderScrub.draw(g);
    btnPlay.draw(g);
    btnPause.draw(g);
    btnStop.draw(g);
    btnLoop.draw(g);
    float sigY = audY + 210.0f;
    UITheme::fillRoundedRect(g, panelBg, 20.0f, sigY, panW, 150.0f, 8.0f);
    UITheme::drawRoundedRect(g, panelPen, 20.0f, sigY, panW, 150.0f, 8.0f);
    Gdiplus::SolidBrush emeraldHdr(UITheme::EmeraldAccent);
    g.DrawString(L"REAL-TIME TEST SIGNAL GENERATOR (DEMO)", -1, &hdrFont, Gdiplus::PointF(40.0f, sigY + 16.0f), &emeraldHdr);
    g.DrawString(L"Generate realistic test audio with speech formants, pink noise, white noise, and 60Hz ground hum:", -1, &bodyFont, Gdiplus::PointF(40.0f, sigY + 36.0f), &mutedBrush);
    btnSrcVoice.draw(g);
    btnSrcPink.draw(g);
    btnSrcWhite.draw(g);
    btnSrcHum.draw(g);
    btnSrcSweep.draw(g);
    float oscAreaY = sigY + 170.0f;
    float oscAreaH = static_cast<float>(clientHeight) - oscAreaY - 45.0f;
    if (oscAreaH > 60.0f)
    {
        MeteringData meters;
        dspEngine.getMeteringData(meters);
        OscilloscopeVisualizer bigOsc(Gdiplus::RectF(20.0f, oscAreaY, panW, oscAreaH));
        bigOsc.draw(g, meters.inWaveform, meters.outWaveform);
    }
}
