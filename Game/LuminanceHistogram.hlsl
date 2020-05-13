#define TILE_SIZE   16
#define NUM_BINS    256
#define EPSILON     0.005f

cbuffer cb : register(b0)
{
    uint2 inputSize;
    float minLogLuminance;
    float logLuminanceRange;
    float invLogLuminanceRange;
    float deltaTime;
    float tau;
};

Texture2D<float4> framebuffer : register(t0);

RWByteAddressBuffer histogram : register(u0);
RWTexture2D<float> averageLuminance : register(u1);

groupshared uint histogramBins[NUM_BINS];

float getLuminance(float4 color)
{
    return dot(color, float4(0.2127f, 0.7152f, 0.0722f, 0.0f));
}

uint getBinIndex(float luminance)
{
    if (luminance < EPSILON) {
        return 0;
    }

    float logLuminance = saturate((log2(luminance) - minLogLuminance) * invLogLuminanceRange);
    return uint(logLuminance * 254.0f + 1.0f);
}

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void computeHistogram(uint gi : SV_GroupIndex, uint3 dtid : SV_DispatchThreadID)
{
    histogramBins[gi] = 0;

    GroupMemoryBarrierWithGroupSync();

    if (all(dtid.xy < inputSize)) {
        float luminance = getLuminance(framebuffer[dtid.xy]);
        uint bin = getBinIndex(luminance);

        InterlockedAdd(histogramBins[bin], 1);
    }

    GroupMemoryBarrierWithGroupSync();

    histogram.InterlockedAdd(gi * 4, histogramBins[gi]);
}

groupshared float histogramBins2[NUM_BINS];

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void computeAverage(uint gi : SV_GroupIndex)
{
    float count = float(histogram.Load(gi * 4));
    histogramBins2[gi] = count * (float)gi;

    GroupMemoryBarrierWithGroupSync();

    for (uint sampleIdx = NUM_BINS / 2; sampleIdx > 0; sampleIdx /= 2) {
        if (gi < sampleIdx) {
            histogramBins2[gi] += histogramBins2[gi + sampleIdx];
        }

        GroupMemoryBarrierWithGroupSync();
    }

    if (gi == 0) {
        float weightedLogAverage = (histogramBins2[0].x / max(float(inputSize.x * inputSize.y) - count, 1.0f)) - 1.0f;
        float weightedAverageLuminance = exp2(((weightedLogAverage / 254.0f) * logLuminanceRange) + minLogLuminance);
        float luminanceLastFrame = averageLuminance[uint2(0, 0)];
        float adaptedLuminance = luminanceLastFrame + (weightedAverageLuminance - luminanceLastFrame) * (1.0f - exp(-deltaTime * tau));
        averageLuminance[uint2(0, 0)] = adaptedLuminance;
    }
}
