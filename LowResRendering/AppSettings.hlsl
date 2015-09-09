cbuffer AppSettings : register(b7)
{
    bool EnableSun;
    bool SunAreaLightApproximation;
    float3 SunTintColor;
    float SunIntensityScale;
    float SunSize;
    int SunDirType;
    float3 SunDirection;
    float SunAzimuth;
    float SunElevation;
    int MSAAMode;
    float FilterSize;
    bool EnableAlbedoMaps;
    bool EnableNormalMaps;
    float NormalMapIntensity;
    float DiffuseIntensity;
    float Roughness;
    float SpecularIntensity;
    int NumParticles;
    float EmitRadius;
    float EmitCenterX;
    float EmitCenterY;
    float EmitCenterZ;
    float AbsorptionScale;
    bool EnableParticleAlbedoMap;
    bool BillboardParticles;
    bool RenderLowRes;
    int LowResRenderMode;
    float ResolveSubPixelThreshold;
    float CompositeSubPixelThreshold;
    float NearestDepthThreshold;
    float BloomExposure;
    float BloomMagnitude;
    float BloomBlurSigma;
    bool ShowMSAAEdges;
}

static const int SunDirectionTypes_UnitVector = 0;
static const int SunDirectionTypes_HorizontalCoordSystem = 1;

static const int MSAAModes_MSAANone = 0;
static const int MSAAModes_MSAA2x = 1;

static const int LowResRenderModes_MSAA = 0;
static const int LowResRenderModes_NearestDepth = 1;

static const float ExposureRangeScale = 0.0010f;
static const float BaseSunSize = 0.2700f;
static const int MaxParticles = 32768;
