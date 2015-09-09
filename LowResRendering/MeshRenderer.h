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

#include <Graphics/Model.h>
#include <Graphics/GraphicsTypes.h>
#include <Graphics/DeviceStates.h>
#include <Graphics/Camera.h>
#include <Graphics/SH.h>
#include <Graphics/ShaderCompilation.h>

#include "AppSettings.h"

using namespace SampleFramework11;

class MeshRenderer
{

protected:

    // Constants
    static const uint32 NumCascades = 4;
    static const uint32 ReadbackLatency = 1;

public:

    MeshRenderer();

    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context, const Model* sceneModel);
    void SetModel(const Model* model);

    void RenderDepth(ID3D11DeviceContext* context, const Camera& camera, bool noZClip, bool flippedZRange);
    void RenderMainPass(ID3D11DeviceContext* context, const Camera& camera);

    void Update(const Camera& camera);

    void OnResize(uint32 width, uint32 height);

    void ReduceDepth(ID3D11DeviceContext* context, DepthStencilBuffer& depthTarget,
                     const Camera& camera);

    void RenderSunShadowMap(ID3D11DeviceContext* context, const Camera& camera);


protected:

    void LoadShaders();
    void CreateShadowMaps();
    void ConvertToEVSM(ID3D11DeviceContext* context, uint32 cascadeIdx, Float3 cascadeScale);

    ID3D11DevicePtr device;

    BlendStates blendStates;
    RasterizerStates rasterizerStates;
    DepthStencilStates depthStencilStates;
    SamplerStates samplerStates;

    const Model* sceneModel;

    DepthStencilBuffer sunShadowDepthMap;
    RenderTarget2D tempVSM;

    ID3D11RasterizerStatePtr noZClipRSState;
    ID3D11SamplerStatePtr evsmSampler;

    std::vector<ID3D11InputLayoutPtr> meshInputLayouts;
    VertexShaderPtr meshVS;
    PixelShaderPtr meshPS;

    std::vector<ID3D11InputLayoutPtr> meshDepthInputLayouts;
    VertexShaderPtr meshDepthVS;

    VertexShaderPtr fullScreenVS;
    PixelShaderPtr evsmConvertPS;
    PixelShaderPtr evsmBlurH;
    PixelShaderPtr evsmBlurV;

    ComputeShaderPtr depthReductionInitialCS[2];
    ComputeShaderPtr depthReductionCS;
    std::vector<RenderTarget2D> depthReductionTargets;
    StagingTexture2D reductionStagingTextures[ReadbackLatency];
    uint32 currFrame;

    Float2 reductionDepth;

    // Constant buffers
    struct MeshVSConstants
    {
        Float4Align Float4x4 World;
        Float4Align Float4x4 View;
        Float4Align Float4x4 WorldViewProjection;
    };

    struct MeshPSConstants
    {
        Float4Align Float3 SunDirectionWS;
        float CosSunAngularRadius;
        Float4Align Float3 SunIlluminance;
        float SinSunAngularRadius;
        Float4Align Float3 CameraPosWS;
    };

    struct EVSMConstants
    {
        Float3 CascadeScale;
        float PositiveExponent;
        float NegativeExponent;
        float FilterSize;
        Float2 ShadowMapSize;
    };

    struct ReductionConstants
    {
        Float4x4 Projection;
        float NearClip;
        float FarClip;
        Uint2 TextureSize;
        uint32 NumSamples;
    };

    ConstantBuffer<MeshVSConstants> meshVSConstants;
    ConstantBuffer<MeshPSConstants> meshPSConstants;
    ConstantBuffer<EVSMConstants> evsmConstants;
    ConstantBuffer<ReductionConstants> reductionConstants;

public:

    struct ShadowConstants
    {
        Float4Align Float4x4 ShadowMatrix;
        Float4Align float CascadeSplits[NumCascades];

        Float4Align Float4 CascadeOffsets[NumCascades];
        Float4Align Float4 CascadeScales[NumCascades];

        float PositiveExponent;
        float NegativeExponent;
        float LightBleedingReduction;
    };

    ConstantBuffer<ShadowConstants> shadowConstants;
    RenderTarget2D sunVSM;
};