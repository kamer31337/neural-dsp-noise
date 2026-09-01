#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include "../dsp/dsp_pipeline.h"

enum class SignalSource
{
    WavPlayback = 0,
    TestVoiceWithNoise,
    TestPinkNoise,
    TestWhiteNoise,
    TestHum60Hz,
    TestSineSweep
};

class AudioPlayer
{
public:
    AudioPlayer(DSPPipeline& dsp);
    ~AudioPlayer();

    bool init(uint32_t sampleRate = 48000);
    void shutdown();
    void play();
    void pause();
    void stop();
    bool isPlaying() const;
    void setSignalSource(SignalSource src);
    SignalSource getSignalSource() const;
    bool loadWav(const std::wstring& path);
    bool processAndSaveWav(const std::wstring& inPath, const std::wstring& outPath);
    void setLoop(bool loop);
    bool isLoop() const;
    float getPlaybackProgress() const;
    void seekTo(float normalizedPos);
    float getDurationSeconds() const;

private:
    DSPPipeline& dspEngine;
    uint32_t sampleRate = 48000;
    HWAVEOUT hWaveOut = nullptr;
    static constexpr int NUM_BUFFERS = 4;
    static constexpr int BUFFER_SAMPLES = 1024;
    WAVEHDR waveHeaders[NUM_BUFFERS];
    std::vector<int16_t> audioBuffers[NUM_BUFFERS];
    std::atomic<bool> isRunning{false};
    std::atomic<bool> isPaused{false};
    std::atomic<bool> loopAudio{true};
    std::atomic<SignalSource> currentSource{SignalSource::TestVoiceWithNoise};
    std::thread playbackThread;
    mutable std::mutex fileMutex;
    std::vector<float> loadedSamples;
    size_t playCursor = 0;
    float synthPhase = 0.0f;
    float pinkB0 = 0.0f;
    float pinkB1 = 0.0f;
    float pinkB2 = 0.0f;
    float pinkB3 = 0.0f;
    float pinkB4 = 0.0f;
    float pinkB5 = 0.0f;
    float pinkB6 = 0.0f;
    float voiceTime = 0.0f;
    float sweepPhase = 0.0f;

    void playbackLoop();
    void fillAudioBlock(float* outBuffer, size_t numSamples);
    void generateTestSignal(float* outBuffer, size_t numSamples);
    float samplePinkNoise();
};
