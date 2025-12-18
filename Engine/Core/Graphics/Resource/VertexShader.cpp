#include "pch.h"
#include "VertexShader.h"

#include <d3dcompiler.h>

namespace engine
{
    void VertexShader::Create(const std::string& filePath)
    {
        DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
        shaderFlags |= D3DCOMPILE_DEBUG;
        shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif // _DEBUG

        HR_CHECK(D3DCompileFromFile(
            ToWideChar(filePath).c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main",
            "vs_5_0",
            shaderFlags,
            0,
            &m_vertexShaderBuffer,
            nullptr));

        HR_CHECK(GraphicsDevice::Get().GetDevice()->CreateVertexShader(
            m_vertexShaderBuffer->GetBufferPointer(),
            m_vertexShaderBuffer->GetBufferSize(),
            nullptr,
            &m_vertexShader));
    }

    const Microsoft::WRL::ComPtr<ID3D11VertexShader>& VertexShader::GetShader() const
    {
        return m_vertexShader;
    }

    ID3D11VertexShader* VertexShader::GetRawShader() const
    {
        return m_vertexShader.Get();
    }
}