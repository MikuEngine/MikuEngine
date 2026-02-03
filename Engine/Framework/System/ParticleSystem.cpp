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
#include "Framework/System/SystemManager.h"
#include "Framework/System/CameraSystem.h"
#include "Framework/System/RenderSystem.h"  // TransparentRenderItem 정의를 위해
#include "Framework/Object/Component/Transform.h"

namespace engine
{
	void ParticleSystem::Initialize()
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

		m_blendStateAddtive = resourceManager.GetDefaultBlendState(DefaultBlendType::Additive);
		m_blendStateBlend = resourceManager.GetDefaultBlendState(DefaultBlendType::AlphaBlend);
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

	void ParticleSystem::CollectTransparentItems(
		std::vector<TransparentRenderItem>& items,
		const Vector3& cameraPos)
	{
		for (auto effect : m_components)
		{
			if (!effect->IsActive())
			{
				continue;
			}

			// 파티클 정렬
			effect->SortParticles();

			Vector3 effectPos = effect->GetTransform()->GetWorldPosition();
			float effectDistSq = Vector3::DistanceSquared(effectPos, cameraPos);

			for (auto& emitter : effect->GetEmitters())
			{
				const auto& particles = emitter.GetParticles();
				if (particles.empty())
				{
					continue;
				}

				// 가장 가까운 파티클의 거리 사용
				float emitterDistSq = particles.back().distanceToCamera;

				TransparentRenderItem item;
				item.type = TransparentRenderItem::Type::ParticleEmitter;
				item.distanceSq = emitterDistSq;
				item.particle.effect = effect;
				item.particle.emitter = &emitter;

				items.push_back(item);
			}
		}
	}

	void ParticleSystem::RenderEmitter(ParticleEffect* effect, const ParticleEmitter* emitter)
	{
		auto context = GraphicsDevice::Get().GetDeviceContext();

		// 셰이더 설정 (이미 Render()에서 설정되어 있을 수 있지만, 안전을 위해)
		context->VSSetShader(m_vs->GetRawShader(), nullptr, 0);
		context->PSSetShader(m_ps->GetRawShader(), nullptr, 0);

		context->IASetInputLayout(nullptr);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		ID3D11Buffer* nullBuffer = nullptr;
		static const UINT stride = 0;
		static const UINT offset = 0;

		context->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);
		context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);

		auto blendType = emitter->GetProps().blend;

		static constexpr float blendFactor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };

		switch (blendType)
		{
		case EmitterBlend::Additive:
			context->OMSetBlendState(m_blendStateAddtive->GetRawBlendState(), blendFactor, 0xffffffff);
			break;

		case EmitterBlend::Blend:
			context->OMSetBlendState(m_blendStateBlend->GetRawBlendState(), blendFactor, 0xffffffff);
			break;
		}
		context->OMSetDepthStencilState(m_dsState->GetRawDepthStencilState(), 0);

		const auto& particles = emitter->GetParticles();
		if (particles.empty())
		{
			return;
		}

		auto tex = emitter->GetTexture();
		if (tex)
		{
			context->PSSetShaderResources(static_cast<UINT>(TextureSlot::BaseColor), 1, tex->GetSRV().GetAddressOf());
		}

		size_t countToRender = particles.size();
		if (countToRender > MAX_PARTICLES)
		{
			countToRender = MAX_PARTICLES;
		}

		// World 스케일 중 가장 큰 값으로 파티클 크기 일괄 적용
		Vector3 worldScale = effect->GetTransform()->GetWorldScale();
		float uniformScale = std::max({ worldScale.x, worldScale.y, worldScale.z });

		m_structuredDataBuffer.clear();
		m_structuredDataBuffer.reserve(countToRender);

		for (size_t i = 0; i < countToRender; ++i)
		{
			const auto& p = particles[i];

			ParticleStructuredData data;

			data.position = p.position;
			data.color = p.color;
			data.emissiveColor = p.emissiveColor;
			data.emissiveIntensity = p.emissiveIntensity;
			data.rotation = p.rotation;
			data.size = p.size * uniformScale;
			data.uvOffset = p.uvOffset;
			data.uvScale = p.uvScale;

			m_structuredDataBuffer.push_back(data);
		}

		D3D11_MAPPED_SUBRESOURCE mapped;
		if (SUCCEEDED(context->Map(m_particleBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			std::memcpy(mapped.pData, m_structuredDataBuffer.data(), sizeof(ParticleStructuredData) * m_structuredDataBuffer.size());
			context->Unmap(m_particleBuffer.Get(), 0);
		}

		context->VSSetShaderResources(static_cast<UINT>(TextureSlot::ParticleStructured), 1, m_particleSRV.GetAddressOf());

		context->Draw(static_cast<UINT>(m_structuredDataBuffer.size() * 6), 0);

		context->RSSetState(nullptr);
		context->OMSetDepthStencilState(nullptr, 0);
	}
}