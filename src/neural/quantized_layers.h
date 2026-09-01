#pragma once

#include "quantized_tensor.h"
#include <vector>
#include <string>
#include <chrono>

enum class ActivationType
{
    None = 0,
    Sigmoid,
    Tanh,
    ReLU6
};

class QuantizedDenseLayer
{
public:
    int inFeatures = 0;
    int outFeatures = 0;
    ActivationType activation = ActivationType::None;
    QuantizedTensor weights;
    std::vector<int32_t> bias;
    float outputScale = 1.0f / 127.0f;
    int8_t outputZeroPoint = 0;

    QuantizedDenseLayer() = default;
    QuantizedDenseLayer(int inFeat, int outFeat, ActivationType act = ActivationType::None);
    void initRandom(float weightRange = 0.5f);
    void forward(const QuantizedTensor& input, QuantizedTensor& output) const;
};

class QuantizedConv1DLayer
{
public:
    int inChannels = 0;
    int outChannels = 0;
    int kernelSize = 3;
    int stride = 1;
    int padding = 1;
    ActivationType activation = ActivationType::None;
    QuantizedTensor weights;
    std::vector<int32_t> bias;
    float outputScale = 1.0f / 127.0f;
    int8_t outputZeroPoint = 0;

    QuantizedConv1DLayer() = default;
    QuantizedConv1DLayer(int inCh, int outCh, int kSize, int str = 1, int pad = 1, ActivationType act = ActivationType::None);
    void initRandom(float weightRange = 0.5f);
    void forward(const QuantizedTensor& input, QuantizedTensor& output) const;
};

class QuantizedGRULayer
{
public:
    int inputSize = 0;
    int hiddenSize = 0;
    QuantizedTensor weightIH;
    QuantizedTensor weightHH;
    std::vector<int32_t> biasIH;
    std::vector<int32_t> biasHH;
    QuantizedTensor hiddenState;
    float outputScale = 1.0f / 127.0f;
    int8_t outputZeroPoint = 0;

    QuantizedGRULayer() = default;
    QuantizedGRULayer(int inSize, int hidSize);
    void initRandom(float weightRange = 0.3f);
    void resetState();
    void forward(const QuantizedTensor& input, QuantizedTensor& output);
};

struct ModelProfileStats
{
    double lastInferenceTimeUs = 0.0;
    double avgInferenceTimeUs = 0.0;
    uint64_t totalInferences = 0;
    uint64_t totalMacs = 0;
    float sparsityPercent = 0.0f;
    int totalWeights = 0;
    int weightHistogram[16] = {0};
};

class QuantizedSpectralMaskNet
{
public:
    static constexpr int NUM_BINS = 257;
    static constexpr int EMBED_DIM = 64;
    static constexpr int GRU_HIDDEN = 48;

    QuantizedDenseLayer inputLayer;
    QuantizedConv1DLayer convLayer;
    QuantizedGRULayer gruLayer;
    QuantizedDenseLayer outputLayer;
    ModelProfileStats stats;

    QuantizedSpectralMaskNet();
    void initDefaultWeights();
    void resetState();
    void forward(const float* inputMagnitudes, float* outputMask, float sensitivity = 1.0f);
    void computeProfileStats();

private:
    QuantizedTensor tensorIn;
    QuantizedTensor tensorEmbed;
    QuantizedTensor tensorConv;
    QuantizedTensor tensorGru;
    QuantizedTensor tensorOut;
};
