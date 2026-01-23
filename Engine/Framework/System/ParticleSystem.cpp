#include "EnginePCH.h"
#include "ParticleSystem.h"

#include "Core/Graphics/Device/GraphicsDevice.h"
#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Resource/VertexBuffer.h"
#include "Core/Graphics/Resource/IndexBuffer.h"
#include "Core/Graphics/Resource/VertexShader.h"
#include "Core/Graphics/Resource/PixelShader.h"
#include "Core/Graphics/Resource/InputLayout.h"
#include "Core/Graphics/Resource/BlendState.h"
#include "Core/Graphics/Resource/DepthStencilState.h"
#include "Core/Graphics/Resource/Texture.h"
#include "Core/Graphics/Data/ShaderSlotTypes.h"
#include "Core/Graphics/Data/ParticleStructuredData.h"

namespace engine
{
	ParticleSystem::ParticleSystem()
	{
		auto& resourceManager = ResourceManager::Get();

		m_quadVB = resourceManager.GetGeometryVertexBuffer("DefaultQuad");
		m_quadIB = resourceManager.GetGeometryIndexBuffer("DefaultQuad");

		m_vs = resourceManager.GetOrCreateVertexShader("Resource/Shader/Vertex/Particle_VS.hlsl");
		m_ps = resourceManager.GetOrCreatePixelShader("Resource/Shader/Pixel/Particle_PS.hlsl");

		m_inputLayout = nullptr;
		// Buffer 생성 (Structured Buffer)
		D3D11_BUFFER_DESC desc{};
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.ByteWidth = sizeof(ParticleStructuredData) * MAX_PARTICLES;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // SRV 바인딩
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED; // Structured
		desc.StructureByteStride = sizeof(ParticleStructuredData); // Stride 필수
		GraphicsDevice::Get().GetDevice()->CreateBuffer(&desc, nullptr, m_particleBuffer.GetAddressOf());
		// SRV 생성
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Format = DXGI_FORMAT_UNKNOWN; // StructuredBuffer는 UNKNOWN
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = MAX_PARTICLES;
		GraphicsDevice::Get().GetDevice()->CreateShaderResourceView(m_particleBuffer.Get(), &srvDesc, m_particleSRV.GetAddressOf());

		m_blendState = resourceManager.GetDefaultBlendState(DefaultBlendType::Additive);
		m_dsState = resourceManager.GetDefaultDepthStencilState(DefaultDepthStencilType::DepthRead);
	}

	void ParticleSystem::Update()
	{
		for (auto effect : m_components)
		{
			if (!effect->IsActive())
			{
				continue;
			}

			effect->Update();
		}
	}

	void ParticleSystem::Render(const Matrix& view, const Matrix& projection)
	{
		auto context = GraphicsDevice::Get().GetDeviceContext();

		context->VSSetShader(m_vs->GetRawShader(), nullptr, 0);
		context->PSSetShader(m_ps->GetRawShader(), nullptr, 0);

		context->IASetInputLayout(nullptr);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		ID3D11Buffer* nullBuffer = nullptr;
		static const UINT stride = 0;
		static const UINT offset = 0;

		context->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);
		context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);

		static constexpr float blendFactor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
		context->OMSetBlendState(m_blendState->GetRawBlendState(), blendFactor, 0xffffffff);
		context->OMSetDepthStencilState(m_dsState->GetRawDepthStencilState(), 0);

		for (auto effect : m_components)
		{
			if (!effect->IsActive())
			{
				continue;
			}

			for (auto& emitter : effect->GetEmitters())
			{
				const auto& particles = emitter.GetParticles();
				if (particles.empty())
				{
					continue;
				}

				auto tex = emitter.GetTexture();
				if (tex)
				{
					context->PSSetShaderResources(static_cast<UINT>(TextureSlot::BaseColor), 1, tex->GetSRV().GetAddressOf());
				}

				static std::vector<ParticleStructuredData> structuredData;
				structuredData.clear();

				size_t countToRender = particles.size();
				if (countToRender > MAX_PARTICLES)
				{
					countToRender = MAX_PARTICLES;
				}

				structuredData.reserve(countToRender);

				for (size_t i = 0; i < countToRender; ++i)
				{
					const auto& p = particles[i];

					ParticleStructuredData data;

					data.position = p.position;
					data.color = p.color;
					data.rotation = p.rotation;
					data.size = p.size;
					data.uvOffset = p.uvOffset;
					data.uvScale = p.uvScale;

					structuredData.push_back(data);
				}

				D3D11_MAPPED_SUBRESOURCE mapped;
				if (SUCCEEDED(context->Map(m_particleBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
				{
					std::memcpy(mapped.pData, structuredData.data(), sizeof(ParticleStructuredData) * structuredData.size());
					context->Unmap(m_particleBuffer.Get(), 0);
				}

				context->VSSetShaderResources(static_cast<UINT>(TextureSlot::ParticleStructured), 1, m_particleSRV.GetAddressOf());

				context->Draw(static_cast<UINT>(structuredData.size() * 6), 0);
			}
		}

		ID3D11ShaderResourceView* nullSRV = nullptr;
		context->VSSetShaderResources(0, 1, &nullSRV);
	}
}