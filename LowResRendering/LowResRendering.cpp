//=================================================================================================
//
//  Low-Resolution Rendering Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code and content licensed under the MIT license
//
//=================================================================================================

#include <PCH.h>

#include <commdlg.h>

#include <InterfacePointers.h>
#include <Window.h>
#include <Input.h>
#include <Utility.h>
#include <Graphics/DeviceManager.h>
#include <Graphics/ShaderCompilation.h>
#include <Graphics/Profiler.h>
#include <Graphics/Textures.h>
#include <Graphics/Sampling.h>

#include "LowResRendering.h"
#include "SharedConstants.h"

#include "resource.h"

// Set this to 0 if you don't want to link against NVAPI
#define UseNVAPI_ (1)

#if UseNVAPI_

#include "../Externals/NVAPI-352/nvapi.h"
#pragma comment(lib, "../Externals/NVAPI-352/amd64/nvapi64.lib")

#endif

using namespace SampleFramework11;
using std::wstring;

static const float NearClip = 0.01f;
static const float FarClip = 100.0f;

// Model filenames
static const wstring ModelPaths[] =
{
    L"..\\Content\\Models\\Box\\Box_Grill.fbx",
};

// Saves the specified SRV as a PNG file, using a save file dialog to pick the path
static void SavePNGScreenshot(HWND parentWindow, ID3D11Texture2D* texture)
{
    wchar currDirectory[MAX_PATH] = { 0 };
    GetCurrentDirectory(ArraySize_(currDirectory), currDirectory);

    wchar filePath[MAX_PATH] = { 0 };

    OPENFILENAME ofn;
    ZeroMemory(&ofn , sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = parentWindow;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = ArraySize_(filePath);
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0PNG Files (*.png)\0*.png\0";
    ofn.nFilterIndex = 2;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.lpstrTitle = L"Save Screenshot As..";
    ofn.lpstrDefExt = L"png";
    ofn.Flags = OFN_OVERWRITEPROMPT;
    bool succeeded = GetSaveFileName(&ofn) != 0;
    SetCurrentDirectory(currDirectory);

    if(succeeded)
    {
        try
        {
            SaveTextureAsPNG(texture, filePath);
        }
        catch(Exception e)
        {
            std::wstring errorString = L"Error occured while saving screenshot as an EXR file:\n" + e.GetMessage();
            MessageBox(parentWindow, errorString.c_str(), L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

LowResRendering::LowResRendering() :  App(L"Low Resolution Rendering", MAKEINTRESOURCEW(IDI_DEFAULT)),
                                      camera(16.0f / 9.0f, Pi_4, NearClip, FarClip)
{
    deviceManager.SetMinFeatureLevel(D3D_FEATURE_LEVEL_11_0);
    globalHelpText = "Low Resolution Rendering Sample\n\n"
                     "Demonstrates how to render half-resolution transparents using 4xMSAA "
                     "in order to maintain full-resolution depth testing and rasterization.\n\n"
                     "Controls:\n\n"
                     "Use W/S/A/D/Q/E to move the camera, and hold right-click while dragging the mouse to rotate.";
}

void LowResRendering::BeforeReset()
{
    App::BeforeReset();
}

void LowResRendering::AfterReset()
{
    App::AfterReset();

    float aspect = static_cast<float>(deviceManager.BackBufferWidth()) / deviceManager.BackBufferHeight();
    camera.SetAspectRatio(aspect);

    CreateRenderTargets();

    postProcessor.AfterReset(deviceManager.BackBufferWidth(), deviceManager.BackBufferHeight());
}

void LowResRendering::Initialize()
{
    App::Initialize();

    ID3D11DevicePtr device = deviceManager.Device();
    ID3D11DeviceContextPtr deviceContext = deviceManager.ImmediateContext();

    // Create a font + SpriteRenderer
    font.Initialize(L"Consolas", 18, SpriteFont::Regular, true, device);
    spriteRenderer.Initialize(device);

    // Load the scenes
    for(uint64 i = 0; i < 1; ++i)
        sceneModels[i].CreateWithAssimp(device, ModelPaths[i].c_str(), true);

    Model& currentModel = sceneModels[0];
    meshRenderer.Initialize(device, deviceManager.ImmediateContext(), &currentModel);

    skybox.Initialize(device);

    // Load shaders
    for(uint32 msaaMode = 0; msaaMode < uint32(MSAAModes::NumValues); ++msaaMode)
    {
        CompileOptions opts;
        opts.Add("MSAASamples_", AppSettings::NumMSAASamples(MSAAModes(msaaMode)));
        resolvePS[msaaMode] = CompilePSFromFile(device, L"Resolve.hlsl", "ResolvePS", "ps_5_0", opts);
    }

    fullScreenTriVS = CompileVSFromFile(device, L"Resolve.hlsl", "ResolveVS");

    for(uint32 msaaMode = 0; msaaMode < uint32(MSAAModes::NumValues); ++msaaMode)
    {
        CompileOptions opts;
        opts.Add("MSAASamples_", AppSettings::NumMSAASamples(MSAAModes(msaaMode)));
        depthDownscalePS[msaaMode] = CompilePSFromFile(device, L"DepthDownscale.hlsl", "DepthDownscale", "ps_5_0", opts);
        msaaDepthDownscalePS[msaaMode] = CompilePSFromFile(device, L"DepthDownscale.hlsl", "DepthDownscaleMSAA", "ps_5_0", opts);
        msaaLowResCompositePS[msaaMode] = CompilePSFromFile(device, L"LowResComposite.hlsl", "LowResCompositeMSAA", "ps_5_0", opts);
        msaaLowResResolvePS[msaaMode] = CompilePSFromFile(device, L"LowResComposite.hlsl", "LowResResolve", "ps_5_0", opts);
        nearestDepthCompositePS[msaaMode] = CompilePSFromFile(device, L"LowResComposite.hlsl", "LowResCompositeNearestDepth", "ps_5_0", opts);
    }

    resolveConstants.Initialize(device);

    // Init the post processor
    postProcessor.Initialize(device);

    camera.SetPosition(Float3(0.0f, 2.5f, -20.0f));

    AppSettings::UpdateHorizontalCoords();

    InitializeParticles();

    InitializeNVAPI();
}

// Creates all required render targets
void LowResRendering::CreateRenderTargets()
{
    ID3D11Device* device = deviceManager.Device();
    uint32 width = deviceManager.BackBufferWidth();
    uint32 height = deviceManager.BackBufferHeight();

    const uint32 NumSamples = AppSettings::NumMSAASamples();
    const uint32 Quality = NumSamples > 0 ? D3D11_STANDARD_MULTISAMPLE_PATTERN : 0;
    colorTargetMSAA.Initialize(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, NumSamples, Quality);
    depthBuffer.Initialize(device, width, height, DXGI_FORMAT_D24_UNORM_S8_UINT, true, NumSamples, Quality);

    colorResolveTarget.Initialize(device, width, height, colorTargetMSAA.Format, 1, 1, 0);

    lowResTargetMSAA.Initialize(device, width / 2, height / 2, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, NumSamples * 4, 0);
    lowResTarget.Initialize(device, width / 2, height / 2, DXGI_FORMAT_R16G16B16A16_FLOAT, 1, 1, 0);
    lowResDepthMSAA.Initialize(device, width / 2, height / 2, DXGI_FORMAT_D24_UNORM_S8_UINT, true, NumSamples * 4, 0);
    lowResDepth.Initialize(device, width / 2, height / 2, DXGI_FORMAT_D24_UNORM_S8_UINT, true, 1, 0);

    meshRenderer.OnResize(width, height);
}

void LowResRendering::InitializeNVAPI()
{
    for(uint64 i = 0; i < ArraySize_(msaaLowResRS); ++i)
        msaaLowResRS[i] = rasterizerStates.BackFaceCull();

#if UseNVAPI_

    // This will fail if the user doesn't have an Nvidia GPU, so bail out gracefully
    // if it fails.
    if(NvAPI_Initialize() != NVAPI_OK)
    {
        PrintString("NVAPI failed to initialize. This is normal if you don't have an Nvidia GPU. "
                    "Programmable sample points not available");
        return;
    }

    ID3D11Device* device = deviceManager.Device();

    NvAPI_D3D11_RASTERIZER_DESC_EX rsDesc;
    ZeroMemory(&rsDesc, sizeof(rsDesc));

    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_BACK;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthBias = 0;
    rsDesc.DepthBiasClamp = 1.0f;
    rsDesc.SlopeScaledDepthBias = 0.0f;
    rsDesc.DepthClipEnable = TRUE;
    rsDesc.ScissorEnable = FALSE;
    rsDesc.MultisampleEnable = TRUE;
    rsDesc.AntialiasedLineEnable = FALSE;
    rsDesc.ForcedSampleCount = 0;
    rsDesc.ConservativeRasterEnable = false;
    rsDesc.QuadFillMode = NVAPI_QUAD_FILLMODE_DISABLED;
    rsDesc.PostZCoverageEnable = false;
    rsDesc.CoverageToColorEnable = false;
    rsDesc.CoverageToColorRTIndex = 0;
    rsDesc.ProgrammableSamplePositionsEnable = true;
    rsDesc.InterleavedSamplingEnable = false;

    StaticAssert_(uint64(MSAAModes::NumValues) == 2);

    // Sample points for 1x MSAA aliased as half-res 4x MSAA. These coordinates
    // give us a uniform grid within a pixel
    const Float2 SamplePositions1x[4] =
    {
        Float2(0.25f, 0.25f),
        Float2(0.75f, 0.25f),
        Float2(0.25f, 0.75f),
        Float2(0.75f, 0.75f),
    };

    rsDesc.SampleCount = 4;
    for(uint64 i = 0; i < 4; ++i)
    {
        rsDesc.SamplePositionsX[i] = uint8(Clamp(SamplePositions1x[i].x * 16.0f, 0.0f, 15.0f));
        rsDesc.SamplePositionsY[i] = uint8(Clamp(SamplePositions1x[i].y * 16.0f, 0.0f, 15.0f));
    };

    // This can also fail the user has an older Nvidia GPU that doesn't support programmable sample points
    NvAPI_Status status = NvAPI_D3D11_CreateRasterizerState(device, &rsDesc, &msaaLowResRS[0]);
    if(status != NVAPI_OK)
    {
        PrintString("Failed to create NVAPI rasterizer state. This is normal if you don't have a Maxwell 2.0 "
                    "or newer Nvidia GPU. Programmable sample points not available");
        return;
    }

    // Sample points for 2x MSAA aliased as half-res 8x MSAA. These coordinates
    // give us 4 sets of 2x rotated grid patterns within a pixel.
    Float2 subSampleOffset = Float2(0.25f, 0.25f);
    const Float2 SamplePositions2x[8] =
    {
        Float2(0.25f, 0.25f) + subSampleOffset *  0.5f,
        Float2(0.25f, 0.25f) + subSampleOffset * -0.5f,

        Float2(0.75f, 0.25f) + subSampleOffset *  0.5f,
        Float2(0.75f, 0.25f) + subSampleOffset * -0.5f,

        Float2(0.25f, 0.75f) + subSampleOffset *  0.5f,
        Float2(0.25f, 0.75f) + subSampleOffset * -0.5f,

        Float2(0.75f, 0.75f) + subSampleOffset *  0.5f,
        Float2(0.75f, 0.75f) + subSampleOffset * -0.5f,
    };

    rsDesc.SampleCount = 8;
    for(uint64 i = 0; i < 8; ++i)
    {
        rsDesc.SamplePositionsX[i] = uint8(Clamp(SamplePositions2x[i].x * 16.0f, 0.0f, 15.0f));
        rsDesc.SamplePositionsY[i] = uint8(Clamp(SamplePositions2x[i].y * 16.0f, 0.0f, 15.0f));
    };

    status = NvAPI_D3D11_CreateRasterizerState(device, &rsDesc, &msaaLowResRS[1]);
    if(status != NVAPI_OK)
    {
        PrintString("Failed to create NVAPI rasterizer state. This is normal if you don't have a Maxwell 2.0 "
                    "or newer Nvidia GPU. Programmable sample points not available");
        return;
    }

    // If we got this far, we can go ahead and enable programmable sample point support
    AppSettings::ProgrammableSamplePointsSupported = true;

    PrintString("NVAPI initialization succeeded! Programmable sample points are available");

#else
    PrintString("NVAPI support disabled, programmable sample points not available");
#endif

}

void LowResRendering::InitializeParticles()
{
    ID3D11Device* device = deviceManager.Device();

    smokeTexture = LoadTexture(device, L"..\\Content\\Textures\\Smoke.dds");
    particleConstants.Initialize(device);
    compositeConstants.Initialize(device);

    particlesVS = CompileVSFromFile(device, L"Particles.hlsl", "ParticlesVS");
    particlesPS = CompilePSFromFile(device, L"Particles.hlsl", "ParticlesPS");

    particleData.resize(AppSettings::MaxParticles);

    D3D11_BUFFER_DESC ibDesc;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.ByteWidth = sizeof(uint16) * 6;
    ibDesc.CPUAccessFlags = 0;
    ibDesc.MiscFlags = 0;
    ibDesc.StructureByteStride = 0;
    ibDesc.Usage = D3D11_USAGE_IMMUTABLE;

    const uint16 indices[6] = { 0, 1, 3, 3, 2, 0 };
    D3D11_SUBRESOURCE_DATA bufferInit;
    bufferInit.pSysMem = indices;
    bufferInit.SysMemPitch = 0;
    bufferInit.SysMemSlicePitch = 0;
    DXCall(device->CreateBuffer(&ibDesc, &bufferInit, &particleIB));

    D3D11_BUFFER_DESC pbDesc;
    pbDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    pbDesc.ByteWidth = sizeof(ParticleData) * AppSettings::MaxParticles;
    pbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    pbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    pbDesc.StructureByteStride = sizeof(ParticleData);
    pbDesc.Usage = D3D11_USAGE_DYNAMIC;
    DXCall(device->CreateBuffer(&pbDesc, nullptr, &particleBuffer));
    DXCall(device->CreateShaderResourceView(particleBuffer, nullptr, &particleBufferSRV));

    D3D11_BLEND_DESC blendDesc;
    ZeroMemory(&blendDesc, sizeof(blendDesc));
    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.IndependentBlendEnable = false;
    blendDesc.RenderTarget[0].BlendEnable = true;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    DXCall(device->CreateBlendState(&blendDesc, &particleBlendState));

    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
    DXCall(device->CreateBlendState(&blendDesc, &compositeBlendState));

    #if Debug_
        // Disable particle sorting by default for debug, since it's sloooooooow
        AppSettings::SortParticles.SetValue(false);
    #endif
}

struct ParticleDepthCompare
{
    Float4x4 ViewMatrix;
    bool operator()(const ParticleData& a, const ParticleData& b)
    {
        float depthA = Float3::Transform(a.Position, ViewMatrix).z;
        float depthB = Float3::Transform(b.Position, ViewMatrix).z;
        return depthA > depthB;
    }
};

void LowResRendering::UpdateParticles(const Timer& timer)
{
    randomGenerator.SetSeed(0);

    const Float3 emitCenter = Float3(AppSettings::EmitCenterX, AppSettings::EmitCenterY, AppSettings::EmitCenterZ);
    const float emitRadius = AppSettings::EmitRadius;

    rotationAmount += timer.DeltaSecondsF() * AppSettings::RotationSpeed;
    Float4x4 rotation = XMMatrixRotationY(rotationAmount);

    numParticles = AppSettings::NumParticles * 1024;
    for(uint64 i = 0; i < numParticles; ++i)
    {
        ParticleData& particle = particleData[i];
        Float3 spherePos = SampleSphere(randomGenerator.RandomFloat(), randomGenerator.RandomFloat(),
                                        randomGenerator.RandomFloat(), randomGenerator.RandomFloat());
        spherePos = Float3::TransformDirection(spherePos, rotation);
        particle.Position = emitCenter + spherePos * emitRadius;
        particle.Opacity = 0.5f + randomGenerator.RandomFloat() * 0.5f;
        particle.Size = 0.25f + randomGenerator.RandomFloat() * 0.25f;
    }

    if(AppSettings::SortParticles)
    {
        ParticleDepthCompare comparer;
        comparer.ViewMatrix = camera.ViewMatrix();
        std::sort(particleData.begin(), particleData.begin() + numParticles, comparer);
    }
}

void LowResRendering::Update(const Timer& timer)
{
    AppSettings::UpdateUI();

    MouseState mouseState = MouseState::GetMouseState(window);
    KeyboardState kbState = KeyboardState::GetKeyboardState(window);

    if(kbState.IsKeyDown(KeyboardState::Escape))
        window.Destroy();

    float CamMoveSpeed = 5.0f * timer.DeltaSecondsF();
    const float CamRotSpeed = 0.180f * timer.DeltaSecondsF();
    const float MeshRotSpeed = 0.180f * timer.DeltaSecondsF();

    // Move the camera with keyboard input
    if(kbState.IsKeyDown(KeyboardState::LeftShift))
        CamMoveSpeed *= 0.25f;

    Float3 camPos = camera.Position();
    if(kbState.IsKeyDown(KeyboardState::W))
        camPos += camera.Forward() * CamMoveSpeed;
    else if (kbState.IsKeyDown(KeyboardState::S))
        camPos += camera.Back() * CamMoveSpeed;
    if(kbState.IsKeyDown(KeyboardState::A))
        camPos += camera.Left() * CamMoveSpeed;
    else if (kbState.IsKeyDown(KeyboardState::D))
        camPos += camera.Right() * CamMoveSpeed;
    if(kbState.IsKeyDown(KeyboardState::Q))
        camPos += camera.Up() * CamMoveSpeed;
    else if (kbState.IsKeyDown(KeyboardState::E))
        camPos += camera.Down() * CamMoveSpeed;
    camera.SetPosition(camPos);

    // Rotate the camera with the mouse
    if(mouseState.RButton.Pressed && mouseState.IsOverWindow)
    {
        float xRot = camera.XRotation();
        float yRot = camera.YRotation();
        xRot += mouseState.DY * CamRotSpeed;
        yRot += mouseState.DX * CamRotSpeed;
        camera.SetXRotation(xRot);
        camera.SetYRotation(yRot);
    }

    // Toggle VSYNC
    deviceManager.SetVSYNCEnabled(AppSettings::EnableVSync ? true : false);

    meshRenderer.Update(camera);

    UpdateParticles(timer);
}

void LowResRendering::Render(const Timer& timer)
{
    if(AppSettings::MSAAMode.Changed())
        CreateRenderTargets();

    ID3D11DeviceContextPtr context = deviceManager.ImmediateContext();

    {
        ProfileBlock profileBlock(L"Total GPU Time");

        RenderMainPass();

        RenderParticles(timer);

        RenderAA();

        {
            // Kick off post-processing
            PIXEvent ppEvent(L"Post Processing");
            postProcessor.Render(context, colorResolveTarget.SRView, depthBuffer.SRView, camera,
                                 deviceManager.BackBuffer(), timer.DeltaSecondsF());
        }
    }

    if(AppSettings::TakeScreenshot)
        SavePNGScreenshot(window.GetHwnd(), deviceManager.BackBufferTexture());


    ID3D11RenderTargetView* renderTargets[1] = { deviceManager.BackBuffer() };
    context->OMSetRenderTargets(1, renderTargets, NULL);

    SetViewport(context, deviceManager.BackBufferWidth(), deviceManager.BackBufferHeight());

    RenderHUD(timer);
}

void LowResRendering::RenderMainPass()
{
    PIXEvent event(L"Main Pass");

    ID3D11DeviceContextPtr context = deviceManager.ImmediateContext();

    ID3D11RenderTargetView* renderTargets[1] = { nullptr };
    ID3D11DepthStencilView* ds = depthBuffer.DSView;
    context->OMSetRenderTargets(1, renderTargets, ds);

    SetViewport(context, colorTargetMSAA.Width, colorTargetMSAA.Height);

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    context->ClearRenderTargetView(colorTargetMSAA.RTView, clearColor);
    context->ClearDepthStencilView(ds, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

    meshRenderer.RenderDepth(context, camera, false, false);

    meshRenderer.ReduceDepth(context, depthBuffer, camera);

    if(AppSettings::EnableSun)
        meshRenderer.RenderSunShadowMap(context, camera);

    renderTargets[0] = colorTargetMSAA.RTView;
    context->OMSetRenderTargets(1, renderTargets, ds);
    SetViewport(context, colorTargetMSAA.Width, colorTargetMSAA.Height);

    meshRenderer.RenderMainPass(context, camera);

    Float3 skyScale = Float3(AppSettings::ExposureRangeScale);

    float sunSize = AppSettings::EnableSun ? AppSettings::SunSize : 0.0f;
    skybox.RenderSky(context, AppSettings::SunDirection, AppSettings::GroundAlbedo,
                        AppSettings::SunLuminance(), sunSize,
                        AppSettings::Turbidity, camera.ViewMatrix(),
                        camera.ProjectionMatrix(), skyScale);
}

void LowResRendering::RenderParticles(const Timer& timer)
{
    ID3D11DeviceContextPtr context = deviceManager.ImmediateContext();
    ID3D11RenderTargetView* renderTargets[1] = { nullptr };
    ID3D11ShaderResourceView* srvs[4] = { nullptr };
    float blendFactor[4] = {1, 1, 1, 1};

    ID3D11Buffer* vbs[1] = { nullptr };
    UINT strides[1] = { 0 };
    UINT offsets[1] = { 0 };
    context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
    context->IASetInputLayout(nullptr);
    context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    const bool lowResMSAA = AppSettings::LowResRenderMode == LowResRenderModes::MSAA;
    ID3D11DepthStencilView* lowResDS = lowResMSAA ? lowResDepthMSAA.DSView : lowResDepth.DSView;
    ID3D11RenderTargetView* lowResRT = lowResMSAA ? lowResTargetMSAA.RTView : lowResTarget.RTView;

    ProfileBlock totalProfileBlock(L"Particle Rendering Total");

    if(AppSettings::RenderLowRes)
    {
        PIXEvent downscaleEvent(L"Low-Res Downscale");
        ProfileBlock profileBlock(L"Depth Downscale");

        context->OMSetRenderTargets(1, renderTargets, lowResDS);

        SetViewport(context, lowResTarget.Width, lowResTarget.Height);

        context->ClearDepthStencilView(lowResDS, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

        context->VSSetShader(fullScreenTriVS, nullptr, 0);
        ID3D11PixelShader* downscalePS = depthDownscalePS[AppSettings::MSAAMode];
        if(lowResMSAA)
            downscalePS = msaaDepthDownscalePS[AppSettings::MSAAMode];
        context->PSSetShader(downscalePS, nullptr, 0);

        ID3D11ShaderResourceView* srvs[1] = { depthBuffer.SRView };
        context->PSSetShaderResources(0, 1, srvs);

        context->OMSetBlendState(blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);
        context->OMSetDepthStencilState(depthStencilStates.DepthWriteEnabled(), 0);
        if(lowResMSAA && AppSettings::ProgrammableSamplePoints)
            context->RSSetState(msaaLowResRS[AppSettings::MSAAMode]);
        else
            context->RSSetState(rasterizerStates.BackFaceCull());

        context->Draw(3, 0);

        srvs[0] = nullptr;
        context->PSSetShaderResources(0, 1, srvs);
    }

    {
        PIXEvent renderEvent(L"Particle Rendering");
        ProfileBlock profileBlock(L"Particle Rendering");

        if(AppSettings::RenderLowRes)
        {
            float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
            context->ClearRenderTargetView(lowResRT, clearColor);

            renderTargets[0] = lowResRT;
            context->OMSetRenderTargets(1, renderTargets, lowResDS);
            context->OMSetBlendState(particleBlendState, blendFactor, 0xFFFFFFFF);
            if(lowResMSAA && AppSettings::ProgrammableSamplePoints)
                context->RSSetState(msaaLowResRS[AppSettings::MSAAMode]);
            else
                context->RSSetState(rasterizerStates.BackFaceCull());
        }
        else
        {
            SetViewport(context, colorTargetMSAA.Width, colorTargetMSAA.Height);

            renderTargets[0] = colorTargetMSAA.RTView;
            context->OMSetRenderTargets(1, renderTargets, depthBuffer.DSView);
            context->RSSetState(rasterizerStates.BackFaceCull());
            context->OMSetBlendState(blendStates.AlphaBlend(), blendFactor, 0xFFFFFFFF);
        }

        context->OMSetDepthStencilState(depthStencilStates.DepthEnabled(), 0);

        srvs[0] = particleBufferSRV;
        context->VSSetShaderResources(0, 1, srvs);

        srvs[0] = smokeTexture;
        srvs[1] = meshRenderer.sunVSM.SRView;
        context->PSSetShaderResources(0, 2, srvs);

        D3D11_MAPPED_SUBRESOURCE mapped;
        DXCall(context->Map(particleBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
        memcpy(mapped.pData, particleData.data(), numParticles * sizeof(ParticleData));
        context->Unmap(particleBuffer, 0);

        particleConstants.Data.CameraRight =  camera.WorldMatrix().Right();
        particleConstants.Data.CameraUp =  camera.WorldMatrix().Up();
        particleConstants.Data.Time = timer.ElapsedSecondsF();
        particleConstants.Data.ViewProjection = Float4x4::Transpose(camera.ViewProjectionMatrix());
        particleConstants.Data.CameraPosWS = camera.Position();
        particleConstants.Data.SunIlluminance = AppSettings::SunIlluminance();
        particleConstants.Data.SunDirectionWS = AppSettings::SunDirection;
        particleConstants.ApplyChanges(context);
        particleConstants.SetVS(context, 0);
        particleConstants.SetPS(context, 0);

        meshRenderer.shadowConstants.SetPS(context, 1);

        ID3D11SamplerState* samplers[2] = { samplerStates.Linear(), samplerStates.Anisotropic() };
        context->PSSetSamplers(0, 2, samplers);

        context->VSSetShader(particlesVS, nullptr, 0);
        context->PSSetShader(particlesPS, nullptr, 0);

        context->IASetIndexBuffer(particleIB, DXGI_FORMAT_R16_UINT, 0);
        context->DrawIndexedInstanced(6, uint32(numParticles), 0, 0, 0);

        srvs[0] = srvs[1] = nullptr;
        context->VSSetShaderResources(0, 1, srvs);
        context->PSSetShaderResources(0, 2, srvs);
    }

    if(lowResMSAA)
    {
        PIXEvent compositeEvent(L"Low-Res Resolve");
        ProfileBlock profileBlock(L"Low-Res Resolve");

        renderTargets[0] = lowResTarget.RTView;
        context->OMSetRenderTargets(1, renderTargets, nullptr);

        context->OMSetBlendState(blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);
        context->OMSetDepthStencilState(depthStencilStates.DepthDisabled(), 0);
        context->RSSetState(rasterizerStates.NoCull());
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);

        srvs[0] = lowResTargetMSAA.SRView;
        context->PSSetShaderResources(0, 1, srvs);

        context->VSSetShader(fullScreenTriVS, nullptr, 0);
        context->PSSetShader(msaaLowResResolvePS[AppSettings::MSAAMode], nullptr, 0);

        context->Draw(3, 0);

        srvs[0] = nullptr;
        context->PSSetShaderResources(0, 1, srvs);
    }

    if(AppSettings::RenderLowRes)
    {
        PIXEvent compositeEvent(L"Low-Res Composite");
        ProfileBlock profileBlock(L"Low-Res Composite");

        renderTargets[0] = colorTargetMSAA.RTView;
        context->OMSetRenderTargets(1, renderTargets, nullptr);

        SetViewport(context, colorTargetMSAA.Width, colorTargetMSAA.Height);

        context->OMSetBlendState(compositeBlendState, blendFactor, 0xFFFFFFFF);
        context->OMSetDepthStencilState(depthStencilStates.DepthDisabled(), 0);
        context->RSSetState(rasterizerStates.NoCull());
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);

        compositeConstants.Data.Projection = Float4x4::Transpose(camera.ProjectionMatrix());
        compositeConstants.Data.LowResSize = Float2(float(lowResTarget.Width), float(lowResTarget.Height));
        compositeConstants.ApplyChanges(context);
        compositeConstants.SetPS(context, 0);

        srvs[0] = lowResTargetMSAA.SRView;
        srvs[1] = lowResTarget.SRView;
        srvs[2] = lowResDepth.SRView;
        srvs[3] = depthBuffer.SRView;
        context->PSSetShaderResources(0, 4, srvs);

        ID3D11SamplerState* samplers[2] = { samplerStates.Linear(), samplerStates.Point() };
        context->PSSetSamplers(0, 2, samplers);

        context->VSSetShader(fullScreenTriVS, nullptr, 0);
        if(AppSettings::LowResRenderMode == LowResRenderModes::MSAA)
            context->PSSetShader(msaaLowResCompositePS[AppSettings::MSAAMode], nullptr, 0);
        else if(AppSettings::LowResRenderMode == LowResRenderModes::NearestDepth)
            context->PSSetShader(nearestDepthCompositePS[AppSettings::MSAAMode], nullptr, 0);

        context->Draw(3, 0);

        srvs[0] = srvs[1] = srvs[2] = srvs[3] = nullptr;
        context->PSSetShaderResources(0, 4, srvs);
    }
}

void LowResRendering::RenderAA()
{
    PIXEvent pixEvent(L"MSAA Resolve");

    ID3D11DeviceContext* context = deviceManager.ImmediateContext();

    ID3D11RenderTargetView* rtvs[1] = { colorResolveTarget.RTView };

    context->OMSetRenderTargets(1, rtvs, nullptr);

    float blendFactor[4] = {1, 1, 1, 1};
    context->OMSetBlendState(blendStates.BlendDisabled(), blendFactor, 0xFFFFFFFF);
    context->OMSetDepthStencilState(depthStencilStates.DepthDisabled(), 0);
    context->RSSetState(rasterizerStates.NoCull());

    const uint32 SampleRadius = static_cast<uint32>((AppSettings::FilterSize / 2.0f) + 0.499f);
    ID3D11PixelShader* pixelShader = resolvePS[AppSettings::MSAAMode];
    context->PSSetShader(pixelShader, nullptr, 0);
    context->VSSetShader(fullScreenTriVS, nullptr, 0);

    resolveConstants.Data.TextureSize = Float2(float(colorTargetMSAA.Width), float(colorTargetMSAA.Height));
    resolveConstants.Data.SampleRadius = SampleRadius;
    resolveConstants.ApplyChanges(context);
    resolveConstants.SetPS(context, 0);

    ID3D11ShaderResourceView* srvs[1] = { colorTargetMSAA.SRView };
    context->PSSetShaderResources(0, 1, srvs);

    ID3D11Buffer* vbs[1] = { nullptr };
    UINT strides[1] = { 0 };
    UINT offsets[1] = { 0 };
    context->IASetVertexBuffers(0, 1, vbs, strides, offsets);
    context->IASetInputLayout(nullptr);
    context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
    context->Draw(3, 0);

    rtvs[0] = nullptr;
    context->OMSetRenderTargets(1, rtvs, nullptr);

    srvs[0] = nullptr;
    context->PSSetShaderResources(0, 1, srvs);
}

void LowResRendering::RenderHUD(const Timer& timer)
{
    PIXEvent event(L"HUD Pass");

    ID3D11DeviceContext* context = deviceManager.ImmediateContext();
    spriteRenderer.Begin(context, SpriteRenderer::Point);

    Float4x4 transform = Float4x4::TranslationMatrix(Float3(25.0f, 25.0f, 0.0f));
    wstring fpsText = MakeString(L"Frame Time: %.2fms (%u FPS)", 1000.0f / fps, fps);
    spriteRenderer.RenderText(font, fpsText.c_str(), transform, XMFLOAT4(1, 1, 0, 1));

    Profiler::GlobalProfiler.EndFrame(spriteRenderer, font);

    spriteRenderer.End();
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    LowResRendering app;
    app.Run();
}
