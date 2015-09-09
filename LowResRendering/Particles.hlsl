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
#include "SharedConstants.h"
#include "Shadows.hlsl"

//=================================================================================================
// Resources
//=================================================================================================
cbuffer ParticleConstants : register(b0)
{
    float4x4 ViewProjection;
    float3 CameraRight;
    float3 CameraUp;
    float Time;
    float3 SunDirectionWS;
    float3 SunIlluminance;
    float3 CameraPosWS;
}

StructuredBuffer<ParticleData> ParticleRenderBuffer : register(t0);
Texture2D<float4> ParticleTexture : register(t0);
Texture2DArray SunShadowMap : register(t1);
SamplerState LinearSampler : register(s0);
SamplerState EVSMSampler : register(s1);

struct VSOutput
{
    float4 Position : SV_Position;
    float3 PositionWS : POSITIONWS;
    float2 UV : UV;
    float Opacity : OPACITY;
    float Depth : DEPTH;
};

// Vertex shader for particle rendering
VSOutput ParticlesVS(in uint VertexIdx : SV_VertexID, in uint InstanceIdx : SV_InstanceID)
{
    ParticleData particle = ParticleRenderBuffer[InstanceIdx];

    float2 uv = float2(VertexIdx % 2, VertexIdx / 2);
    float2 posOffset = (uv - 0.5f) * float2(1.0f, -1.0f);

    float3 vtxPositionWS = particle.Position;

    float3 right = float3(1.0f, 0.0f, 0.0f);
    float3 up = float3(0.0f, 1.0f, 0.0f);
    if(BillboardParticles)
    {
        right = CameraRight;
        up = CameraUp;
    }

    vtxPositionWS += posOffset.x * right * particle.Size;
    vtxPositionWS += posOffset.y * up * particle.Size;

    VSOutput output;
    output.Position = mul(float4(vtxPositionWS, 1.0f), ViewProjection);
    output.PositionWS = vtxPositionWS;
    output.UV = uv;
    output.Opacity = particle.Opacity;
    output.Depth = output.Position.w;

    return output;
}

// Helper function that computes the intersection of a ray with the surface of a sphere, and returns
// the distance to the intersection. Assumes that the ray origin is *inside* the sphere.
float SphereDistance(in float3 rayPos, in float3 rayDir, in float3 spherePos, in float sphereRadius)
{
    float3 L = spherePos - rayPos;
    float tc = dot(L, rayDir);
    float d2 = (tc * tc) - (length(L) * length(L));
    float radius2 = sphereRadius * sphereRadius;
    float t1c = sqrt(radius2 - d2);
    return tc + t1c;
}

// Pixel shader for particle rendering
float4 ParticlesPS(in VSOutput input) : SV_Target0
{
    float4 textureSample = 1.0f;
    if(EnableParticleAlbedoMap)
        textureSample = ParticleTexture.Sample(LinearSampler, input.UV);

    float3 albedo = textureSample.xyz;
    float3 lighting = 0.0f;

    if(EnableSun)
    {
        // Compute shadowing from the scene geometry by sampling the shadow map
        float3 sunShadowVisibility = SunShadowVisibility(input.PositionWS, input.Depth, SunShadowMap, EVSMSampler);

        // Fake self-shadowing: compute the distance from the pixel position to the edge of particle emitter
        // radius in the direction of the sun, and compute absorption for that distance.
        float3 sphereCenter = float3(EmitCenterX, EmitCenterY, EmitCenterZ);
        float sphereDist = SphereDistance(input.PositionWS, SunDirectionWS, sphereCenter, EmitRadius + 0.5f);
        sphereDist = max(sphereDist, 0.0f);
        float absorption = 1.0f;
        absorption /= (EmitRadius / 2.0f);
        absorption *= AbsorptionScale;
        float selfShadowing = saturate(exp(-absorption * sphereDist));

        lighting = SunIlluminance * sunShadowVisibility * selfShadowing;
    }

    // Simple ambient term
    lighting += 1000.0f;

    float3 color = albedo * lighting;
    color *= ExposureRangeScale;

    const float baseAlpha = 16.0f / 255.f;
    textureSample.a = saturate((textureSample.a - baseAlpha) / (1.0f - baseAlpha));
    textureSample.a *= 0.3f;

    float alpha = saturate(textureSample.a * input.Opacity);
    return float4(color, alpha);
}