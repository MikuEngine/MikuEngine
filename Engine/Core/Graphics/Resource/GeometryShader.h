#pragma once

#include "Core/Graphics/Resource/Resource.h"

namespace engine
{
	class GeometryShader :
		public Resource
	{
	private:
		Microsoft::WRL::ComPtr<ID3D11GeometryShader> m_geometryShader;

	public:
		void Create(const std::string& filePath);

	public:
		const Microsoft::WRL::ComPtr<ID3D11GeometryShader>& GetShader() const;
		ID3D11GeometryShader* GetRawShader() const;
		ID3D11GeometryShader* const* GetRawShaderAddr() const;
	};
}