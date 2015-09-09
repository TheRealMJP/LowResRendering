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
#include "Shadows.hlsl"
#include <SH.hlsl>
#include "BRDF.hlsl"
#include "AppSettings.hlsl"

//=================================================================================================
// Constant buffers
//=================================================================================================
cbuffer VSConstants : register(b0)
{
    float4x4 World;
	float4x4 View;
    float4x4 WorldViewProjection;
}

cbuffer PSConstants : register(b0)
{
    float3 SunDirectionWS;
    float CosSunAngularRadius;
    float3 SunIlluminance;
    float SinSunAngularRadius;
    float3 CameraPosWS;
}

//=================================================================================================
// Resources
//=================================================================================================
Texture2D AlbedoMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2DArray SunShadowMap : register(t2);

SamplerState AnisoSampler : register(s0);
SamplerState EVSMSampler : register(s1);
SamplerState LinearSampler : register(s2);

//=================================================================================================
// Input/Output structs
//=================================================================================================
struct VSInput
{
    float3 PositionOS 		    : POSITION;
    float3 NormalOS 		    : NORMAL;
    float2 TexCoord 		    : TEXCOORD0;
	float3 TangentOS 		    : TANGENT;
	float3 BitangentOS		    : BITANGENT;
};

struct VSOutput
{
    float4 PositionCS 		    : SV_Position;

    float3 NormalWS 		    : NORMALWS;
    float3 PositionWS           : POSITIONWS;
    float DepthVS               : DEPTHVS;
	float3 TangentWS 		    : TANGENTWS;
	float3 BitangentWS 		    : BITANGENTWS;
	float2 TexCoord 		    : TEXCOORD;
};

struct PSInput
{
    float4 PositionSS 		    : SV_Position;

    float3 NormalWS 		    : NORMALWS;
    float3 PositionWS           : POSITIONWS;
    float DepthVS               : DEPTHVS;
    float3 TangentWS 		    : TANGENTWS;
	float3 BitangentWS 		    : BITANGENTWS;

    float2 TexCoord 		    : TEXCOORD;
};

//=================================================================================================
// Vertex Shader
//=================================================================================================
VSOutput VS(in VSInput input, in uint VertexID : SV_VertexID)
{
    VSOutput output;

    float3 positionOS = input.PositionOS;

    // Calc the world-space position
    output.PositionWS = mul(float4(positionOS, 1.0f), World).xyz;

    // Calc the clip-space position
    output.PositionCS = mul(float4(positionOS, 1.0f), WorldViewProjection);
    output.DepthVS = output.PositionCS.w;


	// Rotate the normal into world space
    output.NormalWS = normalize(mul(input.NormalOS, (float3x3)World));

	// Rotate the rest of the tangent frame into world space
	output.TangentWS = normalize(mul(input.TangentOS, (float3x3)World));
	output.BitangentWS = normalize(mul(input.BitangentOS, (float3x3)World));

    // Pass along the texture coordinates
    output.TexCoord = input.TexCoord;

    return output;
}

//-------------------------------------------------------------------------------------------------
// Calculates the lighting result for an analytical light source
//-------------------------------------------------------------------------------------------------
float3 CalcLighting(in float3 normal, in float3 lightDir, in float3 lightColor,
					in float3 diffuseAlbedo, in float3 specularAlbedo, in float roughness,
					in float3 positionWS, inout float3 irradiance)
{
    float3 lighting = diffuseAlbedo * (1.0f / 3.14159f);

    float3 view = normalize(CameraPosWS - positionWS);
    const float nDotL = saturate(dot(normal, lightDir));
    if(nDotL > 0.0f)
    {
        const float nDotV = saturate(dot(normal, view));
        float3 h = normalize(view + lightDir);

        float3 fresnel = Fresnel(specularAlbedo, h, lightDir);

        float specular = GGX_Specular(roughness, normal, h, view, lightDir);
        lighting += specular * fresnel;
    }

    irradiance += nDotL * lightColor;
    return lighting * nDotL * lightColor;
}

//=================================================================================================
// Pixel Shader
//=================================================================================================
float4 PS(in PSInput input) : SV_Target0
{
	float3 vtxNormal = normalize(input.NormalWS);
    float3 positionWS = input.PositionWS;

    float3 viewWS = normalize(CameraPosWS - positionWS);

    float3 normalWS = vtxNormal;

    float2 uv = input.TexCoord;

	float3 normalTS = float3(0, 0, 1);
	float3 tangentWS = normalize(input.TangentWS);
	float3 bitangentWS = normalize(input.BitangentWS);
	float3x3 tangentToWorld = float3x3(tangentWS, bitangentWS, normalWS);

    if(EnableNormalMaps)
    {
        // Sample the normal map, and convert the normal to world space
        normalTS.xy = NormalMap.Sample(AnisoSampler, uv).xy * 2.0f - 1.0f;
        normalTS.z = sqrt(1.0f - saturate(normalTS.x * normalTS.x + normalTS.y * normalTS.y));
        normalTS = lerp(float3(0, 0, 1), normalTS, NormalMapIntensity);
        normalWS = normalize(mul(normalTS, tangentToWorld));
    }

    float3 diffuseAlbedo = 1.0f;

    if(EnableAlbedoMaps)
        diffuseAlbedo = AlbedoMap.Sample(AnisoSampler, uv).xyz;

    diffuseAlbedo *= DiffuseIntensity;
    diffuseAlbedo *= (1.0f - SpecularIntensity);

    float depthVS = input.DepthVS;

    // Add in the primary directional light
    float3 lighting = 0.0f;

    const float roughness = Roughness * Roughness;

    if(EnableSun)
    {
        float3 sunIrradiance = 0.0f;
        float3 sunShadowVisibility = SunShadowVisibility(positionWS, depthVS, SunShadowMap, EVSMSampler);
        float3 sunDirection = SunDirectionWS;
        if(SunAreaLightApproximation)
        {
            float3 D = SunDirectionWS;
            float3 R = reflect(-viewWS, normalWS);
            float r = SinSunAngularRadius;
            float d = CosSunAngularRadius;
            float3 DDotR = dot(D, R);
            float3 S = R - DDotR * D;
            sunDirection = DDotR < d ? normalize(d * D + normalize(S) * r) : R;
        }
        lighting += CalcLighting(normalWS, sunDirection, SunIlluminance, diffuseAlbedo, SpecularIntensity,
                                 roughness, positionWS, sunIrradiance) * sunShadowVisibility;
    }

	// Add in the indirect
    float3 indirectDiffuse = 5000.0f / Pi;
    lighting += indirectDiffuse * diffuseAlbedo;

    lighting *= ExposureRangeScale;

    return clamp(float4(lighting, 1.0f), 0.0f, FP16Max);
}
