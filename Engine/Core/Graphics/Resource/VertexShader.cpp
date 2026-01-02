#include "pch.h"
#include "VertexShader.h"

#include <d3dcompiler.h>

namespace engine
{
    void VertexShader::Create(const std::string& filePath)
    {
        GraphicsDevice::CompileShaderFromFile(
            filePath,
            "main",
            "vs_5_0",
            m_vertexShaderBuffer);

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