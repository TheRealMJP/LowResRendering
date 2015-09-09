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
#include <Graphics/GraphicsTypes.h>
#include <Graphics/PostProcessorBase.h>
#include <Graphics/DeviceStates.h>

#include "AppSettings.h"

using namespace SampleFramework11;

namespace SampleFramework11
{
    class Camera;
}

class PostProcessor : public PostProcessorBase
{

public:

    void Initialize(ID3D11Device* device);

    void Render(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* input,
                ID3D11ShaderResourceView* depthBuffer, const Camera& camera,
                ID3D11RenderTargetView* output, float deltaSeconds);
    void AfterReset(UINT width, UINT height);

protected:

    void CalcAvgLuminance(ID3D11ShaderResourceView* input);
    TempRenderTarget* Bloom(ID3D11ShaderResourceView* input);
    void ToneMap(ID3D11ShaderResourceView* input,
                 ID3D11ShaderResourceView* bloom,
                 ID3D11RenderTargetView* output);

    PixelShaderPtr toneMap;
    PixelShaderPtr scale;
    PixelShaderPtr bloom;
    PixelShaderPtr blurH;
    PixelShaderPtr blurV;
};