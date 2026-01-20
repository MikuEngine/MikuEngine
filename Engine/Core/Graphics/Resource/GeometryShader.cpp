#include "EnginePCH.h"
#include "GeometryShader.h"

#include "Core/Graphics/Device/GraphicsDevice.h"

namespace engine
{
	void GeometryShader::Create(const std::string& filePath)
	{
		Microsoft::WRL::ComPtr<ID3DBlob> geometryShaderBuffer;

		GraphicsDevice::CompileShaderFromFile(
			filePath,
			"main",
			"gs_5_0",
			geometryShaderBuffer);

		HR_CHECK(GraphicsDevice::Get().GetDevice()->CreateGeometryShader(
			geometryShaderBuffer->GetBufferPointer(),
			geometryShaderBuffer->GetBufferSize(),
			nullptr,
			&m_geometryShader));
	}

	const Microsoft::WRL::ComPtr<ID3D11GeometryShader>& GeometryShader::GetShader() const
	{
		return m_geometryShader;
	}

	ID3D11GeometryShader* GeometryShader::GetRawShader() const
	{
		return m_geometryShader.Get();
	}

	ID3D11GeometryShader* const* GeometryShader::GetRawShaderAddr() const
	{
		return m_geometryShader.GetAddressOf();
	}
}