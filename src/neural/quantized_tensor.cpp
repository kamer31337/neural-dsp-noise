#include "quantized_tensor.h"

QuantizedTensor::QuantizedTensor(const std::vector<int>& dims, float scale, int8_t zeroPoint)
{
    qparams.scale = scale;
    qparams.zeroPoint = zeroPoint;
    resize(dims);
}

size_t QuantizedTensor::size() const
{
    if (shape.empty()) return 0;
    size_t total = 1;
    for (size_t i = 0; i < shape.size(); ++i) total *= static_cast<size_t>(shape[i]);
    return total;
}

void QuantizedTensor::resize(const std::vector<int>& dims)
{
    shape = dims;
    data.resize(size(), 0);
}

void QuantizedTensor::quantize(const float* src, size_t count)
{
    if (count > data.size()) data.resize(count);
    for (size_t i = 0; i < count; ++i) data[i] = quantizeValue(src[i], qparams.scale, qparams.zeroPoint);
}

void QuantizedTensor::dequantize(float* dst, size_t count) const
{
    size_t n = std::min(count, data.size());
    for (size_t i = 0; i < n; ++i) dst[i] = dequantizeValue(data[i], qparams.scale, qparams.zeroPoint);
}

void QuantizedTensor::fill(int8_t value)
{
    std::fill(data.begin(), data.end(), value);
}

int8_t QuantizedTensor::quantizeValue(float x, float scale, int8_t zeroPoint)
{
    if (scale <= 0.0f) scale = 1e-6f;
    float scaled = std::round(x / scale) + static_cast<float>(zeroPoint);
    if (scaled > 127.0f) return 127;
    if (scaled < -128.0f) return -128;
    return static_cast<int8_t>(scaled);
}

float QuantizedTensor::dequantizeValue(int8_t q, float scale, int8_t zeroPoint)
{
    return (static_cast<float>(q) - static_cast<float>(zeroPoint)) * scale;
}

int32_t QuantizedTensor::dotProduct(const int8_t* a, const int8_t* b, size_t count)
{
    int32_t sum = 0;
    size_t i = 0;
    for (; i + 4 <= count; i += 4)
    {
        sum += static_cast<int32_t>(a[i]) * static_cast<int32_t>(b[i]);
        sum += static_cast<int32_t>(a[i + 1]) * static_cast<int32_t>(b[i + 1]);
        sum += static_cast<int32_t>(a[i + 2]) * static_cast<int32_t>(b[i + 2]);
        sum += static_cast<int32_t>(a[i + 3]) * static_cast<int32_t>(b[i + 3]);
    }
    for (; i < count; ++i) sum += static_cast<int32_t>(a[i]) * static_cast<int32_t>(b[i]);
    return sum;
}

void QuantizedTensor::matrixVectorMul(const int8_t* weightMatrix, const int8_t* inputVec, int32_t* outputAcc, int rows, int cols)
{
    for (int r = 0; r < rows; ++r)
    {
        outputAcc[r] = dotProduct(&weightMatrix[r * cols], inputVec, static_cast<size_t>(cols));
    }
}

void QuantizedTensor::rescaleAccumulators(const int32_t* acc, const int32_t* bias, int8_t* output, int count, float inScale, float weightScale, float outScale, int8_t outZeroPoint)
{
    float combinedScale = (inScale * weightScale) / (outScale <= 0.0f ? 1e-6f : outScale);
    for (int i = 0; i < count; ++i)
    {
        int32_t total = acc[i] + (bias ? bias[i] : 0);
        float fval = static_cast<float>(total) * combinedScale + static_cast<float>(outZeroPoint);
        float clamped = std::max(-128.0f, std::min(127.0f, std::round(fval)));
        output[i] = static_cast<int8_t>(clamped);
    }
}

ActivationLUT::ActivationLUT()
{
    for (int i = 0; i < 256; ++i)
    {
        int8_t v = static_cast<int8_t>(i - 128);
        float x = static_cast<float>(v) / 16.0f;
        sigmoidLUT[i] = 1.0f / (1.0f + std::exp(-x));
        tanhLUT[i] = std::tanh(x);
    }
}

const ActivationLUT& ActivationLUT::instance()
{
    static ActivationLUT inst;
    return inst;
}

int8_t ActivationLUT::evaluateSigmoid(int8_t x, float scale, int8_t zeroPoint, float outScale, int8_t outZeroPoint) const
{
    float realX = (static_cast<float>(x) - static_cast<float>(zeroPoint)) * scale;
    float sig = 1.0f / (1.0f + std::exp(-std::max(-12.0f, std::min(12.0f, realX))));
    return QuantizedTensor::quantizeValue(sig, outScale, outZeroPoint);
}

int8_t ActivationLUT::evaluateTanh(int8_t x, float scale, int8_t zeroPoint, float outScale, int8_t outZeroPoint) const
{
    float realX = (static_cast<float>(x) - static_cast<float>(zeroPoint)) * scale;
    float t = std::tanh(std::max(-8.0f, std::min(8.0f, realX)));
    return QuantizedTensor::quantizeValue(t, outScale, outZeroPoint);
}

int8_t ActivationLUT::evaluateReLU6(int8_t x, float scale, int8_t zeroPoint, float outScale, int8_t outZeroPoint) const
{
    float realX = (static_cast<float>(x) - static_cast<float>(zeroPoint)) * scale;
    float r = std::max(0.0f, std::min(6.0f, realX));
    return QuantizedTensor::quantizeValue(r, outScale, outZeroPoint);
}
