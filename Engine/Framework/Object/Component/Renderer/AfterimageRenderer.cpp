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
#include "Core/System/MyTime.h"

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
		m_emissivePS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/EmissiveTransparent_PS.hlsl");
		m_silhouettePS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Silhouette_PS.hlsl");

		// 2패스 실루엣: 1패스 스텐실 쓰기, 2패스 스텐실 테스트 후 단색 채우기
		{
			D3D11_DEPTH_STENCIL_DESC desc{};
			desc.DepthEnable = TRUE;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_LESS;
			desc.StencilEnable = TRUE;
			desc.StencilReadMask = 0xFF;
			desc.StencilWriteMask = 0xFF;
			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			desc.BackFace = desc.FrontFace;
			m_dssStencilWrite = ResourceManager::Get().GetOrCreateDepthStencilState("AfterimageStencilWrite", desc);
		}
		{
			D3D11_DEPTH_STENCIL_DESC desc{};
			desc.DepthEnable = FALSE;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
			desc.DepthFunc = D3D11_COMPARISON_LESS;
			desc.StencilEnable = TRUE;
			desc.StencilReadMask = 0xFF;
			desc.StencilWriteMask = 0xFF;
			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_ZERO;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
			desc.BackFace = desc.FrontFace;
			m_dssStencilTest = ResourceManager::Get().GetOrCreateDepthStencilState("AfterimageStencilTest", desc);
		}
		{
			D3D11_BLEND_DESC desc{};
			desc.RenderTarget[0].RenderTargetWriteMask = 0;
			m_blendColorWriteNone = ResourceManager::Get().GetOrCreateBlendState("AfterimageColorWriteNone", desc);
		}

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
		const float dt = Time::DeltaTime();

		// 솔리드 레이어 감쇠 (선형: alpha -= speed*dt, 지수: alpha *= exp(-speed*dt), 임계값 이하 제거)
		const float solidThreshold = (m_solidDecayCurve == DecayCurve::Exponential) ? DECAY_REMOVE_THRESHOLD : 0.0f;
		if (m_solidDecaySpeed > 0.0f && !m_slicesSolid.empty())
		{
			if (m_solidDecayMode == AlphaDecayMode::Simultaneous)
			{
				if (m_solidDecayCurve == DecayCurve::Exponential)
				{
					const float factor = std::exp(-m_solidDecaySpeed * dt);
					for (auto& slice : m_slicesSolid)
						slice.alpha *= factor;
				}
				else
				{
					for (auto& slice : m_slicesSolid)
						slice.alpha -= m_solidDecaySpeed * dt;
				}
				m_slicesSolid.erase(
					std::remove_if(m_slicesSolid.begin(), m_slicesSolid.end(),
						[solidThreshold](const AfterimageSlice& s) { return s.alpha <= solidThreshold; }),
					m_slicesSolid.end());
			}
			else
			{
				if (m_solidDecayCurve == DecayCurve::Exponential)
					m_slicesSolid.front().alpha *= std::exp(-m_solidDecaySpeed * dt);
				else
					m_slicesSolid.front().alpha -= m_solidDecaySpeed * dt;
				if (m_slicesSolid.front().alpha <= solidThreshold)
					m_slicesSolid.erase(m_slicesSolid.begin());
			}
		}

		// 알파 레이어 감쇠 (동일: 선형/지수 + 임계값 이하 제거)
		const float alphaThreshold = (m_alphaDecayCurve == DecayCurve::Exponential) ? DECAY_REMOVE_THRESHOLD : 0.0f;
		if (m_alphaDecaySpeed > 0.0f && !m_slicesAlpha.empty())
		{
			if (m_alphaDecayMode == AlphaDecayMode::Simultaneous)
			{
				if (m_alphaDecayCurve == DecayCurve::Exponential)
				{
					const float factor = std::exp(-m_alphaDecaySpeed * dt);
					for (auto& slice : m_slicesAlpha)
						slice.alpha *= factor;
				}
				else
				{
					for (auto& slice : m_slicesAlpha)
						slice.alpha -= m_alphaDecaySpeed * dt;
				}
				m_slicesAlpha.erase(
					std::remove_if(m_slicesAlpha.begin(), m_slicesAlpha.end(),
						[alphaThreshold](const AfterimageSlice& s) { return s.alpha <= alphaThreshold; }),
					m_slicesAlpha.end());
			}
			else
			{
				if (m_alphaDecayCurve == DecayCurve::Exponential)
					m_slicesAlpha.front().alpha *= std::exp(-m_alphaDecaySpeed * dt);
				else
					m_slicesAlpha.front().alpha -= m_alphaDecaySpeed * dt;
				if (m_slicesAlpha.front().alpha <= alphaThreshold)
					m_slicesAlpha.erase(m_slicesAlpha.begin());
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

		// D-1: 소스 없음·리소스 미로드 시 스킵
		if (!m_source || !m_isRefreshed || !m_meshData || !m_materialData ||
			!m_vertexBuffer || !m_indexBuffer || !m_inputLayout || !m_vs || !m_transparentPS || !m_emissivePS ||
			!m_silhouettePS || !m_dssStencilWrite || !m_dssStencilTest || !m_blendColorWriteNone)
		{
			return;
		}
		if (!m_drawSolidLayer && !m_drawAlphaLayer)
			return;
		if ((!m_drawSolidLayer || m_slicesSolid.empty()) && (!m_drawAlphaLayer || m_slicesAlpha.empty()))
			return;

		const auto& deviceContext = GraphicsDevice::Get().GetDeviceContext();
		static const UINT s_vertexBufferOffset = 0;
		const UINT s_vertexBufferStride = m_vertexBuffer->GetBufferStride();
		static constexpr float blendFactor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
		const auto& meshSections = m_meshData->GetMeshSections();
		constexpr UINT stencilRef = 1;

		auto restoreMeshPipeline = [&]()
		{
			deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			deviceContext->IASetVertexBuffers(0, 1, m_vertexBuffer->GetBuffer().GetAddressOf(), &s_vertexBufferStride, &s_vertexBufferOffset);
			deviceContext->IASetIndexBuffer(m_indexBuffer->GetRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
			deviceContext->IASetInputLayout(m_inputLayout->GetRawInputLayout());
			deviceContext->VSSetShader(m_vs->GetRawShader(), nullptr, 0);
			deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Object), 1, m_objectConstantBuffer->GetBuffer().GetAddressOf());
			deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Bone), 1, m_boneConstantBuffer->GetBuffer().GetAddressOf());
		};

		// D-2: 공통 파이프라인 (VB, IB, InputLayout, Rasterizer, Sampler, VS, Object/Bone CB)
		restoreMeshPipeline();
		deviceContext->RSSetState(m_rasterizerState->GetRawRasterizerState());
		deviceContext->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, m_samplerState->GetSamplerState().GetAddressOf());
		deviceContext->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Material), 1, m_materialConstantBuffer->GetBuffer().GetAddressOf());

		// 2패스 실루엣: 슬라이스마다 1패스(스텐실 쓰기, 컬러 끔) → 2패스(풀스크린 단색, 스텐실 테스트) → 알파 중첩 없음
		auto drawSlicePass1Mesh = [&](const AfterimageSlice& slice)
		{
			CbObject cbObject{};
			cbObject.world = slice.world.Transpose();
			cbObject.worldInverseTranspose = slice.world.Invert().Transpose();
			cbObject.boneIndex = -1;
			for (const auto& section : meshSections)
			{
				const auto textureSRVs = m_textures[section.materialIndex].AsRawArray();
				deviceContext->PSSetShaderResources(static_cast<UINT>(TextureSlot::BaseColor), static_cast<UINT>(textureSRVs.size()), textureSRVs.data());
				if (m_meshData->IsRigid())
					cbObject.boneIndex = static_cast<int>(section.boneIndex);
				else
					cbObject.boneIndex = -1;
				deviceContext->UpdateSubresource(m_objectConstantBuffer->GetRawBuffer(), 0, nullptr, &cbObject, 0, 0);
				deviceContext->DrawIndexed(section.indexCount, section.indexOffset, section.vertexOffset);
			}
		};

		// 솔리드 레이어: 슬라이스마다 2패스
		auto blendPremul = ResourceManager::Get().GetDefaultBlendState(DefaultBlendType::AlphaBlendPremultiplied);
		if (m_drawSolidLayer && !m_slicesSolid.empty())
		{
			for (const auto& slice : m_slicesSolid)
			{
				const CbBone* boneData = slice.boneSnapshot.has_value() ? &*slice.boneSnapshot : &m_boneTransformData;
				deviceContext->UpdateSubresource(m_boneConstantBuffer->GetRawBuffer(), 0, nullptr, boneData, 0, 0);
				deviceContext->OMSetBlendState(m_blendColorWriteNone->GetRawBlendState(), blendFactor, 0xFFFFFFFF);
				deviceContext->OMSetDepthStencilState(m_dssStencilWrite->GetRawDepthStencilState(), stencilRef);
				deviceContext->PSSetShader(m_emissivePS->GetRawShader(), nullptr, 0);
				drawSlicePass1Mesh(slice);

				deviceContext->OMSetDepthStencilState(m_dssStencilTest->GetRawDepthStencilState(), stencilRef);
				deviceContext->OMSetBlendState(blendPremul->GetRawBlendState(), blendFactor, 0xFFFFFFFF);
				CbMaterial cbMat{};
				cbMat.materialBaseColor = Vector4(m_solidColor.x * slice.alpha, m_solidColor.y * slice.alpha, m_solidColor.z * slice.alpha, slice.alpha);
				cbMat.materialEmissive = Vector3(m_solidColor.x, m_solidColor.y, m_solidColor.z);
				cbMat.materialEmissiveIntensity = m_solidEmissiveIntensity;
				cbMat.materialAlpha = 1.0f;
				deviceContext->UpdateSubresource(m_materialConstantBuffer->GetRawBuffer(), 0, nullptr, &cbMat, 0, 0);
				deviceContext->PSSetShader(m_silhouettePS->GetRawShader(), nullptr, 0);
				GraphicsDevice::Get().DrawFullscreenQuad();
				restoreMeshPipeline();
			}
		}
		// 알파 레이어: 슬라이스마다 2패스
		auto blendAlpha = ResourceManager::Get().GetDefaultBlendState(DefaultBlendType::AlphaBlend);
		if (m_drawAlphaLayer && !m_slicesAlpha.empty())
		{
			for (const auto& slice : m_slicesAlpha)
			{
				const CbBone* boneData = slice.boneSnapshot.has_value() ? &*slice.boneSnapshot : &m_boneTransformData;
				deviceContext->UpdateSubresource(m_boneConstantBuffer->GetRawBuffer(), 0, nullptr, boneData, 0, 0);
				deviceContext->OMSetBlendState(m_blendColorWriteNone->GetRawBlendState(), blendFactor, 0xFFFFFFFF);
				deviceContext->OMSetDepthStencilState(m_dssStencilWrite->GetRawDepthStencilState(), stencilRef);
				deviceContext->PSSetShader(m_transparentPS->GetRawShader(), nullptr, 0);
				drawSlicePass1Mesh(slice);

				const float alpha = slice.alpha * m_alphaTint.w;
				deviceContext->OMSetDepthStencilState(m_dssStencilTest->GetRawDepthStencilState(), stencilRef);
				deviceContext->OMSetBlendState(blendAlpha->GetRawBlendState(), blendFactor, 0xFFFFFFFF);
				CbMaterial cbMat{};
				cbMat.materialBaseColor = Vector4(m_alphaTint.x, m_alphaTint.y, m_alphaTint.z, alpha);
				cbMat.materialEmissive = Vector3(m_alphaTint.x, m_alphaTint.y, m_alphaTint.z);
				cbMat.materialEmissiveIntensity = m_alphaEmissiveIntensity;
				cbMat.materialAlpha = 1.0f;
				deviceContext->UpdateSubresource(m_materialConstantBuffer->GetRawBuffer(), 0, nullptr, &cbMat, 0, 0);
				deviceContext->PSSetShader(m_silhouettePS->GetRawShader(), nullptr, 0);
				GraphicsDevice::Get().DrawFullscreenQuad();
				restoreMeshPipeline();
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
		m_solidLastSampleTime = 0.0f;
		m_alphaLastSampleTime = 0.0f;
		m_slicesSolid.clear();
		m_slicesAlpha.clear();
	}

	void AfterimageRenderer::RecordSample()
	{
		if (!m_isRecording || !GetTransform())
			return;
		const Matrix world = GetTransform()->GetWorld();
		const float now = Time::UnscaledTime();

		if (m_drawSolidLayer)
		{
			if (m_solidSampleInterval <= 0.0f || m_solidLastSampleTime == 0.0f || (now - m_solidLastSampleTime) >= m_solidSampleInterval)
			{
				m_solidLastSampleTime = now;
				for (auto& s : m_slicesSolid)
					s.alpha *= m_solidTrailGradient;
				AfterimageSlice slice;
				slice.world = world;
				slice.alpha = m_solidInitialAlpha;
				if (m_source && m_meshData && !m_meshData->IsRigid())
					slice.boneSnapshot = m_source->GetBoneTransformData();
				m_slicesSolid.push_back(slice);
				while (m_slicesSolid.size() > m_solidMaxSlices)
					m_slicesSolid.erase(m_slicesSolid.begin());
			}
		}
		if (m_drawAlphaLayer)
		{
			if (m_alphaSampleInterval <= 0.0f || m_alphaLastSampleTime == 0.0f || (now - m_alphaLastSampleTime) >= m_alphaSampleInterval)
			{
				m_alphaLastSampleTime = now;
				for (auto& s : m_slicesAlpha)
					s.alpha *= m_alphaTrailGradient;
				AfterimageSlice slice;
				slice.world = world;
				slice.alpha = m_alphaInitialAlpha;
				if (m_source && m_meshData && !m_meshData->IsRigid())
					slice.boneSnapshot = m_source->GetBoneTransformData();
				m_slicesAlpha.push_back(slice);
				while (m_slicesAlpha.size() > m_alphaMaxSlices)
					m_slicesAlpha.erase(m_slicesAlpha.begin());
			}
		}
	}

	void AfterimageRenderer::RecordSample(const Matrix& world)
	{
		if (!m_isRecording)
			return;
		const float now = Time::UnscaledTime();

		if (m_drawSolidLayer)
		{
			if (m_solidSampleInterval <= 0.0f || m_solidLastSampleTime == 0.0f || (now - m_solidLastSampleTime) >= m_solidSampleInterval)
			{
				m_solidLastSampleTime = now;
				for (auto& s : m_slicesSolid)
					s.alpha *= m_solidTrailGradient;
				AfterimageSlice slice;
				slice.world = world;
				slice.alpha = m_solidInitialAlpha;
				if (m_source && m_meshData && !m_meshData->IsRigid())
					slice.boneSnapshot = m_source->GetBoneTransformData();
				m_slicesSolid.push_back(slice);
				while (m_slicesSolid.size() > m_solidMaxSlices)
					m_slicesSolid.erase(m_slicesSolid.begin());
			}
		}
		if (m_drawAlphaLayer)
		{
			if (m_alphaSampleInterval <= 0.0f || m_alphaLastSampleTime == 0.0f || (now - m_alphaLastSampleTime) >= m_alphaSampleInterval)
			{
				m_alphaLastSampleTime = now;
				for (auto& s : m_slicesAlpha)
					s.alpha *= m_alphaTrailGradient;
				AfterimageSlice slice;
				slice.world = world;
				slice.alpha = m_alphaInitialAlpha;
				if (m_source && m_meshData && !m_meshData->IsRigid())
					slice.boneSnapshot = m_source->GetBoneTransformData();
				m_slicesAlpha.push_back(slice);
				while (m_slicesAlpha.size() > m_alphaMaxSlices)
					m_slicesAlpha.erase(m_slicesAlpha.begin());
			}
		}
	}

	void AfterimageRenderer::EndRecording()
	{
		m_isRecording = false;
	}

	void AfterimageRenderer::ClearSlices()
	{
		m_slicesSolid.clear();
		m_slicesAlpha.clear();
	}

	void AfterimageRenderer::RecordTeleportPath(const Matrix& fromWorld, const Matrix& toWorld, size_t numSlices)
	{
		if (numSlices == 0)
			return;
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
			AfterimageSlice sliceSolid;
			sliceSolid.world = sliceWorld;
			sliceSolid.alpha = m_solidInitialAlpha;
			AfterimageSlice sliceAlpha;
			sliceAlpha.world = sliceWorld;
			sliceAlpha.alpha = m_alphaInitialAlpha;
			if (m_source && m_meshData && !m_meshData->IsRigid())
			{
				sliceSolid.boneSnapshot = m_source->GetBoneTransformData();
				sliceAlpha.boneSnapshot = m_source->GetBoneTransformData();
			}
			if (m_drawSolidLayer)
			{
				for (auto& s : m_slicesSolid)
					s.alpha *= m_solidTrailGradient;
				m_slicesSolid.push_back(sliceSolid);
				while (m_slicesSolid.size() > m_solidMaxSlices)
					m_slicesSolid.erase(m_slicesSolid.begin());
			}
			if (m_drawAlphaLayer)
			{
				for (auto& s : m_slicesAlpha)
					s.alpha *= m_alphaTrailGradient;
				m_slicesAlpha.push_back(sliceAlpha);
				while (m_slicesAlpha.size() > m_alphaMaxSlices)
					m_slicesAlpha.erase(m_slicesAlpha.begin());
			}
		}
	}

	void AfterimageRenderer::SetSolidMaxSlices(size_t count)
	{
		m_solidMaxSlices = std::clamp(count, AFTERIMAGE_MIN_SLICES, AFTERIMAGE_MAX_SLICES_CAP);
		while (m_slicesSolid.size() > m_solidMaxSlices)
			m_slicesSolid.erase(m_slicesSolid.begin());
	}

	void AfterimageRenderer::SetSolidSampleInterval(float seconds)
	{
		m_solidSampleInterval = std::max(0.0f, seconds);
	}

	void AfterimageRenderer::SetSolidInitialAlpha(float alpha)
	{
		m_solidInitialAlpha = std::clamp(alpha, 0.0f, 1.0f);
	}

	void AfterimageRenderer::SetSolidDecaySpeed(float speed)
	{
		m_solidDecaySpeed = std::max(0.0f, speed);
	}

	void AfterimageRenderer::SetSolidEmissiveIntensity(float intensity)
	{
		m_solidEmissiveIntensity = std::max(0.0f, intensity);
	}

	void AfterimageRenderer::SetSolidTrailGradient(float gradient)
	{
		m_solidTrailGradient = std::clamp(gradient, 0.0f, 1.0f);
	}

	void AfterimageRenderer::SetAlphaInitialAlpha(float alpha)
	{
		m_alphaInitialAlpha = std::clamp(alpha, 0.0f, 1.0f);
	}

	void AfterimageRenderer::SetAlphaDecaySpeed(float speed)
	{
		m_alphaDecaySpeed = std::max(0.0f, speed);
	}

	void AfterimageRenderer::SetAlphaMaxSlices(size_t count)
	{
		m_alphaMaxSlices = std::clamp(count, AFTERIMAGE_MIN_SLICES, AFTERIMAGE_MAX_SLICES_CAP);
		while (m_slicesAlpha.size() > m_alphaMaxSlices)
			m_slicesAlpha.erase(m_slicesAlpha.begin());
	}

	void AfterimageRenderer::SetAlphaSampleInterval(float seconds)
	{
		m_alphaSampleInterval = std::max(0.0f, seconds);
	}

	void AfterimageRenderer::SetSolidColor(const Vector4& color)
	{
		m_solidColor = color;
		m_solidColor.w = 1.0f;
	}

	void AfterimageRenderer::SetAlphaTint(const Vector4& color)
	{
		m_alphaTint = color;
		m_alphaTint.w = std::clamp(m_alphaTint.w, 0.0f, 1.0f);
	}

	void AfterimageRenderer::SetAlphaEmissiveIntensity(float intensity)
	{
		m_alphaEmissiveIntensity = std::max(0.0f, intensity);
	}

	void AfterimageRenderer::SetAlphaTrailGradient(float gradient)
	{
		m_alphaTrailGradient = std::clamp(gradient, 0.0f, 1.0f);
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

		ImGui::Text("Recording: %s", m_isRecording ? "Yes" : "No");

		ImGui::SeparatorText("Solid Layer (emissive, no lighting)");
		bool drawSolid = m_drawSolidLayer;
		if (ImGui::Checkbox("Draw Solid Layer", &drawSolid))
			SetDrawSolidLayer(drawSolid);
		if (m_drawSolidLayer)
		{
			float sc[4] = { m_solidColor.x, m_solidColor.y, m_solidColor.z, 1.0f };
			if (ImGui::ColorEdit3("Solid Color", sc, ImGuiColorEditFlags_NoInputs))
				SetSolidColor(Vector4(sc[0], sc[1], sc[2], 1.0f));
			float solidInitialAlpha = m_solidInitialAlpha;
			if (ImGui::SliderFloat("Solid Initial Alpha", &solidInitialAlpha, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
				SetSolidInitialAlpha(solidInitialAlpha);
			float solidDecay = m_solidDecaySpeed;
			if (ImGui::DragFloat("Solid Decay Speed", &solidDecay, 0.1f, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
				SetSolidDecaySpeed(solidDecay);
			int solidDecayMode = static_cast<int>(m_solidDecayMode);
			const char* solidDecayNames[] = { "Simultaneous", "Sequential" };
			if (ImGui::Combo("Solid Decay Mode", &solidDecayMode, solidDecayNames, 2))
				SetSolidDecayMode(static_cast<AlphaDecayMode>(solidDecayMode));
			int solidCurve = static_cast<int>(m_solidDecayCurve);
			const char* solidCurveNames[] = { "Linear", "Exponential" };
			if (ImGui::Combo("Solid Decay Curve", &solidCurve, solidCurveNames, 2))
				SetSolidDecayCurve(static_cast<DecayCurve>(solidCurve));
			float solidEmissive = m_solidEmissiveIntensity;
			if (ImGui::DragFloat("Solid Emissive Intensity", &solidEmissive, 0.1f, 0.0f, 100.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
				SetSolidEmissiveIntensity(solidEmissive);
			ImGui::TextUnformatted("(1 = normal, >1 = brighter)");
			float solidGrad = m_solidTrailGradient;
			if (ImGui::SliderFloat("Solid Trail Gradient", &solidGrad, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
				SetSolidTrailGradient(solidGrad);
			ImGui::TextUnformatted("(1 = same alpha, <1 = front fades first)");
			int solidMax = static_cast<int>(m_solidMaxSlices);
			if (ImGui::SliderInt("Solid Max Slices", &solidMax, static_cast<int>(AFTERIMAGE_MIN_SLICES), static_cast<int>(AFTERIMAGE_MAX_SLICES_CAP), "%d", ImGuiSliderFlags_AlwaysClamp))
				SetSolidMaxSlices(static_cast<size_t>(solidMax));
			float solidInterval = m_solidSampleInterval;
			if (ImGui::DragFloat("Solid Sample Interval (s)", &solidInterval, 0.005f, 0.0f, 0.2f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
				SetSolidSampleInterval(solidInterval);
			ImGui::Text("Solid Slices: %zu / %zu", m_slicesSolid.size(), m_solidMaxSlices);
		}

		ImGui::SeparatorText("Alpha Layer (lit, tint + fade)");
		bool drawAlpha = m_drawAlphaLayer;
		if (ImGui::Checkbox("Draw Alpha Layer", &drawAlpha))
			SetDrawAlphaLayer(drawAlpha);
		if (m_drawAlphaLayer)
		{
			float ac[4] = { m_alphaTint.x, m_alphaTint.y, m_alphaTint.z, m_alphaTint.w };
			if (ImGui::ColorEdit4("Alpha Tint (RGB + A scale)", ac, ImGuiColorEditFlags_NoInputs))
				SetAlphaTint(Vector4(ac[0], ac[1], ac[2], ac[3]));
			float initialAlpha = m_alphaInitialAlpha;
			if (ImGui::SliderFloat("Alpha Initial Alpha", &initialAlpha, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
				SetAlphaInitialAlpha(initialAlpha);
			float decaySpeed = m_alphaDecaySpeed;
			if (ImGui::DragFloat("Alpha Decay Speed", &decaySpeed, 0.1f, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
				SetAlphaDecaySpeed(decaySpeed);
			int decayMode = static_cast<int>(m_alphaDecayMode);
			const char* decayModeNames[] = { "Simultaneous", "Sequential" };
			if (ImGui::Combo("Alpha Decay Mode", &decayMode, decayModeNames, 2))
				SetAlphaDecayMode(static_cast<AlphaDecayMode>(decayMode));
			int alphaCurve = static_cast<int>(m_alphaDecayCurve);
			const char* alphaCurveNames[] = { "Linear", "Exponential" };
			if (ImGui::Combo("Alpha Decay Curve", &alphaCurve, alphaCurveNames, 2))
				SetAlphaDecayCurve(static_cast<DecayCurve>(alphaCurve));
			int alphaMax = static_cast<int>(m_alphaMaxSlices);
			if (ImGui::SliderInt("Alpha Max Slices", &alphaMax, static_cast<int>(AFTERIMAGE_MIN_SLICES), static_cast<int>(AFTERIMAGE_MAX_SLICES_CAP), "%d", ImGuiSliderFlags_AlwaysClamp))
				SetAlphaMaxSlices(static_cast<size_t>(alphaMax));
			float alphaInterval = m_alphaSampleInterval;
			if (ImGui::DragFloat("Alpha Sample Interval (s)", &alphaInterval, 0.005f, 0.0f, 0.2f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
				SetAlphaSampleInterval(alphaInterval);
			float alphaEmissive = m_alphaEmissiveIntensity;
			if (ImGui::DragFloat("Alpha Emissive Intensity", &alphaEmissive, 0.1f, 0.0f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
				SetAlphaEmissiveIntensity(alphaEmissive);
			ImGui::TextUnformatted("(0 = lighting only, >0 = add emissive)");
			float alphaGrad = m_alphaTrailGradient;
			if (ImGui::SliderFloat("Alpha Trail Gradient", &alphaGrad, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
				SetAlphaTrailGradient(alphaGrad);
			ImGui::TextUnformatted("(1 = same alpha, <1 = front fades first)");
			ImGui::Text("Alpha Slices: %zu / %zu", m_slicesAlpha.size(), m_alphaMaxSlices);
		}

		ImGui::Unindent();
	}

	void AfterimageRenderer::Save(json& j) const
	{
		Object::Save(j);

		j["DrawSolidLayer"] = m_drawSolidLayer;
		j["SolidColor"] = { m_solidColor.x, m_solidColor.y, m_solidColor.z };
		j["SolidInitialAlpha"] = m_solidInitialAlpha;
		j["SolidDecaySpeed"] = m_solidDecaySpeed;
		j["SolidDecayMode"] = static_cast<int>(m_solidDecayMode);
		j["SolidDecayCurve"] = static_cast<int>(m_solidDecayCurve);
		j["SolidEmissiveIntensity"] = m_solidEmissiveIntensity;
		j["SolidTrailGradient"] = m_solidTrailGradient;
		j["SolidMaxSlices"] = m_solidMaxSlices;
		j["SolidSampleInterval"] = m_solidSampleInterval;
		j["DrawAlphaLayer"] = m_drawAlphaLayer;
		j["AlphaTint"] = { m_alphaTint.x, m_alphaTint.y, m_alphaTint.z, m_alphaTint.w };
		j["AlphaEmissiveIntensity"] = m_alphaEmissiveIntensity;
		j["AlphaInitialAlpha"] = m_alphaInitialAlpha;
		j["AlphaDecaySpeed"] = m_alphaDecaySpeed;
		j["AlphaDecayMode"] = static_cast<int>(m_alphaDecayMode);
		j["AlphaDecayCurve"] = static_cast<int>(m_alphaDecayCurve);
		j["AlphaTrailGradient"] = m_alphaTrailGradient;
		j["AlphaMaxSlices"] = m_alphaMaxSlices;
		j["AlphaSampleInterval"] = m_alphaSampleInterval;

		// 소스는 런타임 핸들(인스턴스/씬 내부 유효)이므로 저장하지 않음.
		// 로드 후 Initialize()에서 같은 GameObject의 SkeletalMeshRenderer로 자동 재결정됨.
	}

	void AfterimageRenderer::Load(const json& j)
	{
		Object::Load(j);

		if (j.contains("DrawSolidLayer"))
			m_drawSolidLayer = j["DrawSolidLayer"].get<bool>();
		if (j.contains("SolidColor") && j["SolidColor"].is_array() && j["SolidColor"].size() >= 3)
		{
			m_solidColor.x = j["SolidColor"][0].get<float>();
			m_solidColor.y = j["SolidColor"][1].get<float>();
			m_solidColor.z = j["SolidColor"][2].get<float>();
			m_solidColor.w = 1.0f;
		}
		JsonGet(j, "SolidInitialAlpha", m_solidInitialAlpha, 1.0f);
		JsonGet(j, "SolidDecaySpeed", m_solidDecaySpeed, 1.5f);
		if (j.contains("SolidDecayMode"))
		{
			int dm = static_cast<int>(m_solidDecayMode);
			JsonGet(j, "SolidDecayMode", dm, 0);
			m_solidDecayMode = (dm == 1) ? AlphaDecayMode::Sequential : AlphaDecayMode::Simultaneous;
		}
		if (j.contains("SolidDecayCurve"))
		{
			int dc = static_cast<int>(m_solidDecayCurve);
			JsonGet(j, "SolidDecayCurve", dc, 1);
			m_solidDecayCurve = (dc == 0) ? DecayCurve::Linear : DecayCurve::Exponential;
		}
		JsonGet(j, "SolidEmissiveIntensity", m_solidEmissiveIntensity, 1.0f);
		JsonGet(j, "SolidTrailGradient", m_solidTrailGradient, 0.92f);
		JsonGet(j, "SolidMaxSlices", m_solidMaxSlices, static_cast<size_t>(AFTERIMAGE_DEFAULT_MAX_SLICES));
		JsonGet(j, "SolidSampleInterval", m_solidSampleInterval, 0.0f);
		if (j.contains("DrawAlphaLayer"))
			m_drawAlphaLayer = j["DrawAlphaLayer"].get<bool>();
		if (j.contains("AlphaTint") && j["AlphaTint"].is_array() && j["AlphaTint"].size() >= 4)
		{
			m_alphaTint.x = j["AlphaTint"][0].get<float>();
			m_alphaTint.y = j["AlphaTint"][1].get<float>();
			m_alphaTint.z = j["AlphaTint"][2].get<float>();
			m_alphaTint.w = j["AlphaTint"][3].get<float>();
		}
		JsonGet(j, "AlphaEmissiveIntensity", m_alphaEmissiveIntensity, 0.0f);
		JsonGet(j, "AlphaTrailGradient", m_alphaTrailGradient, 0.92f);
		JsonGet(j, "AlphaInitialAlpha", m_alphaInitialAlpha, 0.7f);
		JsonGet(j, "AlphaDecaySpeed", m_alphaDecaySpeed, 1.5f);
		int decayMode = static_cast<int>(m_alphaDecayMode);
		JsonGet(j, "AlphaDecayMode", decayMode, 0);
		m_alphaDecayMode = (decayMode == 1) ? AlphaDecayMode::Sequential : AlphaDecayMode::Simultaneous;
		if (j.contains("AlphaDecayCurve"))
		{
			int dc = static_cast<int>(m_alphaDecayCurve);
			JsonGet(j, "AlphaDecayCurve", dc, 1);
			m_alphaDecayCurve = (dc == 0) ? DecayCurve::Linear : DecayCurve::Exponential;
		}
		JsonGet(j, "AlphaMaxSlices", m_alphaMaxSlices, static_cast<size_t>(AFTERIMAGE_DEFAULT_MAX_SLICES));
		JsonGet(j, "AlphaSampleInterval", m_alphaSampleInterval, 0.0f);

		m_solidInitialAlpha = std::clamp(m_solidInitialAlpha, 0.0f, 1.0f);
		m_solidDecaySpeed = std::max(0.0f, m_solidDecaySpeed);
		m_solidEmissiveIntensity = std::max(0.0f, m_solidEmissiveIntensity);
		m_solidTrailGradient = std::clamp(m_solidTrailGradient, 0.0f, 1.0f);
		m_solidMaxSlices = std::clamp(m_solidMaxSlices, AFTERIMAGE_MIN_SLICES, AFTERIMAGE_MAX_SLICES_CAP);
		m_solidSampleInterval = std::max(0.0f, m_solidSampleInterval);
		m_alphaEmissiveIntensity = std::max(0.0f, m_alphaEmissiveIntensity);
		m_alphaTrailGradient = std::clamp(m_alphaTrailGradient, 0.0f, 1.0f);
		m_alphaInitialAlpha = std::clamp(m_alphaInitialAlpha, 0.0f, 1.0f);
		m_alphaDecaySpeed = std::max(0.0f, m_alphaDecaySpeed);
		m_alphaMaxSlices = std::clamp(m_alphaMaxSlices, AFTERIMAGE_MIN_SLICES, AFTERIMAGE_MAX_SLICES_CAP);
		m_alphaSampleInterval = std::max(0.0f, m_alphaSampleInterval);
		m_alphaTint.w = std::clamp(m_alphaTint.w, 0.0f, 1.0f);

		// SourceHandleIndex/Generation은 저장하지 않음(런타임 전용). 소스는 Initialize()에서 같은 GameObject의 SkeletalMeshRenderer로 재결정됨.
	}
}
