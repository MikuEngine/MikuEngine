#include "EnginePCH.h"
#include "AfterimageRenderer.h"

#include "Framework/Object/Component/Renderer/SkeletalMeshRenderer.h"
#include "Framework/Asset/AssetManager.h"
#include "Framework/Asset/SkeletalMeshData.h"
#include "Framework/Asset/MaterialData.h"
#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Resource/VertexBuffer.h"
#include "Core/Graphics/Resource/IndexBuffer.h"
#include "Core/Graphics/Resource/ConstantBuffer.h"
#include "Core/Graphics/Resource/VertexShader.h"
#include "Core/Graphics/Resource/PixelShader.h"
#include "Core/Graphics/Resource/InputLayout.h"
#include "Core/Graphics/Resource/SamplerState.h"
#include "Core/Graphics/Resource/RasterizerState.h"
#include "Core/Graphics/Resource/MaterialHelper.h"
#include "Core/Graphics/Resource/BlendState.h"
#include "Core/Graphics/Resource/DepthStencilState.h"
#include "Core/Graphics/Data/Vertex.h"
#include "Core/Graphics/Data/ShaderSlotTypes.h"
#include "Core/Graphics/Data/ConstantBufferTypes.h"
#include "Core/Graphics/Device/GraphicsDevice.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Transform.h"

namespace engine
{
	void AfterimageRenderer::Initialize()
	{
		m_objectConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Object", sizeof(CbObject));
		m_materialConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Material", sizeof(CbMaterial));
		m_boneConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Bone", sizeof(CbBone));

		for (auto& m : m_boneTransformData.boneTransform)
		{
			m = Matrix::Identity;
		}

		m_samplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);
		m_rasterizerState = ResourceManager::Get().GetDefaultRasterizerState(DefaultRasterizerType::SolidBack);

		m_vs = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Skinned_VS.hlsl");
		m_transparentPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/LightTransparent_PS.hlsl");

		Renderer::Initialize();

		// 같은 GameObject의 SkeletalMeshRenderer를 소스로 자동 설정 (소스 미지정 시)
		if (!m_source && GetGameObject())
		{
			if (SkeletalMeshRenderer* smr = GetGameObject()->GetComponent<SkeletalMeshRenderer>())
			{
				SetSource(smr);
			}
		}
	}

	void AfterimageRenderer::SetSource(SkeletalMeshRenderer* source)
	{
		m_source = source;
		if (m_source)
		{
			m_meshFilePath = m_source->GetMeshPath();
			Refresh();
		}
		else
		{
			m_meshFilePath.clear();
			m_isRefreshed = false;
		}
	}

	void AfterimageRenderer::Refresh()
	{
		if (m_meshFilePath.empty())
		{
			m_meshData.reset();
			m_materialData.reset();
			m_vertexBuffer.reset();
			m_indexBuffer.reset();
			m_textures.clear();
			m_inputLayout.reset();
			m_isRefreshed = false;
			return;
		}

		m_meshData = AssetManager::Get().GetOrCreateSkeletalMeshData(m_meshFilePath);
		m_materialData = AssetManager::Get().GetOrCreateMaterialData(m_meshFilePath);

		if (m_meshData)
		{
			if (m_meshData->IsRigid())
			{
				m_vs = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Rigid_VS.hlsl");
				m_vertexBuffer = ResourceManager::Get().GetOrCreateVertexBuffer<CommonVertex>(m_meshFilePath, m_meshData->GetVertices());
				m_inputLayout = m_vs->GetOrCreateInputLayout<CommonVertex>();
			}
			else
			{
				m_vs = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Skinned_VS.hlsl");
				m_vertexBuffer = ResourceManager::Get().GetOrCreateVertexBuffer<BoneWeightVertex>(m_meshFilePath, m_meshData->GetBoneWeightVertices());
				m_inputLayout = m_vs->GetOrCreateInputLayout<BoneWeightVertex>();
			}
			m_indexBuffer = ResourceManager::Get().GetOrCreateIndexBuffer(m_meshFilePath, m_meshData->GetIndices());
		}

		if (m_materialData)
		{
			SetupTextures(m_materialData, m_textures);
		}

		m_isRefreshed = (m_meshData != nullptr && m_vertexBuffer != nullptr && m_indexBuffer != nullptr);
	}

	void AfterimageRenderer::Update()
	{
		// E-2: 알파 감쇠 (소스 없어도 기존 슬라이스는 감쇠)
		if (m_alphaDecaySpeed > 0.0f && !m_slices.empty())
		{
			const float dt = Time::DeltaTime();
			if (m_alphaDecayMode == AlphaDecayMode::Simultaneous)
			{
				for (auto& slice : m_slices)
				{
					slice.alpha -= m_alphaDecaySpeed * dt;
				}
				m_slices.erase(
					std::remove_if(m_slices.begin(), m_slices.end(),
						[](const AfterimageSlice& s) { return s.alpha <= 0.0f; }),
					m_slices.end());
			}
			else // Sequential: 가장 오래된 슬라이스(인덱스 0)만 감쇠, 0 이하면 제거
			{
				m_slices.front().alpha -= m_alphaDecaySpeed * dt;
				if (m_slices.front().alpha <= 0.0f)
				{
					m_slices.erase(m_slices.begin());
				}
			}
		}

		if (!m_source)
		{
			return;
		}
		// 소스가 나중에 메쉬를 설정한 경우 지연 Refresh
		if (!m_isRefreshed && !m_source->GetMeshPath().empty())
		{
			m_meshFilePath = m_source->GetMeshPath();
			Refresh();
		}
		// B-4: 소스에서 본 데이터 복사
		m_boneTransformData = m_source->GetBoneTransformData();
	}

	bool AfterimageRenderer::HasRenderType(RenderType type) const
	{
		return type == RenderType::Transparent;
	}

	void AfterimageRenderer::Draw(RenderType type) const
	{
		if (type != RenderType::Transparent)
		{
			return;
		}

		// D-1: 소스 없음·리소스 미로드·슬라이스 없으면 스킵
		if (!m_source || !m_isRefreshed || m_slices.empty() || !m_meshData || !m_materialData ||
			!m_vertexBuffer || !m_indexBuffer || !m_inputLayout || !m_vs || !m_transparentPS)
		{
			return;
		}

		const auto& deviceContext = GraphicsDevice::Get().GetDeviceContext();
		static const UINT s_vertexBufferOffset = 0;
		const UINT s_vertexBufferStride = m_vertexBuffer->GetBufferStride();

		// D-2: 파이프라인 설정 (VB, IB, InputLayout, Rasterizer, Sampler, Bone CB, Blend/Depth, VS/PS)
		deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer->GetBuffer().GetAddressOf(), &s_vertexBufferStride, &s_vertexBufferOffset);
		deviceContext->IASetIndexBuffer(m_indexBuffer->GetRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
		deviceContext->IASetInputLayout(m_inputLayout->GetRawInputLayout());
		deviceContext->RSSetState(m_rasterizerState->GetRawRasterizerState());
		deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerState->GetSamplerState().GetAddressOf());

		deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Bone), 1, m_boneConstantBuffer->GetBuffer().GetAddressOf());
		deviceContext->UpdateSubresource(m_boneConstantBuffer->GetRawBuffer(), 0, nullptr, &m_boneTransformData, 0, 0);

		static constexpr float blendFactor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
		auto blendState = ResourceManager::Get().GetDefaultBlendState(DefaultBlendType::AlphaBlend);
		auto depthState = ResourceManager::Get().GetDefaultDepthStencilState(DefaultDepthStencilType::DepthRead);
		deviceContext->OMSetBlendState(blendState->GetRawBlendState(), blendFactor, 0xFFFFFFFF);
		deviceContext->OMSetDepthStencilState(depthState->GetRawDepthStencilState(), 0);

		deviceContext->VSSetShader(m_vs->GetRawShader(), nullptr, 0);
		deviceContext->PSSetShader(m_transparentPS->GetRawShader(), nullptr, 0);
		deviceContext->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Material), 1, m_materialConstantBuffer->GetBuffer().GetAddressOf());
		deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Object), 1, m_objectConstantBuffer->GetBuffer().GetAddressOf());

		const auto& meshSections = m_meshData->GetMeshSections();
		const auto& materials = m_materialData->GetMaterials();

		// D-3, D-4: 슬라이스마다 월드·알파 설정 후 섹션 루프에서 DrawIndexed
		for (const auto& slice : m_slices)
		{
			CbObject cbObject{};
			cbObject.world = slice.world.Transpose();
			cbObject.worldInverseTranspose = slice.world.Invert().Transpose();
			cbObject.boneIndex = -1;

			CbMaterial cbMaterial{};
			cbMaterial.materialBaseColor = Vector4(1.0f, 1.0f, 1.0f, slice.alpha);
			cbMaterial.materialEmissive = Vector3(1.0f, 1.0f, 1.0f);
			cbMaterial.materialRoughness = 0.0f;
			cbMaterial.materialMetalness = 0.0f;
			cbMaterial.materialAmbientOcclusion = 1.0f;
			cbMaterial.materialEmissiveIntensity = 0.0f;
			cbMaterial.overrideMaterial = 1;

			deviceContext->UpdateSubresource(m_materialConstantBuffer->GetRawBuffer(), 0, nullptr, &cbMaterial, 0, 0);

			for (const auto& section : meshSections)
			{
				if (materials[section.materialIndex].renderType != MaterialRenderType::Transparent)
				{
					continue;
				}
				const auto textureSRVs = m_textures[section.materialIndex].AsRawArray();
				deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::BaseColor), static_cast<UINT>(textureSRVs.size()), textureSRVs.data());

				if (m_meshData->IsRigid())
				{
					cbObject.boneIndex = static_cast<int>(section.boneIndex);
				}
				else
				{
					cbObject.boneIndex = -1;
				}
				deviceContext->UpdateSubresource(m_objectConstantBuffer->GetRawBuffer(), 0, nullptr, &cbObject, 0, 0);
				deviceContext->DrawIndexed(section.indexCount, section.indexOffset, section.vertexOffset);
			}
		}

		// D-5: Blend/Depth state 원복
		deviceContext->OMSetBlendState(nullptr, blendFactor, 0xFFFFFFFF);
		deviceContext->OMSetDepthStencilState(nullptr, 0);
	}

	DirectX::BoundingBox AfterimageRenderer::GetBounds() const
	{
		return DirectX::BoundingBox();
	}

	void AfterimageRenderer::BeginRecording()
	{
		m_isRecording = true;
		// (선택) 기존 슬라이스 클리어 – 새 대시마다 깨끗하게 시작
		m_slices.clear();
	}

	void AfterimageRenderer::RecordSample()
	{
		if (!m_isRecording || !GetTransform())
		{
			return;
		}
		AfterimageSlice slice;
		slice.world = GetTransform()->GetWorld();
		slice.alpha = m_initialAlpha;
		m_slices.push_back(slice);
		if (m_slices.size() > m_maxSlices)
		{
			m_slices.erase(m_slices.begin());
		}
	}

	void AfterimageRenderer::RecordSample(const Matrix& world)
	{
		if (!m_isRecording)
		{
			return;
		}
		AfterimageSlice slice;
		slice.world = world;
		slice.alpha = m_initialAlpha;
		m_slices.push_back(slice);
		if (m_slices.size() > m_maxSlices)
		{
			m_slices.erase(m_slices.begin());
		}
	}

	void AfterimageRenderer::EndRecording()
	{
		m_isRecording = false;
	}

	void AfterimageRenderer::ClearSlices()
	{
		m_slices.clear();
	}

	void AfterimageRenderer::RecordTeleportPath(const Matrix& fromWorld, const Matrix& toWorld, size_t numSlices)
	{
		if (numSlices == 0)
		{
			return;
		}
		const Vector3 fromPos = fromWorld.Translation();
		const Vector3 toPos = toWorld.Translation();
		for (size_t i = 1; i <= numSlices; ++i)
		{
			const float t = static_cast<float>(i) / static_cast<float>(numSlices + 1);
			const Vector3 pos = Vector3::Lerp(fromPos, toPos, t);
			Matrix sliceWorld = toWorld;
			sliceWorld._41 = pos.x;
			sliceWorld._42 = pos.y;
			sliceWorld._43 = pos.z;
			AfterimageSlice slice;
			slice.world = sliceWorld;
			slice.alpha = m_initialAlpha;
			m_slices.push_back(slice);
			while (m_slices.size() > m_maxSlices)
			{
				m_slices.erase(m_slices.begin());
			}
		}
	}

	void AfterimageRenderer::SetInitialAlpha(float alpha)
	{
		m_initialAlpha = std::clamp(alpha, 0.0f, 1.0f);
	}

	void AfterimageRenderer::SetAlphaDecaySpeed(float speed)
	{
		m_alphaDecaySpeed = std::max(0.0f, speed);
	}

	void AfterimageRenderer::SetMaxSlices(size_t count)
	{
		m_maxSlices = std::clamp(count, AFTERIMAGE_MIN_SLICES, AFTERIMAGE_MAX_SLICES_CAP);
		while (m_slices.size() > m_maxSlices)
		{
			m_slices.erase(m_slices.begin());
		}
	}

	void AfterimageRenderer::OnGui()
	{
		ImGui::Indent();

		// 소스 상태
		if (m_source)
		{
			ImGui::Text("Source: SkeletalMeshRenderer (valid)");
			ImGui::Text("Mesh: %s", m_meshFilePath.c_str());
		}
		else
		{
			ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "Source: (none) - same GameObject SkeletalMeshRenderer will auto-set");
		}

		// 같은 GameObject의 SkeletalMeshRenderer로 소스 설정
		if (GetGameObject() && ImGui::Button("Set Source (Same GameObject)"))
		{
			if (SkeletalMeshRenderer* smr = GetGameObject()->GetComponent<SkeletalMeshRenderer>())
			{
				SetSource(smr);
			}
		}

		ImGui::SeparatorText("Afterimage");

		float initialAlpha = m_initialAlpha;
		if (ImGui::SliderFloat("Initial Alpha", &initialAlpha, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
		{
			SetInitialAlpha(initialAlpha);
		}

		float decaySpeed = m_alphaDecaySpeed;
		if (ImGui::DragFloat("Alpha Decay Speed", &decaySpeed, 0.1f, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
		{
			SetAlphaDecaySpeed(decaySpeed);
		}

		int decayMode = static_cast<int>(m_alphaDecayMode);
		const char* decayModeNames[] = { "Simultaneous (all fade together)", "Sequential (oldest fades first)" };
		if (ImGui::Combo("Alpha Decay Mode", &decayMode, decayModeNames, 2))
		{
			SetAlphaDecayMode(static_cast<AlphaDecayMode>(decayMode));
		}

		int maxSlices = static_cast<int>(m_maxSlices);
		if (ImGui::SliderInt("Max Slices", &maxSlices, static_cast<int>(AFTERIMAGE_MIN_SLICES), static_cast<int>(AFTERIMAGE_MAX_SLICES_CAP), "%d", ImGuiSliderFlags_AlwaysClamp))
		{
			SetMaxSlices(static_cast<size_t>(maxSlices));
		}
		ImGui::Text("Slices: %zu / %zu", m_slices.size(), m_maxSlices);
		ImGui::Text("Recording: %s", m_isRecording ? "Yes" : "No");

		ImGui::Unindent();
	}

	void AfterimageRenderer::Save(json& j) const
	{
		Object::Save(j);

		j["InitialAlpha"] = m_initialAlpha;
		j["AlphaDecaySpeed"] = m_alphaDecaySpeed;
		j["AlphaDecayMode"] = static_cast<int>(m_alphaDecayMode);
		j["MaxSlices"] = m_maxSlices;

		if (m_source)
		{
			Handle h = m_source.GetHandle();
			j["SourceHandleIndex"] = h.index;
			j["SourceHandleGeneration"] = h.generation;
		}
	}

	void AfterimageRenderer::Load(const json& j)
	{
		Object::Load(j);

		JsonGet(j, "InitialAlpha", m_initialAlpha, 0.7f);
		JsonGet(j, "AlphaDecaySpeed", m_alphaDecaySpeed, 1.5f);
		int decayMode = static_cast<int>(m_alphaDecayMode);
		JsonGet(j, "AlphaDecayMode", decayMode, 0);
		m_alphaDecayMode = (decayMode == 1) ? AlphaDecayMode::Sequential : AlphaDecayMode::Simultaneous;
		JsonGet(j, "MaxSlices", m_maxSlices, static_cast<size_t>(AFTERIMAGE_DEFAULT_MAX_SLICES));

		m_initialAlpha = std::clamp(m_initialAlpha, 0.0f, 1.0f);
		m_alphaDecaySpeed = std::max(0.0f, m_alphaDecaySpeed);
		m_maxSlices = std::clamp(m_maxSlices, AFTERIMAGE_MIN_SLICES, AFTERIMAGE_MAX_SLICES_CAP);

		if (j.contains("SourceHandleIndex") && j.contains("SourceHandleGeneration"))
		{
			Handle h;
			h.index = j["SourceHandleIndex"].get<std::uint32_t>();
			h.generation = j["SourceHandleGeneration"].get<std::uint32_t>();
			m_source = Ptr<SkeletalMeshRenderer>(h);
			if (m_source)
			{
				m_meshFilePath = m_source->GetMeshPath();
				Refresh();
			}
		}
	}
}
