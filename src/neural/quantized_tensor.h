#pragma once

#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstring>

struct QuantizationParams
{
    float scale = 1.0f;
    int8_t zeroPoint = 0;
    float minVal = -1.0f;
    float maxVal = 1.0f;
};

class QuantizedTensor
{
public:
    std::vector<int8_t> data;
    std::vector<int> shape;
    QuantizationParams qparams;

    QuantizedTensor() = default;
    QuantizedTensor(const std::vector<int>& dims, float scale = 1.0f, int8_t zeroPoint = 0);

    size_t size() const;
    void resize(const std::vector<int>& dims);
    void quantize(const float* src, size_t count);
    void dequantize(float* dst, size_t count) const;
    void fill(int8_t value);

    static int8_t quantizeValue(float x, float scale, int8_t zeroPoint);
    static float dequantizeValue(int8_t q, float scale, int8_t zeroPoint);
    static int32_t dotProduct(const int8_t* a, const int8_t* b, size_t count);
    static void matrixVectorMul(const int8_t* weightMatrix, const int8_t* inputVec, int32_t* outputAcc, int rows, int cols);
    static void rescaleAccumulators(const int32_t* acc, const int32_t* bias, int8_t* output, int count, float inScale, float weightScale, float outScale, int8_t outZeroPoint);
};

class ActivationLUT
{
public:
    static const ActivationLUT& instance();
    int8_t evaluateSigmoid(int8_t x, float scale, int8_t zeroPoint, float outScale, int8_t outZeroPoint) const;
    int8_t evaluateTanh(int8_t x, float scale, int8_t zeroPoint, float outScale, int8_t outZeroPoint) const;
    int8_t evaluateReLU6(int8_t x, float scale, int8_t zeroPoint, float outScale, int8_t outZeroPoint) const;

private:
    ActivationLUT();
    float sigmoidLUT[256];
    float tanhLUT[256];
};
