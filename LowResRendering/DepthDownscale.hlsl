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

//=================================================================================================
// Resources
//=================================================================================================
#if MSAA_
    Texture2DMS<float> FullResDepth : register(t0);
#else
    Texture2D<float> FullResDepth : register(t0);
#endif

// Downscales the high-resolution depth buffer to a half-resolution MSAA depth buffer. When
// we do this we're not *really* downsampling, we're just stuffing the depth values into corresponding
// depth subsamples. This effectively lets us perform the depth test at full resolution, while shading
// at half resolution. If we were on a platform where it were possible to alias a depth buffer as a
// lower-resolution MSAA depth buffer, then we wouldn't need this step.
float DepthDownscaleMSAA(in float4 Position : SV_Position, in uint SampleIdx : SV_SampleIndex) : SV_Depth
{
    float output = 1.0f;

    #if MSAASamples_ == 1
        const uint NumOutputSamples = 4;
        const float2 LoadOffsets[NumOutputSamples] =
        {
            float2(-0.5f, -0.5f),
            float2( 0.5f, -0.5f),
            float2(-0.5f,  0.5f),
            float2( 0.5f,  0.5f),
        };

        float2 loadPos = Position.xy * 2.0f + LoadOffsets[min(SampleIdx, NumOutputSamples - 1)];
        output = FullResDepth[uint2(loadPos)];
    #elif MSAASamples_ == 2
        const uint NumOutputSamples = 8;

        const float2 LoadOffsets[NumOutputSamples] =
        {
            float2(-0.5f, -0.5f),
            float2(-0.5f, -0.5f),

            float2( 0.5f, -0.5f),
            float2( 0.5f, -0.5f),

            float2(-0.5f,  0.5f),
            float2(-0.5f,  0.5f),

            float2( 0.5f,  0.5f),
            float2( 0.5f,  0.5f),
        };

        float2 loadPos = Position.xy * 2.0f + LoadOffsets[min(SampleIdx, NumOutputSamples - 1)];
        output = FullResDepth.Load(uint2(loadPos), SampleIdx % 2);
    #else
        #error "Not implemented"
    #endif

    return output;
}

// Downscales the depth buffer to half resolution using point sampling. Used for "Nearest-Depth"
// low-res rendering mode.
float DepthDownscale(in float4 Position : SV_Position) : SV_Depth
{
    #if MSAA_
        return FullResDepth.Load(uint2(Position.xy) * 2, 0);
    #else
        return FullResDepth[uint2(Position.xy * 2)];
    #endif
}