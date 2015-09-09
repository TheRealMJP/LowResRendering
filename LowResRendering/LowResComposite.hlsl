//=================================================================================================
//
//  Low-Resolution Rendering Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code and content licensed under the MIT license
//
//=================================================================================================

//=================================================================================================
// Includes
//=================================================================================================
#include <Constants.hlsl>
#include "AppSettings.hlsl"

//=================================================================================================
// Constants
//=================================================================================================
#ifndef MSAASamples_
    #define MSAASamples_ 1
#endif

#define MSAA_ (MSAASamples_ > 1)

cbuffer CompositeConstants : register(b0)
{
    float4x4 Projection;
    float2 LowResSize;
}

//=================================================================================================
// Resources
//=================================================================================================
Texture2DMS<float4> LowResTextureMSAA : register(t0);
Texture2D<float4> LowResTexture : register(t1);
Texture2D<float> LowResDepth : register(t2);
#if MSAA_
    Texture2DMS<float> FullResDepth : register(t3);
#else
    Texture2D<float> FullResDepth : register(t3);
#endif
SamplerState LinearSampler : register(s0);
SamplerState PointSampler : register(s1);

// MSAA resolve pixel shader used for "MSAA" low-resolution rendering mode
float4 LowResResolve(in float4 Position : SV_Position, in float2 UV : UV) : SV_Target0
{
    const uint NumLowResSamples = MSAASamples_ * 4;
    float4 firstSample = LowResTextureMSAA.Load(uint2(Position.xy), 0);
    float4 output = firstSample;
    float alphaDiff = 0.0f;

    [unroll]
    for(uint i = 1; i < NumLowResSamples; ++i)
    {
        float4 currSample = LowResTextureMSAA.Load(uint2(Position.xy), i);
        output += currSample;
        alphaDiff += abs(currSample.w - firstSample.w);
    }

    // Standard box-filter resolve
    output /= NumLowResSamples;
    alphaDiff /= NumLowResSamples;

    // Try to determine if there's a sub-pixel in this pixel by comparing the alpha values.
    // If it's over a certain threshold, we'll 'mark' this pixel by outputting -FP16Max.
    // We do this to make sure that we trip the alpha threshold test in the composite shader
    if(alphaDiff > ResolveSubPixelThreshold)
        output = -FP16Max;

    return output;
}

// Upscale + composite pixel shader used for "MSAA" low-resolution rendering mode
float4 LowResCompositeMSAA(in float4 Position : SV_Position, in float2 UV : UV,
                           in uint SampleIdx : SV_SampleIndex) : SV_Target0
{
    // Figure out which sub-sample corresponds to the high-resoution pixel that we're shading
    uint2 hiResPos = uint2(Position.xy);
    uint2 lowResPos = uint2(Position.xy / 2.0f);
    uint lowResSampleIdx = hiResPos.x % 2 + (hiResPos.y % 2) * 2;
    #if MSAA_
        lowResSampleIdx *= MSAASamples_;
        lowResSampleIdx += SampleIdx;
    #endif

    float4 msaaResult = LowResTextureMSAA.Load(lowResPos, lowResSampleIdx);
    float4 filteredResult = LowResTexture.SampleLevel(LinearSampler, UV, 0.0f);

    // By default, use the filtered result from sampling the resolved texture. However if there's a big difference
    // in the alpha values of the filtered result and the corresponding sub-sample from the original MSAA texture,
    // then we use the sub-sample result with no filtering. This different usually occurs due to the alpha
    // threshold test in the resolve, but can also occur where there's a large depth discontinuity in the 4 pixels
    // used for filtering.
    float4 result = filteredResult;
    if(msaaResult.a - filteredResult.a > CompositeSubPixelThreshold)
    {
        result = msaaResult;
        if(ShowMSAAEdges)
            result = float4(0.0f, FP16Max, 0.0f, 0.0f);
    }

    return result;
}

// Helper function for "Nearest-Depth" compositing shader.
void UpdateNearestSample(inout float minDist, inout float2 nearestUV, in float z, in float2 uv, in float fullResZ)
{
    float d = abs(z - fullResZ);
    if(d < minDist)
    {
        minDist = d;
        nearestUV = uv;
    }
}

// Upscale + composite pixel shader used for "Nearest-Depth" low-resolution rendering mode
float4 LowResCompositeNearestDepth(in float4 Position : SV_Position, in float2 UV : UV,
                                   in uint SampleIdx : SV_SampleIndex) : SV_Target0
{
    // Get the linear low-res and full-res depth values
    float4 lowResZW = LowResDepth.GatherRed(LinearSampler, UV);
    float4 lowResDepth = Projection._43 / (lowResZW - Projection._33);

    #if MSAA_
        float fullResZW = FullResDepth.Load(uint2(Position.xy), SampleIdx);
    #else
        float fullResZW = FullResDepth[uint2(Position.xy)];
    #endif
    float fullResDepth = Projection._43 / (fullResZW - Projection._33);

    // Find the best UV to use when upsampling (the one closest to the z-value)
    float minDist = 1.e8f;
    float2 lowResTexelSize = 1.0f / LowResSize;

    float2 uv00 = UV - 0.5f * lowResTexelSize;
    float2 nearestUV = uv00;
    float z00 = lowResDepth.w;
    UpdateNearestSample(minDist, nearestUV, z00, uv00, fullResDepth);

    float2 uv10 = float2(uv00.x + lowResTexelSize.x, uv00.y);
    float z10 = lowResDepth.z;
    UpdateNearestSample(minDist, nearestUV, z10, uv10, fullResDepth);

    float2 uv01 = float2(uv00.x, uv00.y + lowResTexelSize.y);
    float z01 = lowResDepth.x;
    UpdateNearestSample(minDist, nearestUV, z01, uv01, fullResDepth);

    float2 uv11 = float2(uv00.x + lowResTexelSize.x, uv00.y + lowResTexelSize.y);
    float z11 = lowResDepth.y;
    UpdateNearestSample(minDist, nearestUV, z11, uv11, fullResDepth);

    float4 output = 0.0f;

    [branch]
    if(abs(z00 - fullResDepth) < NearestDepthThreshold && abs(z10 - fullResDepth) < NearestDepthThreshold &&
       abs(z01 - fullResDepth) < NearestDepthThreshold && abs(z11 - fullResDepth) < NearestDepthThreshold)
    {
        output = LowResTexture.SampleLevel(LinearSampler, UV, 0.0f);
    }
    else
    {
        output = LowResTexture.SampleLevel(PointSampler, nearestUV, 0.0f);
    }

    return output;
}