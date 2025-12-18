#include "pch.h"
#include "PixelShader.h"

#include <d3dcompiler.h>

namespace engine
{
    void PixelShader::Create(const std::string& filePath)
    {
        DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
        shaderFlags |= D3DCOMPILE_DEBUG;
        shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG

        Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBuffer;

        HR_CHECK(D3DCompileFromFile(
            ToWideChar(filePath).c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main",
            "ps_5_0",
            shaderFlags,
            0,
            &pixelShaderBuffer,
            nullptr));

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