#include "quantized_layers.h"
#include "default_weights.h"
#include <random>
#include <chrono>

QuantizedDenseLayer::QuantizedDenseLayer(int inFeat, int outFeat, ActivationType act)
{
    inFeatures = inFeat;
    outFeatures = outFeat;
    activation = act;
    weights.resize({outFeatures, inFeatures});
    weights.qparams.scale = 1.0f / 127.0f;
    weights.qparams.zeroPoint = 0;
    bias.resize(outFeatures, 0);
}

void QuantizedDenseLayer::initRandom(float weightRange)
{
    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> dist(-weightRange, weightRange);
    std::vector<float> fweights(weights.size());
    for (size_t i = 0; i < fweights.size(); ++i) fweights[i] = dist(rng);
    weights.quantize(fweights.data(), fweights.size());
    for (size_t i = 0; i < bias.size(); ++i) bias[i] = 0;
}

void QuantizedDenseLayer::forward(const QuantizedTensor& input, QuantizedTensor& output) const
{
    output.resize({outFeatures});
    output.qparams.scale = outputScale;
    output.qparams.zeroPoint = outputZeroPoint;
    std::vector<int32_t> accum(outFeatures, 0);
    QuantizedTensor::matrixVectorMul(weights.data.data(), input.data.data(), accum.data(), outFeatures, inFeatures);
    QuantizedTensor::rescaleAccumulators(accum.data(), bias.data(), output.data.data(), outFeatures, input.qparams.scale, weights.qparams.scale, outputScale, outputZeroPoint);
    const ActivationLUT& lut = ActivationLUT::instance();
    for (int i = 0; i < outFeatures; ++i)
    {
        int8_t v = output.data[i];
        if (activation == ActivationType::Sigmoid) output.data[i] = lut.evaluateSigmoid(v, outputScale, outputZeroPoint, outputScale, outputZeroPoint);
        else if (activation == ActivationType::Tanh) output.data[i] = lut.evaluateTanh(v, outputScale, outputZeroPoint, outputScale, outputZeroPoint);
        else if (activation == ActivationType::ReLU6) output.data[i] = lut.evaluateReLU6(v, outputScale, outputZeroPoint, outputScale, outputZeroPoint);
    }
}

QuantizedConv1DLayer::QuantizedConv1DLayer(int inCh, int outCh, int kSize, int str, int pad, ActivationType act)
{
    inChannels = inCh;
    outChannels = outCh;
    kernelSize = kSize;
    stride = str;
    padding = pad;
    activation = act;
    weights.resize({outChannels, inChannels, kernelSize});
    weights.qparams.scale = 1.0f / 127.0f;
    weights.qparams.zeroPoint = 0;
    bias.resize(outChannels, 0);
}

void QuantizedConv1DLayer::initRandom(float weightRange)
{
    std::mt19937 rng(4242);
    std::uniform_real_distribution<float> dist(-weightRange, weightRange);
    std::vector<float> fweights(weights.size());
    for (size_t i = 0; i < fweights.size(); ++i) fweights[i] = dist(rng);
    weights.quantize(fweights.data(), fweights.size());
    for (size_t i = 0; i < bias.size(); ++i) bias[i] = 0;
}

void QuantizedConv1DLayer::forward(const QuantizedTensor& input, QuantizedTensor& output) const
{
    int inLen = static_cast<int>(input.data.size()) / inChannels;
    if (inLen < 1) inLen = static_cast<int>(input.data.size());
    int outLen = (inLen + 2 * padding - kernelSize) / stride + 1;
    if (outLen < 1) outLen = 1;
    output.resize({outChannels, outLen});
    output.qparams.scale = outputScale;
    output.qparams.zeroPoint = outputZeroPoint;
    const ActivationLUT& lut = ActivationLUT::instance();
    for (int oc = 0; oc < outChannels; ++oc)
    {
        for (int ol = 0; ol < outLen; ++ol)
        {
            int32_t acc = bias[oc];
            int inStart = ol * stride - padding;
            for (int k = 0; k < kernelSize; ++k)
            {
                int inIdx = inStart + k;
                if (inIdx >= 0 && inIdx < inLen)
                {
                    for (int ic = 0; ic < inChannels; ++ic)
                    {
                        int8_t inVal = input.data[ic * inLen + inIdx];
                        int8_t wVal = weights.data[(oc * inChannels + ic) * kernelSize + k];
                        acc += static_cast<int32_t>(inVal) * static_cast<int32_t>(wVal);
                    }
                }
            }
            float fval = static_cast<float>(acc) * (input.qparams.scale * weights.qparams.scale) / outputScale + static_cast<float>(outputZeroPoint);
            float clamped = std::max(-128.0f, std::min(127.0f, std::round(fval)));
            int8_t qval = static_cast<int8_t>(clamped);
            if (activation == ActivationType::Sigmoid) qval = lut.evaluateSigmoid(qval, outputScale, outputZeroPoint, outputScale, outputZeroPoint);
            else if (activation == ActivationType::Tanh) qval = lut.evaluateTanh(qval, outputScale, outputZeroPoint, outputScale, outputZeroPoint);
            else if (activation == ActivationType::ReLU6) qval = lut.evaluateReLU6(qval, outputScale, outputZeroPoint, outputScale, outputZeroPoint);
            output.data[oc * outLen + ol] = qval;
        }
    }
}

QuantizedGRULayer::QuantizedGRULayer(int inSize, int hidSize)
{
    inputSize = inSize;
    hiddenSize = hidSize;
    weightIH.resize({3 * hiddenSize, inputSize});
    weightIH.qparams.scale = 1.0f / 127.0f;
    weightHH.resize({3 * hiddenSize, hiddenSize});
    weightHH.qparams.scale = 1.0f / 127.0f;
    biasIH.resize(3 * hiddenSize, 0);
    biasHH.resize(3 * hiddenSize, 0);
    hiddenState.resize({hiddenSize});
    hiddenState.qparams.scale = 1.0f / 127.0f;
    hiddenState.fill(0);
}

void QuantizedGRULayer::initRandom(float weightRange)
{
    std::mt19937 rng(9999);
    std::uniform_real_distribution<float> dist(-weightRange, weightRange);
    std::vector<float> fwIH(weightIH.size());
    for (size_t i = 0; i < fwIH.size(); ++i) fwIH[i] = dist(rng);
    weightIH.quantize(fwIH.data(), fwIH.size());
    std::vector<float> fwHH(weightHH.size());
    for (size_t i = 0; i < fwHH.size(); ++i) fwHH[i] = dist(rng);
    weightHH.quantize(fwHH.data(), fwHH.size());
    hiddenState.fill(0);
}

void QuantizedGRULayer::resetState()
{
    hiddenState.fill(0);
}

void QuantizedGRULayer::forward(const QuantizedTensor& input, QuantizedTensor& output)
{
    output.resize({hiddenSize});
    output.qparams.scale = outputScale;
    output.qparams.zeroPoint = outputZeroPoint;
    std::vector<int32_t> gatesIH(3 * hiddenSize, 0);
    std::vector<int32_t> gatesHH(3 * hiddenSize, 0);
    QuantizedTensor::matrixVectorMul(weightIH.data.data(), input.data.data(), gatesIH.data(), 3 * hiddenSize, inputSize);
    QuantizedTensor::matrixVectorMul(weightHH.data.data(), hiddenState.data.data(), gatesHH.data(), 3 * hiddenSize, hiddenSize);
    float inScale = input.qparams.scale * weightIH.qparams.scale;
    float hScale = hiddenState.qparams.scale * weightHH.qparams.scale;
    for (int h = 0; h < hiddenSize; ++h)
    {
        float rRaw = static_cast<float>(gatesIH[h] + biasIH[h]) * inScale + static_cast<float>(gatesHH[h] + biasHH[h]) * hScale;
        float r = 1.0f / (1.0f + std::exp(-std::max(-10.0f, std::min(10.0f, rRaw))));
        float zRaw = static_cast<float>(gatesIH[hiddenSize + h] + biasIH[hiddenSize + h]) * inScale + static_cast<float>(gatesHH[hiddenSize + h] + biasHH[hiddenSize + h]) * hScale;
        float z = 1.0f / (1.0f + std::exp(-std::max(-10.0f, std::min(10.0f, zRaw))));
        float nRaw = static_cast<float>(gatesIH[2 * hiddenSize + h] + biasIH[2 * hiddenSize + h]) * inScale + r * (static_cast<float>(gatesHH[2 * hiddenSize + h] + biasHH[2 * hiddenSize + h]) * hScale);
        float n = std::tanh(std::max(-6.0f, std::min(6.0f, nRaw)));
        float hPrev = static_cast<float>(hiddenState.data[h]) * hiddenState.qparams.scale;
        float hNew = (1.0f - z) * n + z * hPrev;
        int8_t qH = QuantizedTensor::quantizeValue(hNew, outputScale, outputZeroPoint);
        hiddenState.data[h] = qH;
        output.data[h] = qH;
    }
}

QuantizedSpectralMaskNet::QuantizedSpectralMaskNet()
{
    inputLayer = QuantizedDenseLayer(NUM_BINS, EMBED_DIM, ActivationType::ReLU6);
    convLayer = QuantizedConv1DLayer(1, 1, 3, 1, 1, ActivationType::Tanh);
    gruLayer = QuantizedGRULayer(EMBED_DIM, GRU_HIDDEN);
    outputLayer = QuantizedDenseLayer(GRU_HIDDEN, NUM_BINS, ActivationType::Sigmoid);
    tensorIn = QuantizedTensor({NUM_BINS}, 1.0f / 127.0f, 0);
    tensorEmbed = QuantizedTensor({EMBED_DIM}, 1.0f / 127.0f, 0);
    tensorConv = QuantizedTensor({EMBED_DIM}, 1.0f / 127.0f, 0);
    tensorGru = QuantizedTensor({GRU_HIDDEN}, 1.0f / 127.0f, 0);
    tensorOut = QuantizedTensor({NUM_BINS}, 1.0f / 127.0f, 0);
    initDefaultWeights();
    computeProfileStats();
}

void QuantizedSpectralMaskNet::initDefaultWeights()
{
    inputLayer.weights.resize({EMBED_DIM, NUM_BINS});
    convLayer.weights.resize({1, 1, 3});
    gruLayer.weightIH.resize({3 * GRU_HIDDEN, EMBED_DIM});
    gruLayer.weightHH.resize({3 * GRU_HIDDEN, GRU_HIDDEN});
    outputLayer.weights.resize({NUM_BINS, GRU_HIDDEN});
    DefaultModelWeights::generateCalibratedWeights(NUM_BINS, EMBED_DIM, GRU_HIDDEN, inputLayer.weights.data.data(), convLayer.weights.data.data(), gruLayer.weightIH.data.data(), gruLayer.weightHH.data.data(), outputLayer.weights.data.data());
    for (size_t i = 0; i < inputLayer.bias.size(); ++i) inputLayer.bias[i] = 10;
    for (size_t i = 0; i < outputLayer.bias.size(); ++i) outputLayer.bias[i] = 20;
}

void QuantizedSpectralMaskNet::resetState()
{
    gruLayer.resetState();
}

void QuantizedSpectralMaskNet::forward(const float* inputMagnitudes, float* outputMask, float sensitivity)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    std::vector<float> logMags(NUM_BINS);
    for (int i = 0; i < NUM_BINS; ++i)
    {
        float m = std::max(0.0f, inputMagnitudes[i]);
        logMags[i] = std::log1p(m * sensitivity * 10.0f) * 0.25f;
    }
    tensorIn.quantize(logMags.data(), NUM_BINS);
    inputLayer.forward(tensorIn, tensorEmbed);
    convLayer.forward(tensorEmbed, tensorConv);
    gruLayer.forward(tensorConv, tensorGru);
    outputLayer.forward(tensorGru, tensorOut);
    tensorOut.dequantize(outputMask, NUM_BINS);
    for (int i = 0; i < NUM_BINS; ++i)
    {
        float rawM = outputMask[i];
        float shaped = 1.0f / (1.0f + std::exp(-8.0f * (rawM - 0.35f)));
        outputMask[i] = std::max(0.0f, std::min(1.0f, shaped));
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    double durationUs = std::chrono::duration<double, std::micro>(endTime - startTime).count();
    stats.lastInferenceTimeUs = durationUs;
    stats.totalInferences++;
    stats.avgInferenceTimeUs = (stats.avgInferenceTimeUs * 0.95) + (durationUs * 0.05);
}

void QuantizedSpectralMaskNet::computeProfileStats()
{
    stats.totalWeights = static_cast<int>(inputLayer.weights.size() + convLayer.weights.size() + gruLayer.weightIH.size() + gruLayer.weightHH.size() + outputLayer.weights.size());
    stats.totalMacs = static_cast<uint64_t>(inputLayer.weights.size()) + static_cast<uint64_t>(convLayer.weights.size()) + static_cast<uint64_t>(gruLayer.weightIH.size() + gruLayer.weightHH.size()) + static_cast<uint64_t>(outputLayer.weights.size());
    int zeroCount = 0;
    std::fill(std::begin(stats.weightHistogram), std::end(stats.weightHistogram), 0);
    auto countWeights = [&](const QuantizedTensor& t)
    {
        for (int8_t v : t.data)
        {
            if (v == 0) zeroCount++;
            int bucket = (static_cast<int>(v) + 128) / 16;
            if (bucket >= 0 && bucket < 16) stats.weightHistogram[bucket]++;
        }
    };
    countWeights(inputLayer.weights);
    countWeights(convLayer.weights);
    countWeights(gruLayer.weightIH);
    countWeights(gruLayer.weightHH);
    countWeights(outputLayer.weights);
    stats.sparsityPercent = (static_cast<float>(zeroCount) / static_cast<float>(stats.totalWeights)) * 100.0f;
}
