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

#include <Graphics/ShaderCompilation.h>
#include <Graphics/Profiler.h>
#include <Graphics/Camera.h>
#include <Graphics/Textures.h>

#include "PostProcessor.h"
#include "SharedConstants.h"

// Constants
static const uint32 TGSize = 16;
static const uint32 LumMapSize = 1024;

void PostProcessor::Initialize(ID3D11Device* device)
{
    PostProcessorBase::Initialize(device);

    // Load the shaders
    toneMap = CompilePSFromFile(device, L"PostProcessing.hlsl", "ToneMap");
    scale = CompilePSFromFile(device, L"PostProcessing.hlsl", "Scale");
    blurH = CompilePSFromFile(device, L"PostProcessing.hlsl", "BlurH");
    blurV = CompilePSFromFile(device, L"PostProcessing.hlsl", "BlurV");
    bloom = CompilePSFromFile(device, L"PostProcessing.hlsl", "Bloom");
}

void PostProcessor::AfterReset(uint32 width, uint32 height)
{
    PostProcessorBase::AfterReset(width, height);
}

void PostProcessor::Render(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* input,
                           ID3D11ShaderResourceView* depthBuffer, const Camera& camera,
                           ID3D11RenderTargetView* output, float deltaSeconds)
{
    PostProcessorBase::Render(deviceContext, input, output);

    TempRenderTarget* bloom = Bloom(input);

    // Apply tone mapping
    ToneMap(input, bloom->SRView, output);

    bloom->InUse = false;

    // Check for leaked temp render targets
    for(uint64 i = 0; i < tempRenderTargets.size(); ++i)
        Assert_(tempRenderTargets[i]->InUse == false);
}

TempRenderTarget* PostProcessor::Bloom(ID3D11ShaderResourceView* input)
{
    PIXEvent pixEvent(L"Bloom");

    TempRenderTarget* downscale1 = GetTempRenderTarget(inputWidth / 2, inputHeight / 2, DXGI_FORMAT_R16G16B16A16_FLOAT);
    inputs.push_back(input);
    outputs.push_back(downscale1->RTView);
    PostProcess(bloom, L"Bloom Initial Pass");

    // Blur it
    for(uint64 i = 0; i < 2; ++i)
    {
        TempRenderTarget* blurTemp = GetTempRenderTarget(inputWidth / 2, inputHeight / 2, DXGI_FORMAT_R16G16B16A16_FLOAT);
        PostProcess(downscale1->SRView, blurTemp->RTView, blurH, L"Horizontal Bloom Blur");

        PostProcess(blurTemp->SRView, downscale1->RTView, blurV, L"Vertical Bloom Blur");
        blurTemp->InUse = false;
    }

    return downscale1;
}

void PostProcessor::ToneMap(ID3D11ShaderResourceView* input,
                            ID3D11ShaderResourceView* bloom,
                            ID3D11RenderTargetView* output)
{
    // Use an intermediate render target so that we can render do sRGB conversion ourselves in the shader
    TempRenderTarget* tmOutput = GetTempRenderTarget(inputWidth, inputHeight, DXGI_FORMAT_R8G8B8A8_UNORM);

    inputs.push_back(input);
    inputs.push_back(bloom);
    outputs.push_back(tmOutput->RTView);

    PostProcess(toneMap, L"Tone Mapping");

    ID3D11ResourcePtr outputResource;
    output->GetResource(&outputResource);

    context->CopyResource(outputResource, tmOutput->Texture);

    tmOutput->InUse = false;
}