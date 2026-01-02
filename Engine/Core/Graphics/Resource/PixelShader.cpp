#include "pch.h"
#include "PixelShader.h"

#include <d3dcompiler.h>

namespace engine
{
    void PixelShader::Create(const std::string& filePath)
    {
        Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBuffer;

        GraphicsDevice::CompileShaderFromFile(
            filePath,
            "main",
            "ps_5_0",
            pixelShaderBuffer);

        HR_CHECK(GraphicsDevice::Get().GetDevice()->CreatePixelShader(
            pixelShaderBuffer->GetBufferPointer(),
            pixelShaderBuffer->GetBufferSize(),
            nullptr,
            &m_pixelShader));
    }

    const Microsoft::WRL::ComPtr<ID3D11PixelShader>& PixelShader::GetShader() const
    {
        return m_pixelShader;
    }

    ID3D11PixelShader* PixelShader::GetRawShader() const
    {
        return m_pixelShader.Get();
    }
}