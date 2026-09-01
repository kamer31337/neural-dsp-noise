#include <windows.h>
#include <gdiplus.h>
#include "dsp/dsp_pipeline.h"
#include "audio/audio_player.h"
#include "gui/gui_window.h"

#ifdef _MSC_VER
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")
#endif

static int RunApp(HINSTANCE hInstance, int nCmdShow)
{
    if (!hInstance) hInstance = GetModuleHandleW(nullptr);
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken = 0;
    Gdiplus::Status status = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
    if (status != Gdiplus::Ok)
    {
        MessageBoxW(nullptr, L"Failed to initialize GDI+ library.", L"Initialization Error", MB_OK | MB_ICONERROR);
        return -1;
    }
    DSPPipeline dspEngine;
    dspEngine.init(48000.0f, 512, 128);
    AudioPlayer audioPlayer(dspEngine);
    MainWindow mainWindow(hInstance, dspEngine, audioPlayer);
    if (!mainWindow.create(1180, 840, L"Neural DSP - Quantized Noise Reduction & Sound Enhancer"))
    {
        MessageBoxW(nullptr, L"Failed to create main application window.", L"Initialization Error", MB_OK | MB_ICONERROR);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return -1;
    }
    mainWindow.show(nCmdShow);
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return static_cast<int>(msg.wParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    return RunApp(hInstance, nCmdShow);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)pCmdLine;
    return RunApp(hInstance, nCmdShow);
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    return RunApp(GetModuleHandleW(nullptr), SW_SHOWDEFAULT);
}