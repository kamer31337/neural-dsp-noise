#include "audio_player.h"
#include "wav_file.h"
#include <cmath>
#include <random>

constexpr float PI_2 = 6.28318530717958647692f;

AudioPlayer::AudioPlayer(DSPPipeline& dsp) : dspEngine(dsp)
{
}

AudioPlayer::~AudioPlayer()
{
    shutdown();
}

bool AudioPlayer::init(uint32_t sr)
{
    if (isRunning) shutdown();
    sampleRate = sr;
    WAVEFORMATEX wfx;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = sampleRate;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;
    MMRESULT res = waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    if (res != MMSYSERR_NOERROR)
    {
        hWaveOut = nullptr;
    }
    else
    {
        for (int i = 0; i < NUM_BUFFERS; ++i)
        {
            audioBuffers[i].assign(BUFFER_SAMPLES, 0);
            ZeroMemory(&waveHeaders[i], sizeof(WAVEHDR));
            waveHeaders[i].lpData = reinterpret_cast<LPSTR>(audioBuffers[i].data());
            waveHeaders[i].dwBufferLength = BUFFER_SAMPLES * sizeof(int16_t);
            waveHeaders[i].dwFlags = 0;
            waveOutPrepareHeader(hWaveOut, &waveHeaders[i], sizeof(WAVEHDR));
        }
    }
    isRunning = true;
    isPaused = false;
    playbackThread = std::thread(&AudioPlayer::playbackLoop, this);
    return true;
}

void AudioPlayer::shutdown()
{
    if (isRunning)
    {
        isRunning = false;
        if (playbackThread.joinable()) playbackThread.join();
    }
    if (hWaveOut)
    {
        waveOutReset(hWaveOut);
        for (int i = 0; i < NUM_BUFFERS; ++i)
        {
            if (waveHeaders[i].dwFlags & WHDR_PREPARED) waveOutUnprepareHeader(hWaveOut, &waveHeaders[i], sizeof(WAVEHDR));
        }
        waveOutClose(hWaveOut);
        hWaveOut = nullptr;
    }
}

void AudioPlayer::play()
{
    isPaused = false;
}

void AudioPlayer::pause()
{
    isPaused = true;
}

void AudioPlayer::stop()
{
    isPaused = true;
    std::lock_guard<std::mutex> lock(fileMutex);
    playCursor = 0;
}

bool AudioPlayer::isPlaying() const
{
    return isRunning && !isPaused;
}

void AudioPlayer::setSignalSource(SignalSource src)
{
    currentSource = src;
}

SignalSource AudioPlayer::getSignalSource() const
{
    return currentSource.load();
}

bool AudioPlayer::loadWav(const std::wstring& path)
{
    std::lock_guard<std::mutex> lock(fileMutex);
    uint32_t sr = 48000;
    uint16_t ch = 1;
    bool ok = WavFile::load(path, loadedSamples, sr, ch);
    if (ok)
    {
        if (ch == 2)
        {
            size_t monoLen = loadedSamples.size() / 2;
            std::vector<float> mono(monoLen);
            for (size_t i = 0; i < monoLen; ++i) mono[i] = 0.5f * (loadedSamples[i * 2] + loadedSamples[i * 2 + 1]);
            loadedSamples = std::move(mono);
        }
        playCursor = 0;
        currentSource = SignalSource::WavPlayback;
    }
    return ok;
}

bool AudioPlayer::processAndSaveWav(const std::wstring& inPath, const std::wstring& outPath)
{
    std::vector<float> inSamples;
    uint32_t sr = 48000;
    uint16_t ch = 1;
    if (!WavFile::load(inPath, inSamples, sr, ch)) return false;
    std::vector<float> monoIn;
    if (ch == 2)
    {
        size_t monoLen = inSamples.size() / 2;
        monoIn.resize(monoLen);
        for (size_t i = 0; i < monoLen; ++i) monoIn[i] = 0.5f * (inSamples[i * 2] + inSamples[i * 2 + 1]);
    }
    else
    {
        monoIn = inSamples;
    }
    std::vector<float> outSamples(monoIn.size());
    size_t blockSize = 512;
    for (size_t i = 0; i < monoIn.size(); i += blockSize)
    {
        size_t count = std::min(blockSize, monoIn.size() - i);
        dspEngine.processBlock(&monoIn[i], &outSamples[i], count);
    }
    return WavFile::save(outPath, outSamples, sr, 1, 16);
}

void AudioPlayer::setLoop(bool loop)
{
    loopAudio = loop;
}

bool AudioPlayer::isLoop() const
{
    return loopAudio.load();
}

float AudioPlayer::getPlaybackProgress() const
{
    std::lock_guard<std::mutex> lock(fileMutex);
    if (loadedSamples.empty()) return 0.0f;
    return static_cast<float>(playCursor) / static_cast<float>(loadedSamples.size());
}

void AudioPlayer::seekTo(float normalizedPos)
{
    std::lock_guard<std::mutex> lock(fileMutex);
    if (loadedSamples.empty()) return;
    float clamped = std::max(0.0f, std::min(1.0f, normalizedPos));
    playCursor = static_cast<size_t>(clamped * static_cast<float>(loadedSamples.size()));
}

float AudioPlayer::getDurationSeconds() const
{
    std::lock_guard<std::mutex> lock(fileMutex);
    if (loadedSamples.empty()) return 0.0f;
    return static_cast<float>(loadedSamples.size()) / static_cast<float>(sampleRate);
}

float AudioPlayer::samplePinkNoise()
{
    float white = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
    pinkB0 = 0.99886f * pinkB0 + white * 0.0555179f;
    pinkB1 = 0.99332f * pinkB1 + white * 0.0750759f;
    pinkB2 = 0.96900f * pinkB2 + white * 0.1538520f;
    pinkB3 = 0.86650f * pinkB3 + white * 0.3104856f;
    pinkB4 = 0.55000f * pinkB4 + white * 0.5329522f;
    pinkB5 = -0.7616f * pinkB5 - white * 0.0168980f;
    float pink = pinkB0 + pinkB1 + pinkB2 + pinkB3 + pinkB4 + pinkB5 + pinkB6 + white * 0.5362f;
    pinkB6 = white * 0.115926f;
    return pink * 0.11f;
}

void AudioPlayer::generateTestSignal(float* outBuffer, size_t numSamples)
{
    SignalSource src = currentSource.load();
    for (size_t i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;
        if (src == SignalSource::TestVoiceWithNoise)
        {
            voiceTime += 1.0f / static_cast<float>(sampleRate);
            float f0 = 130.0f + 15.0f * std::sin(voiceTime * 1.5f);
            synthPhase += f0 * PI_2 / static_cast<float>(sampleRate);
            if (synthPhase >= PI_2) synthPhase -= PI_2;
            float voice = std::sin(synthPhase) * 0.4f + std::sin(synthPhase * 2.0f) * 0.25f + std::sin(synthPhase * 3.0f) * 0.15f + std::sin(synthPhase * 4.0f) * 0.1f;
            float speechVowel = std::sin(voiceTime * 3.0f) * 0.5f + 0.5f;
            voice *= speechVowel;
            float pink = samplePinkNoise() * 0.35f;
            float white = ((static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f) * 0.08f;
            float hum = (std::sin(voiceTime * 60.0f * PI_2) * 0.2f) + (std::sin(voiceTime * 180.0f * PI_2) * 0.1f);
            sample = voice * 0.6f + pink + white + hum;
        }
        else if (src == SignalSource::TestPinkNoise)
        {
            sample = samplePinkNoise() * 0.8f;
        }
        else if (src == SignalSource::TestWhiteNoise)
        {
            sample = ((static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f) * 0.4f;
        }
        else if (src == SignalSource::TestHum60Hz)
        {
            voiceTime += 1.0f / static_cast<float>(sampleRate);
            sample = (std::sin(voiceTime * 60.0f * PI_2) * 0.5f) + (std::sin(voiceTime * 120.0f * PI_2) * 0.3f) + (std::sin(voiceTime * 180.0f * PI_2) * 0.15f);
        }
        else if (src == SignalSource::TestSineSweep)
        {
            voiceTime += 1.0f / static_cast<float>(sampleRate);
            if (voiceTime > 4.0f) voiceTime = 0.0f;
            float freq = 40.0f * std::pow(20000.0f / 40.0f, voiceTime / 4.0f);
            sweepPhase += freq * PI_2 / static_cast<float>(sampleRate);
            if (sweepPhase >= PI_2) sweepPhase -= PI_2;
            sample = std::sin(sweepPhase) * 0.5f;
        }
        outBuffer[i] = sample;
    }
}

void AudioPlayer::fillAudioBlock(float* outBuffer, size_t numSamples)
{
    if (isPaused)
    {
        std::fill(outBuffer, outBuffer + numSamples, 0.0f);
        return;
    }
    if (currentSource == SignalSource::WavPlayback)
    {
        std::lock_guard<std::mutex> lock(fileMutex);
        if (loadedSamples.empty())
        {
            std::fill(outBuffer, outBuffer + numSamples, 0.0f);
            return;
        }
        for (size_t i = 0; i < numSamples; ++i)
        {
            if (playCursor < loadedSamples.size())
            {
                outBuffer[i] = loadedSamples[playCursor++];
            }
            else
            {
                if (loopAudio)
                {
                    playCursor = 0;
                    outBuffer[i] = loadedSamples[playCursor++];
                }
                else
                {
                    outBuffer[i] = 0.0f;
                    isPaused = true;
                }
            }
        }
    }
    else
    {
        generateTestSignal(outBuffer, numSamples);
    }
}

void AudioPlayer::playbackLoop()
{
    std::vector<float> rawBuffer(BUFFER_SAMPLES);
    std::vector<float> processedBuffer(BUFFER_SAMPLES);
    int bufferIdx = 0;
    while (isRunning)
    {
        if (hWaveOut)
        {
            WAVEHDR& hdr = waveHeaders[bufferIdx];
            while (isRunning && (hdr.dwFlags & WHDR_PREPARED) && !(hdr.dwFlags & WHDR_DONE) && (hdr.dwFlags & WHDR_INQUEUE))
            {
                Sleep(5);
            }
            if (!isRunning) break;
            fillAudioBlock(rawBuffer.data(), BUFFER_SAMPLES);
            dspEngine.processBlock(rawBuffer.data(), processedBuffer.data(), BUFFER_SAMPLES);
            for (int i = 0; i < BUFFER_SAMPLES; ++i)
            {
                float s = std::max(-1.0f, std::min(1.0f, processedBuffer[i]));
                audioBuffers[bufferIdx][i] = static_cast<int16_t>(std::round(s * 32767.0f));
            }
            if (hdr.dwFlags & WHDR_PREPARED) waveOutUnprepareHeader(hWaveOut, &hdr, sizeof(WAVEHDR));
            hdr.lpData = reinterpret_cast<LPSTR>(audioBuffers[bufferIdx].data());
            hdr.dwBufferLength = BUFFER_SAMPLES * sizeof(int16_t);
            hdr.dwFlags = 0;
            waveOutPrepareHeader(hWaveOut, &hdr, sizeof(WAVEHDR));
            waveOutWrite(hWaveOut, &hdr, sizeof(WAVEHDR));
            bufferIdx = (bufferIdx + 1) % NUM_BUFFERS;
        }
        else
        {
            fillAudioBlock(rawBuffer.data(), BUFFER_SAMPLES);
            dspEngine.processBlock(rawBuffer.data(), processedBuffer.data(), BUFFER_SAMPLES);
            Sleep(20);
        }
    }
}
