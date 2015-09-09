#include <PCH.h>
#include <TwHelper.h>
#include "AppSettings.h"

using namespace SampleFramework11;

static const char* SunDirectionTypesLabels[2] =
{
    "Unit Vector",
    "Horizontal Coordinate System",
};

static const char* MSAAModesLabels[2] =
{
    "None",
    "2x",
};

static const char* LowResRenderModesLabels[2] =
{
    "MSAA",
    "Nearest-Depth",
};

namespace AppSettings
{
    BoolSetting EnableSun;
    BoolSetting SunAreaLightApproximation;
    ColorSetting SunTintColor;
    FloatSetting SunIntensityScale;
    FloatSetting SunSize;
    BoolSetting NormalizeSunIntensity;
    SunDirectionTypesSetting SunDirType;
    DirectionSetting SunDirection;
    FloatSetting SunAzimuth;
    FloatSetting SunElevation;
    FloatSetting Turbidity;
    ColorSetting GroundAlbedo;
    MSAAModesSetting MSAAMode;
    FloatSetting FilterSize;
    BoolSetting EnableAlbedoMaps;
    BoolSetting EnableNormalMaps;
    FloatSetting NormalMapIntensity;
    FloatSetting DiffuseIntensity;
    FloatSetting Roughness;
    FloatSetting SpecularIntensity;
    IntSetting NumParticles;
    FloatSetting EmitRadius;
    FloatSetting EmitCenterX;
    FloatSetting EmitCenterY;
    FloatSetting EmitCenterZ;
    FloatSetting RotationSpeed;
    FloatSetting AbsorptionScale;
    BoolSetting SortParticles;
    BoolSetting EnableParticleAlbedoMap;
    BoolSetting BillboardParticles;
    BoolSetting RenderLowRes;
    LowResRenderModesSetting LowResRenderMode;
    FloatSetting ResolveSubPixelThreshold;
    FloatSetting CompositeSubPixelThreshold;
    BoolSetting ProgrammableSamplePoints;
    FloatSetting NearestDepthThreshold;
    FloatSetting BloomExposure;
    FloatSetting BloomMagnitude;
    FloatSetting BloomBlurSigma;
    BoolSetting EnableVSync;
    Button TakeScreenshot;
    BoolSetting ShowMSAAEdges;

    ConstantBuffer<AppSettingsCBuffer> CBuffer;

    void Initialize(ID3D11Device* device)
    {
        TwBar* tweakBar = Settings.TweakBar();

        EnableSun.Initialize(tweakBar, "EnableSun", "Sun Light", "Enable Sun", "Enables the sun light", true);
        Settings.AddSetting(&EnableSun);

        SunAreaLightApproximation.Initialize(tweakBar, "SunAreaLightApproximation", "Sun Light", "Sun Area Light Approximation", "Controls whether the sun is treated as a disc area light in the real-time shader", true);
        Settings.AddSetting(&SunAreaLightApproximation);

        SunTintColor.Initialize(tweakBar, "SunTintColor", "Sun Light", "Sun Tint Color", "The color of the sun", Float3(1.0000f, 1.0000f, 1.0000f), false, -340282300000000000000000000000000000000.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ColorUnit::None);
        Settings.AddSetting(&SunTintColor);

        SunIntensityScale.Initialize(tweakBar, "SunIntensityScale", "Sun Light", "Sun Intensity Scale", "Scale the intensity of the sun", 1.0000f, 0.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SunIntensityScale);

        SunSize.Initialize(tweakBar, "SunSize", "Sun Light", "Sun Size", "Angular radius of the sun in degrees", 0.2700f, 0.0100f, 340282300000000000000000000000000000000.0000f, 0.0010f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SunSize);

        NormalizeSunIntensity.Initialize(tweakBar, "NormalizeSunIntensity", "Sun Light", "Normalize Sun Intensity", "", false);
        Settings.AddSetting(&NormalizeSunIntensity);

        SunDirType.Initialize(tweakBar, "SunDirType", "Sun Light", "Sun Dir Type", "Input direction type for the sun", SunDirectionTypes::UnitVector, 2, SunDirectionTypesLabels);
        Settings.AddSetting(&SunDirType);

        SunDirection.Initialize(tweakBar, "SunDirection", "Sun Light", "Sun Direction", "Director of the sun", Float3(-0.7500f, 0.9770f, -0.4000f));
        Settings.AddSetting(&SunDirection);

        SunAzimuth.Initialize(tweakBar, "SunAzimuth", "Sun Light", "Sun Azimuth", "Angle around the horizon", 0.0000f, 0.0000f, 360.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SunAzimuth);

        SunElevation.Initialize(tweakBar, "SunElevation", "Sun Light", "Sun Elevation", "Elevation of sun from ground. 0 degrees is aligned on the horizon while 90 degrees is directly overhead", 0.0000f, 0.0000f, 90.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SunElevation);

        Turbidity.Initialize(tweakBar, "Turbidity", "Sky", "Turbidity", "Atmospheric turbidity (thickness) uses for procedural sun and sky model", 2.0000f, 1.0000f, 10.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&Turbidity);

        GroundAlbedo.Initialize(tweakBar, "GroundAlbedo", "Sky", "Ground Albedo", "Ground albedo color used for procedural sun and sky model", Float3(0.5000f, 0.5000f, 0.5000f), false, -340282300000000000000000000000000000000.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ColorUnit::None);
        Settings.AddSetting(&GroundAlbedo);

        MSAAMode.Initialize(tweakBar, "MSAAMode", "Anti Aliasing", "MSAAMode", "MSAA mode to use for full-resolution rendering", MSAAModes::MSAANone, 2, MSAAModesLabels);
        Settings.AddSetting(&MSAAMode);

        FilterSize.Initialize(tweakBar, "FilterSize", "Anti Aliasing", "Filter Size", "Filter radius for MSAA resolve", 2.0000f, 0.0000f, 6.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&FilterSize);

        EnableAlbedoMaps.Initialize(tweakBar, "EnableAlbedoMaps", "Scene", "Enable Albedo Maps", "Enables albedo maps", true);
        Settings.AddSetting(&EnableAlbedoMaps);

        EnableNormalMaps.Initialize(tweakBar, "EnableNormalMaps", "Scene", "Enable Normal Maps", "Enables normal maps", true);
        Settings.AddSetting(&EnableNormalMaps);

        NormalMapIntensity.Initialize(tweakBar, "NormalMapIntensity", "Scene", "Normal Map Intensity", "Intensity of the normal map", 0.5000f, 0.0000f, 1.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&NormalMapIntensity);

        DiffuseIntensity.Initialize(tweakBar, "DiffuseIntensity", "Scene", "Diffuse Intensity", "Diffuse albedo intensity parameter for the material", 0.7500f, 0.0000f, 1.0000f, 0.0010f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&DiffuseIntensity);

        Roughness.Initialize(tweakBar, "Roughness", "Scene", "Specular Roughness", "Specular roughness parameter for the material", 0.2500f, 0.0010f, 1.0000f, 0.0010f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&Roughness);

        SpecularIntensity.Initialize(tweakBar, "SpecularIntensity", "Scene", "Specular Intensity", "Specular intensity parameter for the material", 0.0400f, 0.0000f, 1.0000f, 0.0010f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&SpecularIntensity);

        NumParticles.Initialize(tweakBar, "NumParticles", "Particles", "Num Particles (x1024)", "The number of particles to render, in increments of 1024", 8, 0, 32);
        Settings.AddSetting(&NumParticles);

        EmitRadius.Initialize(tweakBar, "EmitRadius", "Particles", "Emit Radius", "The radius in which to emit particles", 2.0000f, 0.0100f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&EmitRadius);

        EmitCenterX.Initialize(tweakBar, "EmitCenterX", "Particles", "Emit Center X", "The X coordinate of the point from which to emit particles", 0.0000f, -340282300000000000000000000000000000000.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&EmitCenterX);

        EmitCenterY.Initialize(tweakBar, "EmitCenterY", "Particles", "Emit Center Y", "The Y coordinate of the point from which to emit particles", 2.5000f, -340282300000000000000000000000000000000.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&EmitCenterY);

        EmitCenterZ.Initialize(tweakBar, "EmitCenterZ", "Particles", "Emit Center Z", "The Z coordinate of the point from which to emit particles", 0.0000f, -340282300000000000000000000000000000000.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&EmitCenterZ);

        RotationSpeed.Initialize(tweakBar, "RotationSpeed", "Particles", "Rotation Speed", "Controls how fast to rotate the particles around the emitter center", 0.5000f, 0.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&RotationSpeed);

        AbsorptionScale.Initialize(tweakBar, "AbsorptionScale", "Particles", "Absorption Scale", "Scaled the absorption coefficient used for particle self-shadowing", 1.0000f, 0.0000f, 340282300000000000000000000000000000000.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&AbsorptionScale);

        SortParticles.Initialize(tweakBar, "SortParticles", "Particles", "Sort Particles", "Enables sorting each particle by their depth", true);
        Settings.AddSetting(&SortParticles);

        EnableParticleAlbedoMap.Initialize(tweakBar, "EnableParticleAlbedoMap", "Particles", "Enable Particle Albedo Map", "Enables or disables sampling an albedo map in the particle pixel shader", true);
        Settings.AddSetting(&EnableParticleAlbedoMap);

        BillboardParticles.Initialize(tweakBar, "BillboardParticles", "Particles", "Billboard Particles", "Enables or disabled billboarding of particles towards the camera", true);
        Settings.AddSetting(&BillboardParticles);

        RenderLowRes.Initialize(tweakBar, "RenderLowRes", "Particles", "Render Low-Res", "Renders the particles at half resolution", true);
        Settings.AddSetting(&RenderLowRes);

        LowResRenderMode.Initialize(tweakBar, "LowResRenderMode", "Particles", "Low-Res Render Mode", "Specifies the technique to use for upscaling particles from half resolution", LowResRenderModes::MSAA, 2, LowResRenderModesLabels);
        Settings.AddSetting(&LowResRenderMode);

        ResolveSubPixelThreshold.Initialize(tweakBar, "ResolveSubPixelThreshold", "Particles", "Resolve Sub-Pixel Threshold", "Threshold used during low-resolution resolve for determining pixels containing sub-pixel edges", 0.0250f, 0.0000f, 1.0000f, 0.0010f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&ResolveSubPixelThreshold);

        CompositeSubPixelThreshold.Initialize(tweakBar, "CompositeSubPixelThreshold", "Particles", "Composite Sub-Pixel Threshold", "Threshold used during low-resolution composite for determining pixels containing sub-pixel edges", 0.1000f, 0.0000f, 1.0000f, 0.0010f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&CompositeSubPixelThreshold);

        ProgrammableSamplePoints.Initialize(tweakBar, "ProgrammableSamplePoints", "Particles", "Programmable Sample Points", "Use programmable sample positions when rendering low-resolution particles with 'MSAA' mode", true);
        Settings.AddSetting(&ProgrammableSamplePoints);
        ProgrammableSamplePoints.SetVisible(false);

        NearestDepthThreshold.Initialize(tweakBar, "NearestDepthThreshold", "Particles", "Nearest-Depth Threshold", "Depth threshold to use for nearest-depth upsampling", 0.2500f, 0.0000f, 100.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&NearestDepthThreshold);

        BloomExposure.Initialize(tweakBar, "BloomExposure", "Post Processing", "Bloom Exposure Offset", "Exposure offset applied to generate the input of the bloom pass", -4.0000f, -10.0000f, 0.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BloomExposure);

        BloomMagnitude.Initialize(tweakBar, "BloomMagnitude", "Post Processing", "Bloom Magnitude", "Scale factor applied to the bloom results when combined with tone-mapped result", 1.0000f, 0.0000f, 2.0000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BloomMagnitude);

        BloomBlurSigma.Initialize(tweakBar, "BloomBlurSigma", "Post Processing", "Bloom Blur Sigma", "Sigma parameter of the Gaussian filter used in the bloom pass", 2.5000f, 0.5000f, 2.5000f, 0.0100f, ConversionMode::None, 1.0000f);
        Settings.AddSetting(&BloomBlurSigma);

        EnableVSync.Initialize(tweakBar, "EnableVSync", "Debug", "Enable VSync", "Enables or disables vertical sync during Present", true);
        Settings.AddSetting(&EnableVSync);

        TakeScreenshot.Initialize(tweakBar, "TakeScreenshot", "Debug", "Take Screenshot", "Captures the screen output (before HUD rendering), and saves it to a file");
        Settings.AddSetting(&TakeScreenshot);

        ShowMSAAEdges.Initialize(tweakBar, "ShowMSAAEdges", "Debug", "Show MSAAEdges", "When using MSAA low-res render mode, shows pixels that use subpixel data", false);
        Settings.AddSetting(&ShowMSAAEdges);

        TwHelper::SetOpened(tweakBar, "Sun Light", false);

        TwHelper::SetOpened(tweakBar, "Sky", false);

        TwHelper::SetOpened(tweakBar, "Anti Aliasing", false);

        TwHelper::SetOpened(tweakBar, "Scene", false);

        TwHelper::SetOpened(tweakBar, "Particles", true);

        TwHelper::SetOpened(tweakBar, "Post Processing", false);

        TwHelper::SetOpened(tweakBar, "Debug", true);

        CBuffer.Initialize(device);
    }

    void Update()
    {
    }

    void UpdateCBuffer(ID3D11DeviceContext* context)
    {
        CBuffer.Data.EnableSun = EnableSun;
        CBuffer.Data.SunAreaLightApproximation = SunAreaLightApproximation;
        CBuffer.Data.SunTintColor = SunTintColor;
        CBuffer.Data.SunIntensityScale = SunIntensityScale;
        CBuffer.Data.SunSize = SunSize;
        CBuffer.Data.SunDirType = SunDirType;
        CBuffer.Data.SunDirection = SunDirection;
        CBuffer.Data.SunAzimuth = SunAzimuth;
        CBuffer.Data.SunElevation = SunElevation;
        CBuffer.Data.MSAAMode = MSAAMode;
        CBuffer.Data.FilterSize = FilterSize;
        CBuffer.Data.EnableAlbedoMaps = EnableAlbedoMaps;
        CBuffer.Data.EnableNormalMaps = EnableNormalMaps;
        CBuffer.Data.NormalMapIntensity = NormalMapIntensity;
        CBuffer.Data.DiffuseIntensity = DiffuseIntensity;
        CBuffer.Data.Roughness = Roughness;
        CBuffer.Data.SpecularIntensity = SpecularIntensity;
        CBuffer.Data.NumParticles = NumParticles;
        CBuffer.Data.EmitRadius = EmitRadius;
        CBuffer.Data.EmitCenterX = EmitCenterX;
        CBuffer.Data.EmitCenterY = EmitCenterY;
        CBuffer.Data.EmitCenterZ = EmitCenterZ;
        CBuffer.Data.AbsorptionScale = AbsorptionScale;
        CBuffer.Data.EnableParticleAlbedoMap = EnableParticleAlbedoMap;
        CBuffer.Data.BillboardParticles = BillboardParticles;
        CBuffer.Data.RenderLowRes = RenderLowRes;
        CBuffer.Data.LowResRenderMode = LowResRenderMode;
        CBuffer.Data.ResolveSubPixelThreshold = ResolveSubPixelThreshold;
        CBuffer.Data.CompositeSubPixelThreshold = CompositeSubPixelThreshold;
        CBuffer.Data.NearestDepthThreshold = NearestDepthThreshold;
        CBuffer.Data.BloomExposure = BloomExposure;
        CBuffer.Data.BloomMagnitude = BloomMagnitude;
        CBuffer.Data.BloomBlurSigma = BloomBlurSigma;
        CBuffer.Data.ShowMSAAEdges = ShowMSAAEdges;

        CBuffer.ApplyChanges(context);
        CBuffer.SetVS(context, 7);
        CBuffer.SetHS(context, 7);
        CBuffer.SetDS(context, 7);
        CBuffer.SetGS(context, 7);
        CBuffer.SetPS(context, 7);
        CBuffer.SetCS(context, 7);
    }
}

// ================================================================================================

#include <Graphics\\Spectrum.h>
#include <Graphics\\Sampling.h>

namespace AppSettings
{
    // Returns the result of performing a irradiance/illuminance integral over the portion
    // of the hemisphere covered by a region with angular radius = theta
    static float IlluminanceIntegral(float theta)
    {
        float cosTheta = std::cos(theta);
        return Pi * (1.0f - (cosTheta * cosTheta));
    }

    Float3 SunLuminance(bool& cached)
    {
        Float3 sunDirection = AppSettings::SunDirection;
        sunDirection.y = Saturate(sunDirection.y);
        sunDirection = Float3::Normalize(sunDirection);
        const float turbidity = Clamp(AppSettings::Turbidity.Value(), 1.0f, 32.0f);
        const float intensityScale = AppSettings::SunIntensityScale;
        const Float3 tintColor = AppSettings::SunTintColor;
        const bool32 normalizeIntensity = AppSettings::NormalizeSunIntensity;
        const float sunSize = AppSettings::SunSize;

        static float turbidityCache = 2.0f;
        static Float3 sunDirectionCache = Float3(-0.579149902f, 0.754439294f, -0.308879942f);
        static Float3 luminanceCache = Float3(1.61212531e+009f, 1.36822630e+009f, 1.07235315e+009f);
        static Float3 sunTintCache = Float3(1.0f, 1.0f, 1.0f);
        static float sunIntensityCache = 1.0f;
        static bool32 normalizeCache = false;
        static float sunSizeCache = AppSettings::BaseSunSize;

        if(turbidityCache == turbidity && sunDirection == sunDirectionCache
            && intensityScale == sunIntensityCache && tintColor == sunTintCache
            && normalizeCache == normalizeIntensity && sunSize == sunSizeCache)
        {
            cached = true;
            return luminanceCache;
        }

        cached = false;

        float thetaS = std::acos(1.0f - sunDirection.y);
        float elevation = Pi_2 - thetaS;

        // Get the sun's luminance, then apply tint and scale factors
        Float3 sunLuminance;

        // For now, we'll compute an average luminance value from Hosek solar radiance model, even though
        // we could compute illuminance directly while we're sampling the disk
        SampledSpectrum groundAlbedoSpectrum = SampledSpectrum::FromRGB(GroundAlbedo);
        SampledSpectrum solarRadiance;

        const uint64 NumDiscSamples = 4;
        for(uint64 x = 0; x < NumDiscSamples; ++x)
        {
            for(uint64 y = 0; y < NumDiscSamples; ++y)
            {
                float u = (x + 0.5f) / NumDiscSamples;
                float v = (y + 0.5f) / NumDiscSamples;
                Float2 discSamplePos = SquareToConcentricDiskMapping(u, v);

                float theta = elevation + discSamplePos.y * DegToRad(AppSettings::BaseSunSize);
                float gamma = discSamplePos.x * DegToRad(AppSettings::BaseSunSize);

                for(int32 i = 0; i < NumSpectralSamples; ++i)
                {
                    ArHosekSkyModelState* skyState = arhosekskymodelstate_alloc_init(elevation, turbidity, groundAlbedoSpectrum[i]);
                    float wavelength = Lerp(float(SampledLambdaStart), float(SampledLambdaEnd), i / float(NumSpectralSamples));

                    solarRadiance[i] = float(arhosekskymodel_solar_radiance(skyState, theta, gamma, wavelength));

                    arhosekskymodelstate_free(skyState);
                    skyState = nullptr;
                }

                Float3 sampleRadiance = solarRadiance.ToRGB();
                sunLuminance += sampleRadiance;
            }
        }

        // Account for luminous efficiency, coordinate system scaling, and sample averaging
        sunLuminance *= 683.0f * 100.0f * (1.0f / NumDiscSamples) * (1.0f / NumDiscSamples);

        sunLuminance = sunLuminance * tintColor;
        sunLuminance = sunLuminance * intensityScale;

        if(normalizeIntensity)
        {
            // Normalize so that the intensity stays the same even when the sun is bigger or smaller
            const float baseIntegral = IlluminanceIntegral(DegToRad(AppSettings::BaseSunSize));
            const float currIntegral = IlluminanceIntegral(DegToRad(AppSettings::SunSize));
            sunLuminance *= (baseIntegral / currIntegral);
        }

        turbidityCache = turbidity;
        sunDirectionCache = sunDirection;
        luminanceCache = sunLuminance;
        sunIntensityCache = intensityScale;
        sunTintCache = tintColor;
        normalizeCache = normalizeIntensity;
        sunSizeCache = sunSize;

        return sunLuminance;
    }

    Float3 SunLuminance()
    {
        bool cached = false;
        return SunLuminance(cached);
    }

    Float3 SunIlluminance()
    {
        Float3 sunLuminance = SunLuminance();

        // Compute partial integral over the hemisphere in order to compute illuminance
        float theta = DegToRad(AppSettings::SunSize);
        float integralFactor = IlluminanceIntegral(theta);

        return sunLuminance * integralFactor;
    }

    bool ProgrammableSamplePointsSupported = false;

    void UpdateUI()
    {
        bool enableSunDirection = SunDirType == SunDirectionTypes::UnitVector;
        bool enableHorizCoord = SunDirType == SunDirectionTypes::HorizontalCoordSystem;

        SunDirection.SetVisible(enableSunDirection);
        SunAzimuth.SetVisible(enableHorizCoord);
        SunElevation.SetVisible(enableHorizCoord);

        if(AppSettings::HasSunDirChanged())
        {
            if(SunDirType == SunDirectionTypes::UnitVector)
            {
                UpdateHorizontalCoords();
            }
            else if(SunDirType == SunDirectionTypes::HorizontalCoordSystem)
            {
                UpdateUnitVector();
            }
        }

        NearestDepthThreshold.SetVisible(LowResRenderMode == LowResRenderModes::NearestDepth);
        ResolveSubPixelThreshold.SetVisible(LowResRenderMode == LowResRenderModes::MSAA);
        CompositeSubPixelThreshold.SetVisible(LowResRenderMode == LowResRenderModes::MSAA);
        ShowMSAAEdges.SetVisible(LowResRenderMode == LowResRenderModes::MSAA);
        ProgrammableSamplePoints.SetVisible(ProgrammableSamplePointsSupported && LowResRenderMode == LowResRenderModes::MSAA);
    }
}