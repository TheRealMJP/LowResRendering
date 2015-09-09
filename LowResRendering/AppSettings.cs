enum SunDirectionTypes
{
    [EnumLabel("Unit Vector")]
    UnitVector,

    [EnumLabel("Horizontal Coordinate System")]
    HorizontalCoordSystem,
}

enum LightUnits
{
    Luminance = 0,
    Illuminance = 1,
    LuminousPower = 2,
    EV100 = 3
}

enum Scenes
{
    Box = 0,
}

enum MSAAModes
{
    [EnumLabel("None")]
    MSAANone = 0,

    [EnumLabel("2x")]
    MSAA2x,
}

enum LowResRenderModes
{
    [EnumLabel("MSAA")]
    MSAA,

    [EnumLabel("Nearest-Depth")]
    NearestDepth,
}

public class Settings
{
    // Scale factor for bringing lighting values down into a range suitable for fp16 storage.
    // Equal to 2^-10.
    const float ExposureRangeScale = 0.0009765625f;

    const float BaseSunSize = 0.27f;

    [ExpandGroup(false)]
    public class SunLight
    {
        [HelpText("Enables the sun light")]
        bool EnableSun = true;

        [HelpText("Controls whether the sun is treated as a disc area light in the real-time shader")]
        bool SunAreaLightApproximation = true;

        [HelpText("The color of the sun")]
        Color SunTintColor = new Color(1.0f, 1.0f, 1.0f, 1.0f);

        [HelpText("Scale the intensity of the sun")]
        [MinValue(0.0f)]
        float SunIntensityScale = 1.0f;

        [HelpText("Angular radius of the sun in degrees")]
        [MinValue(0.01f)]
        [StepSize(0.001f)]
        float SunSize = BaseSunSize;

        [UseAsShaderConstant(false)]
        bool NormalizeSunIntensity = false;

        [HelpText("Input direction type for the sun")]
        SunDirectionTypes SunDirType;

        [HelpText("Director of the sun")]
        Direction SunDirection = new Direction(-0.75f, 0.977f, -0.4f);

        [HelpText("Angle around the horizon")]
        [MinValue(0.0f)]
        [MaxValue(360.0f)]
        float SunAzimuth;

        [HelpText("Elevation of sun from ground. 0 degrees is aligned on the horizon while 90 degrees is directly overhead")]
        [MinValue(0.0f)]
        [MaxValue(90.0f)]
        float SunElevation;
    }

    [ExpandGroup(false)]
    public class Sky
    {
        [MinValue(1.0f)]
        [MaxValue(10.0f)]
        [UseAsShaderConstant(false)]
        [HelpText("Atmospheric turbidity (thickness) uses for procedural sun and sky model")]
        float Turbidity = 2.0f;

        [HDR(false)]
        [UseAsShaderConstant(false)]
        [HelpText("Ground albedo color used for procedural sun and sky model")]
        Color GroundAlbedo = new Color(0.5f, 0.5f, 0.5f);
    }

    [ExpandGroup(false)]
    public class AntiAliasing
    {
        [HelpText("MSAA mode to use for full-resolution rendering")]
        MSAAModes MSAAMode = MSAAModes.MSAANone;

        [MinValue(0.0f)]
        [MaxValue(6.0f)]
        [StepSize(0.01f)]
        [HelpText("Filter radius for MSAA resolve")]
        float FilterSize = 2.0f;
    }

    [ExpandGroup(false)]
    public class Scene
    {
        [DisplayName("Enable Albedo Maps")]
        [HelpText("Enables albedo maps")]
        bool EnableAlbedoMaps = true;

        [DisplayName("Enable Normal Maps")]
        [HelpText("Enables normal maps")]
        bool EnableNormalMaps = true;

        [DisplayName("Normal Map Intensity")]
        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        [StepSize(0.01f)]
        [HelpText("Intensity of the normal map")]
        float NormalMapIntensity = 0.5f;

        [DisplayName("Diffuse Intensity")]
        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        [StepSize(0.001f)]
        [HelpText("Diffuse albedo intensity parameter for the material")]
        float DiffuseIntensity = 0.75f;

        [DisplayName("Specular Roughness")]
        [MinValue(0.001f)]
        [MaxValue(1.0f)]
        [StepSize(0.001f)]
        [HelpText("Specular roughness parameter for the material")]
        float Roughness = 0.25f;

        [DisplayName("Specular Intensity")]
        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        [StepSize(0.001f)]
        [HelpText("Specular intensity parameter for the material")]
        float SpecularIntensity = 0.04f;
    }

    const int MaxParticles = 1024 * 32;

    [ExpandGroup(true)]
    public class Particles
    {
        [MinValue(0)]
        [MaxValue(MaxParticles / 1024)]
        [HelpText("The number of particles to render, in increments of 1024")]
        [DisplayName("Num Particles (x1024)")]
        int NumParticles = 8;

        [MinValue(0.01f)]
        [StepSize(0.01f)]
        [HelpText("The radius in which to emit particles")]
        float EmitRadius = 2.0f;

        [StepSize(0.01f)]
        [HelpText("The X coordinate of the point from which to emit particles")]
        float EmitCenterX = 0.0f;

        [StepSize(0.01f)]
        [HelpText("The Y coordinate of the point from which to emit particles")]
        float EmitCenterY = 2.5f;

        [StepSize(0.01f)]
        [HelpText("The Z coordinate of the point from which to emit particles")]
        float EmitCenterZ = 0.0f;

        [MinValue(0.0f)]
        [UseAsShaderConstant(false)]
        [HelpText("Controls how fast to rotate the particles around the emitter center")]
        float RotationSpeed = 0.5f;

        [MinValue(0.0f)]
        [HelpText("Scaled the absorption coefficient used for particle self-shadowing")]
        float AbsorptionScale = 1.0f;

        [UseAsShaderConstant(false)]
        [HelpText("Enables sorting each particle by their depth")]
        bool SortParticles = true;

        [HelpText("Enables or disables sampling an albedo map in the particle pixel shader")]
        bool EnableParticleAlbedoMap = true;

        [HelpText("Enables or disabled billboarding of particles towards the camera")]
        bool BillboardParticles = true;

        [HelpText("Renders the particles at half resolution")]
        [DisplayName("Render Low-Res")]
        bool RenderLowRes = true;

        [HelpText("Specifies the technique to use for upscaling particles from half resolution")]
        [DisplayName("Low-Res Render Mode")]
        LowResRenderModes LowResRenderMode = LowResRenderModes.MSAA;

        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        [StepSize(0.001f)]
        [HelpText("Threshold used during low-resolution resolve for determining pixels containing sub-pixel edges")]
        [DisplayName("Resolve Sub-Pixel Threshold")]
        float ResolveSubPixelThreshold = 0.025f;

        [MinValue(0.0f)]
        [MaxValue(1.0f)]
        [StepSize(0.001f)]
        [HelpText("Threshold used during low-resolution composite for determining pixels containing sub-pixel edges")]
        [DisplayName("Composite Sub-Pixel Threshold")]
        float CompositeSubPixelThreshold = 0.1f;

        [Visible(false)]
        [UseAsShaderConstant(false)]
        [HelpText("Use programmable sample positions when rendering low-resolution particles with 'MSAA' mode")]
        bool ProgrammableSamplePoints = true;

        [MinValue(0.0f)]
        [MaxValue(100.0f)]
        [StepSize(0.01f)]
        [DisplayName("Nearest-Depth Threshold")]
        [HelpText("Depth threshold to use for nearest-depth upsampling")]
        float NearestDepthThreshold = 0.25f;
    }

    [ExpandGroup(false)]
    public class PostProcessing
    {
        [DisplayName("Bloom Exposure Offset")]
        [MinValue(-10.0f)]
        [MaxValue(0.0f)]
        [StepSize(0.01f)]
        [HelpText("Exposure offset applied to generate the input of the bloom pass")]
        float BloomExposure = -4.0f;

        [DisplayName("Bloom Magnitude")]
        [MinValue(0.0f)]
        [MaxValue(2.0f)]
        [StepSize(0.01f)]
        [HelpText("Scale factor applied to the bloom results when combined with tone-mapped result")]
        float BloomMagnitude = 1.0f;

        [DisplayName("Bloom Blur Sigma")]
        [MinValue(0.5f)]
        [MaxValue(2.5f)]
        [StepSize(0.01f)]
        [HelpText("Sigma parameter of the Gaussian filter used in the bloom pass")]
        float BloomBlurSigma = 2.5f;
    }

    [ExpandGroup(true)]
    public class Debug
    {
        [UseAsShaderConstant(false)]
        [DisplayName("Enable VSync")]
        [HelpText("Enables or disables vertical sync during Present")]
        bool EnableVSync = true;

        [HelpText("Captures the screen output (before HUD rendering), and saves it to a file")]
        Button TakeScreenshot;

        [HelpText("When using MSAA low-res render mode, shows pixels that use subpixel data")]
        bool ShowMSAAEdges = false;
    }
}