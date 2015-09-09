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

#ifndef FilterType_
    #define FilterType_ 0
#endif

#define MSAA_ (MSAASamples_ > 1)

#if MSAASamples_ == 8
    static const float2 SampleOffsets[8] = {
        float2(0.580f, 0.298f),
        float2(0.419f, 0.698f),
        float2(0.819f, 0.580f),
        float2(0.298f, 0.180f),
        float2(0.180f, 0.819f),
        float2(0.058f, 0.419f),
        float2(0.698f, 0.941f),
        float2(0.941f, 0.058f),
    };
#elif MSAASamples_ == 4
    static const float2 SampleOffsets[4] = {
        float2(0.380f, 0.141f),
        float2(0.859f, 0.380f),
        float2(0.141f, 0.620f),
        float2(0.619f, 0.859f),
    };
#elif MSAASamples_ == 2
    static const float2 SampleOffsets[2] = {
        float2(0.741f, 0.741f),
        float2(0.258f, 0.258f),
    };
#else
    static const float2 SampleOffsets[1] = {
        float2(0.5f, 0.5f),
    };
#endif

//=================================================================================================
// Resources
//=================================================================================================
#if MSAA_
    Texture2DMS<float4> InputTexture : register(t0);
#else
    Texture2D<float4> InputTexture : register(t0);
#endif

cbuffer ResolveConstants : register(b0)
{
    int SampleRadius;
    float2 TextureSize;
}

float3 SampleTexture(in float2 samplePos, in uint subSampleIdx)
{
    #if MSAA_
        return InputTexture.Load(uint2(samplePos), subSampleIdx).xyz;
    #else
        return InputTexture[uint2(samplePos)].xyz;
    #endif
}

float FilterSmoothstep(in float x)
{
    return 1.0f - smoothstep(0.0f, 1.0f, x);
}

float Filter(in float x)
{
    return FilterSmoothstep(x);
}

struct VSOutput
{
    float4 Position : SV_Position;
    float2 UV : UV;
};

VSOutput ResolveVS(in uint VertexID : SV_VertexID)
{
    VSOutput output;

    if(VertexID == 0)
    {
        output.Position = float4(-1.0f, 1.0f, 1.0f, 1.0f);
        output.UV = float2(0.0f, 0.0f);
    }
    else if(VertexID == 1)
    {
        output.Position = float4(3.0f, 1.0f, 1.0f, 1.0f);
        output.UV = float2(2.0f, 0.0f);
    }
    else
    {
        output.Position = float4(-1.0f, -3.0f, 1.0f, 1.0f);
        output.UV = float2(0.0f, 2.0f);
    }

    return output;
}

float Luminance(in float3 clr)
{
    return dot(clr, float3(0.299f, 0.587f, 0.114f));
}

float4 ResolvePS(in float4 Position : SV_Position) : SV_Target0
{
    const bool InverseLuminanceFiltering = true;
    const bool UseExposureFiltering = true;
    const float ExposureFilterOffset = 2.0f;

    const float exposure = exp2(-16.0f) / ExposureRangeScale;

    float2 pixelPos = Position.xy;
    float3 sum = 0.0f;
    float totalWeight = 0.0f;

    for(int y = -SampleRadius; y <= SampleRadius; ++y)
    {
        for(int x = -SampleRadius; x <= SampleRadius; ++x)
        {
            float2 sampleOffset = float2(x, y);
            float2 samplePos = pixelPos + sampleOffset;
            samplePos = clamp(samplePos, 0, TextureSize - 1.0f);

            [unroll]
            for(uint subSampleIdx = 0; subSampleIdx < MSAASamples_; ++subSampleIdx)
            {
                sampleOffset += SampleOffsets[subSampleIdx].xy - 0.5f;

                float sampleDist = length(sampleOffset) / (FilterSize / 2.0f);

                [branch]
                if(sampleDist <= 1.0f)
                {
                    float3 sample = SampleTexture(samplePos, subSampleIdx);
                    sample = max(sample, 0.0f);

                    float weight = Filter(sampleDist);

                    float sampleLum = Luminance(sample);

                    if(UseExposureFiltering)
                        sampleLum *= exposure;

                    if(InverseLuminanceFiltering)
                        weight *= 1.0f / (1.0f + sampleLum);

                    sum += sample * weight;
                    totalWeight += weight;
                }
            }
        }
    }

    float3 output = sum / max(totalWeight, 0.00001f);
    output = max(output, 0.0f);

    return float4(output, 1.0f);
}