//=================================================================================================
//
//  Low-Resolution Rendering Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code and content licensed under the MIT license
//
//=================================================================================================

#if _WINDOWS

#pragma once

typedef SampleFramework11::Float2 float2;
typedef SampleFramework11::Float3 float3;
typedef SampleFramework11::Float4 float4;

typedef uint32 uint;
typedef SampleFramework11::Uint2 uint2;
typedef SampleFramework11::Uint3 uint3;
typedef SampleFramework11::Uint4 uint4;

#endif

static const uint ReductionTGSize = 16;

struct ParticleData
{
    float3 Position;
    float Size;
    float Opacity;
    float Lifetime;
};