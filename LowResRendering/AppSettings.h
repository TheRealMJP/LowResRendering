#pragma once

#include <PCH.h>
#include <Settings.h>
#include <Graphics\GraphicsTypes.h>

using namespace SampleFramework11;

enum class SunDirectionTypes
{
    UnitVector = 0,
    HorizontalCoordSystem = 1,

    NumValues
};

typedef EnumSettingT<SunDirectionTypes> SunDirectionTypesSetting;

enum class MSAAModes
{
    MSAANone = 0,
    MSAA2x = 1,

    NumValues
};

typedef EnumSettingT<MSAAModes> MSAAModesSetting;

enum class LowResRenderModes
{
    MSAA = 0,
    NearestDepth = 1,

    NumValues
};

typedef EnumSettingT<LowResRenderModes> LowResRenderModesSetting;

namespace AppSettings
{
    static const float ExposureRangeScale = 0.0010f;
    static const float BaseSunSize = 0.2700f;
    static const int64 MaxParticles = 32768;

    extern BoolSetting EnableSun;
    extern BoolSetting SunAreaLightApproximation;
    extern ColorSetting SunTintColor;
    extern FloatSetting SunIntensityScale;
    extern FloatSetting SunSize;
    extern BoolSetting NormalizeSunIntensity;
    extern SunDirectionTypesSetting SunDirType;
    extern DirectionSetting SunDirection;
    extern FloatSetting SunAzimuth;
    extern FloatSetting SunElevation;
    extern FloatSetting Turbidity;
    extern ColorSetting GroundAlbedo;
    extern MSAAModesSetting MSAAMode;
    extern FloatSetting FilterSize;
    extern BoolSetting EnableAlbedoMaps;
    extern BoolSetting EnableNormalMaps;
    extern FloatSetting NormalMapIntensity;
    extern FloatSetting DiffuseIntensity;
    extern FloatSetting Roughness;
    extern FloatSetting SpecularIntensity;
    extern IntSetting NumParticles;
    extern FloatSetting EmitRadius;
    extern FloatSetting EmitCenterX;
    extern FloatSetting EmitCenterY;
    extern FloatSetting EmitCenterZ;
    extern FloatSetting RotationSpeed;
    extern FloatSetting AbsorptionScale;
    extern BoolSetting SortParticles;
    extern BoolSetting EnableParticleAlbedoMap;
    extern BoolSetting BillboardParticles;
    extern BoolSetting RenderLowRes;
    extern LowResRenderModesSetting LowResRenderMode;
    extern FloatSetting ResolveSubPixelThreshold;
    extern FloatSetting CompositeSubPixelThreshold;
    extern BoolSetting ProgrammableSamplePoints;
    extern FloatSetting NearestDepthThreshold;
    extern FloatSetting BloomExposure;
    extern FloatSetting BloomMagnitude;
    extern FloatSetting BloomBlurSigma;
    extern BoolSetting EnableVSync;
    extern Button TakeScreenshot;
    extern BoolSetting ShowMSAAEdges;

    struct AppSettingsCBuffer
    {
        bool32 EnableSun;
        bool32 SunAreaLightApproximation;
        Float4Align Float3 SunTintColor;
        float SunIntensityScale;
        float SunSize;
        int32 SunDirType;
        Float4Align Float3 SunDirection;
        float SunAzimuth;
        float SunElevation;
        int32 MSAAMode;
        float FilterSize;
        bool32 EnableAlbedoMaps;
        bool32 EnableNormalMaps;
        float NormalMapIntensity;
        float DiffuseIntensity;
        float Roughness;
        float SpecularIntensity;
        int32 NumParticles;
        float EmitRadius;
        float EmitCenterX;
        float EmitCenterY;
        float EmitCenterZ;
        float AbsorptionScale;
        bool32 EnableParticleAlbedoMap;
        bool32 BillboardParticles;
        bool32 RenderLowRes;
        int32 LowResRenderMode;
        float ResolveSubPixelThreshold;
        float CompositeSubPixelThreshold;
        float NearestDepthThreshold;
        float BloomExposure;
        float BloomMagnitude;
        float BloomBlurSigma;
        bool32 ShowMSAAEdges;
    };

    extern ConstantBuffer<AppSettingsCBuffer> CBuffer;

    void Initialize(ID3D11Device* device);
    void Update();
    void UpdateCBuffer(ID3D11DeviceContext* context);
};

// ================================================================================================

namespace AppSettings
{
    Float3 SunLuminance();
    Float3 SunIlluminance();

    inline bool HasSunDirChanged()
    {
        return AppSettings::SunDirType.Changed() ||
               AppSettings::SunDirection.Changed() ||
               AppSettings::SunAzimuth.Changed() ||
               AppSettings::SunElevation.Changed();
    }

    inline void UpdateHorizontalCoords()
    {
        Float3 sunDir = SunDirection.Value();
        SunElevation.SetValue(RadToDeg(asin(sunDir.y)));

        float rad = atan2(sunDir.z, sunDir.x);
        if(rad < 0.0f)
            rad = 2.0f * Pi + rad;

        float deg = RadToDeg(rad);
        SunAzimuth.SetValue(deg);
    }

    inline void UpdateUnitVector()
    {
        Float3 newDir;
        newDir.x = cos(DegToRad(SunAzimuth)) * cos(DegToRad(SunElevation));
        newDir.y = sin(DegToRad(SunElevation));
        newDir.z = sin(DegToRad(SunAzimuth)) * cos(DegToRad(SunElevation));
        SunDirection.SetValue(newDir);
    }

    inline uint32 NumMSAASamples(MSAAModes mode)
    {
        static const uint32 NumSamples[uint32(MSAAModes::NumValues)] = { 1, 2 };
        return NumSamples[uint32(mode)];
    }

    inline uint32 NumMSAASamples()
    {
        return NumMSAASamples(MSAAMode);
    }

    void UpdateUI();

    extern bool ProgrammableSamplePointsSupported;
}