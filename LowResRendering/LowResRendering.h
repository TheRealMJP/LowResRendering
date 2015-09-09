//=================================================================================================
//
//  Low-Resolution Rendering Sample
//  by MJP
//  http://mynameismjp.wordpress.com/
//
//  All code and content licensed under the MIT license
//
//=================================================================================================

#pragma once

#include <PCH.h>

#include <App.h>
#include <InterfacePointers.h>
#include <Input.h>
#include <Graphics/Camera.h>
#include <Graphics/Model.h>
#include <Graphics/SpriteFont.h>
#include <Graphics/SpriteRenderer.h>
#include <Graphics/Skybox.h>
#include <Graphics/GraphicsTypes.h>

#include "PostProcessor.h"
#include "MeshRenderer.h"

using namespace SampleFramework11;

struct ParticleData;

class LowResRendering : public App
{

protected:

    FirstPersonCamera camera;

    SpriteFont font;
    SampleFramework11::SpriteRenderer spriteRenderer;
    Skybox skybox;

    PostProcessor postProcessor;
    DepthStencilBuffer depthBuffer;
    RenderTarget2D colorTargetMSAA;
    RenderTarget2D colorResolveTarget;
    RenderTarget2D lowResTargetMSAA;
    RenderTarget2D lowResTarget;
    DepthStencilBuffer lowResDepthMSAA;
    DepthStencilBuffer lowResDepth;

    // Model
    Model sceneModels[1];
    MeshRenderer meshRenderer;

    ID3D11RasterizerStatePtr msaaLowResRS[uint64(MSAAModes::NumValues)];
    PixelShaderPtr depthDownscalePS[uint64(MSAAModes::NumValues)];
    PixelShaderPtr msaaDepthDownscalePS[uint64(MSAAModes::NumValues)];
    PixelShaderPtr msaaLowResCompositePS[uint64(MSAAModes::NumValues)];
    PixelShaderPtr msaaLowResResolvePS[uint64(MSAAModes::NumValues)];
    PixelShaderPtr nearestDepthCompositePS[uint64(MSAAModes::NumValues)];

    VertexShaderPtr particlesVS;
    PixelShaderPtr particlesPS;
    std::vector<ParticleData> particleData;
    uint64 numParticles = 0;
    float rotationAmount = 0.0f;
    ID3D11BufferPtr particleBuffer;
    ID3D11ShaderResourceViewPtr particleBufferSRV;
    ID3D11BufferPtr particleIB;
    ID3D11ShaderResourceViewPtr smokeTexture;
    Random randomGenerator;
    ID3D11BlendStatePtr particleBlendState;
    ID3D11BlendStatePtr compositeBlendState;

    VertexShaderPtr fullScreenTriVS;
    PixelShaderPtr resolvePS[uint64(MSAAModes::NumValues)];

    struct ParticleConstants
    {
        Float4x4 ViewProjection;
        Float3 CameraRight;
        Float4Align Float3 CameraUp;
        float Time;
        Float4Align Float3 SunDirectionWS;
        Float4Align Float3 SunIlluminance;
        Float4Align Float3 CameraPosWS;
    };

    ConstantBuffer<ParticleConstants> particleConstants;

    struct CompositeConstants
    {
        Float4x4 Projection;
        Float2 LowResSize;
    };

    ConstantBuffer<CompositeConstants> compositeConstants;

    struct ResolveConstants
    {
        uint32 SampleRadius;
        Float2 TextureSize;
    };

    ConstantBuffer<ResolveConstants> resolveConstants;

    virtual void Initialize() override;
    virtual void Render(const Timer& timer) override;
    virtual void Update(const Timer& timer) override;
    virtual void BeforeReset() override;
    virtual void AfterReset() override;

    void CreateRenderTargets();
    void InitializeNVAPI();
    void InitializeParticles();

    void UpdateParticles(const Timer& timer);

    void RenderMainPass();
    void RenderParticles(const Timer& timer);
    void RenderAA();
    void RenderHUD(const Timer& timer);

public:

    LowResRendering();
};
