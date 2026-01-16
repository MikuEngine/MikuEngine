#include "EnginePCH.h"
#include "UIImage.h"

#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Resource/Texture.h"
#include "Core/Graphics/Resource/ConstantBuffer.h"
#include "Core/Graphics/Resource/VertexShader.h"
#include "Core/Graphics/Resource/PixelShader.h"
#include "Core/Graphics/Resource/InputLayout.h"
#include "Core/Graphics/Resource/VertexBuffer.h"
#include "Core/Graphics/Resource/IndexBuffer.h"
#include "Core/Graphics/Resource/SamplerState.h"
#include "Core/Graphics/Resource/BlendState.h"
#include "Core/Graphics/Resource/DepthStencilState.h"
#include "Core/Graphics/Data/ShaderSlotTypes.h"
#include "Core/Graphics/Data/ConstantBufferTypes.h"
#include "Core/Graphics/Device/GraphicsDevice.h"

#include "Framework/Object/Component/RectTransform.h"
#include "Framework/Asset/MaterialData.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"

namespace engine
{
	void UIImage::Initialize()
	{
		//UIElement::Initialize();
		if (m_texture == nullptr)
		{
			m_texture = ResourceManager::Get().GetDefaultTexture(DefaultTextureType::White);
			m_textureFilePath = "None";
		}

		if (!m_isLoaded)
		{
			m_vsFilePath = "Resource/Shader/Vertex/FullscreenQuad_VS.hlsl";
			m_opaquePSFilePath = "Resource/Shader/Pixel/Sprite_Unlit_PS.hlsl";
			m_cutoutPSFilePath = "Resource/Shader/Pixel/Sprite_Unlit_Cutout_PS.hlsl";
			m_transparentPSFilePath = "Resource/Shader/Pixel/Blit_PS.hlsl";

			m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);


			m_transparentPS = ResourceManager::Get().GetOrCreatePixelShader(m_transparentPSFilePath);
		}

		m_vertexBuffer = ResourceManager::Get().GetGeometryVertexBuffer("DefaultQuad");
		m_indexBuffer = ResourceManager::Get().GetGeometryIndexBuffer("DefaultQuad");

		m_inputLayout = m_vs->GetOrCreateInputLayout<PositionTexCoordVertex>();
		m_samplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);

		m_blend = ResourceManager::Get().GetDefaultBlendState(DefaultBlendType::AlphaBlend);
		m_depthNone = ResourceManager::Get().GetDefaultDepthStencilState(DefaultDepthStencilType::None);

		SystemManager::Get().GetRenderSystem().Register(this);
	}

	void UIImage::DrawUI() const
	{
		if (!IsActive() || !GetGameObject())
			return;

		// RectTransform 필요
		auto* rt = dynamic_cast<RectTransform*>(GetTransform());
		if (!rt)
			return;

		auto& gd = GraphicsDevice::Get();
		auto dc = gd.GetDeviceContext();
		const D3D11_VIEWPORT vp = gd.GetViewport();
		const float W = vp.Width;
		const float H = vp.Height;

		UIRect root;

		const UIRect& rect = rt->GetWorldRectResolved(root);

		float cx = rect.x + rect.w * 0.5f;
		float cy = rect.y + rect.h * 0.5f;

		CbObject cbObject{};
		
		Matrix matWorld = Matrix::CreateScale(rect.w, rect.h, 1.0f) * Matrix::CreateTranslation(rect.x, rect.y, 0.0f);

		









		if (m_drawOnlyWhenRectValid)
		{
			if (rect.w <= 0.0f || rect.h <= 0.0f)
				return;
		}

		// 텍스처 없으면 스킵
		if (!m_texture)
			return;


		D3D11_VIEWPORT uiVp{};
		uiVp.TopLeftX = rect.x;
		uiVp.TopLeftY = rect.y;
		uiVp.Width = rect.w;
		uiVp.Height = rect.h;
		uiVp.MinDepth = 0.0f;
		uiVp.MaxDepth = 1.0f;

		// IA
		{
			dc->IASetInputLayout(m_inputLayout->GetRawInputLayout());

			UINT stride = m_vertexBuffer->GetBufferStride();
			UINT offset = 0;
			ID3D11Buffer* vb = m_vertexBuffer->GetRawBuffer();
			dc->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

			dc->IASetIndexBuffer(m_indexBuffer->GetRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
			dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}

		// VS/PS
		dc->VSSetShader(m_vs->GetRawShader(), nullptr, 0);
		dc->PSSetShader(m_transparentPS->GetRawShader(), nullptr, 0);

		// States (UI는 깊이 끄고, 알파블렌드)
		{
			float blendFactor[4] = { 0,0,0,0 };
			if (m_useAlphaBlend && m_blend)
				dc->OMSetBlendState(m_blend->GetBlendState().Get(), blendFactor, 0xffffffff);
			else
				dc->OMSetBlendState(nullptr, blendFactor, 0xffffffff);

			if (m_depthNone)
				dc->OMSetDepthStencilState(m_depthNone->GetDepthStencilState().Get(), 0);
			else
				dc->OMSetDepthStencilState(nullptr, 0);
		}

		// Texture/Sampler : Blit_PS가 사용하는 슬롯에 맞춰줌
		{
			ID3D11ShaderResourceView* srv = m_texture->GetRawSRV();
			dc->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, &srv);

			auto samp = m_samplerState ? m_samplerState->GetSamplerState().GetAddressOf() : nullptr;
			if (samp)
				dc->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, samp);
		}

		// Draw
		dc->DrawIndexed(m_indexBuffer->GetIndexCount(), 0, 0);

		// SRV unbind (D3D11 경고 방지)
		{
			ID3D11ShaderResourceView* nullSRV = nullptr;
			dc->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, &nullSRV);
		}
	}

	void UIImage::SetTexture(const std::string& textureFilePath)
	{
		if (textureFilePath.empty() || textureFilePath == "None")
		{
			return;
		}

		m_textureFilePath = textureFilePath;

		m_texture = ResourceManager::Get().GetOrCreateTexture(textureFilePath);
		m_width = m_texture->GetWidth();
		m_height = m_texture->GetHeight();
	}

	const std::string& UIImage::GetTexturePath() const
	{
		return m_textureFilePath;
	}

	void UIImage::SetAlphaBlend(bool enable)
	{
		m_useAlphaBlend = enable;
	}

	bool UIImage::IsAlphaBlend() const
	{
		return m_useAlphaBlend;
	}

	bool UIImage::HasRenderType(RenderType type) const
	{
		return type == RenderType::Screen;
	}

	void UIImage::Draw(RenderType type) const
	{
		UIElement::Draw(type);

		//if (type != RenderType::Screen)
		//	return;

		//DrawUI();
	}

	DirectX::BoundingBox UIImage::GetBounds() const
	{
		DirectX::BoundingBox b{};
		b.Center = { 0,0,0 };
		b.Extents = { 0,0,0 };
		return b;
	}

	void UIImage::OnGui()
	{
		ImGui::Text("Texture: %s", std::filesystem::path(m_textureFilePath).filename().string().c_str());
		std::string selectedTex;

		static std::vector<std::string> texExtensions{ ".png", ".jpg", ".tga" };
		static std::string hlslExtension{ ".hlsl" };

		if (DrawFileSelector("Select Texture", "Resource/Texture", texExtensions, selectedTex))
		{
			SetTexture(selectedTex);
		}
		ImGui::Spacing();
		// 2. Settings (RenderType, Shadow, etc.)
		// RenderType (Enum to Checkbox or Combo)
		static const char* renderTypes[] = { "Opaque", "Cutout", "Transparent" };
		int currentType = static_cast<int>(m_renderType);
		if (ImGui::Combo("Render Type", &currentType, renderTypes, IM_ARRAYSIZE(renderTypes)))
		{
			m_renderType = static_cast<MaterialRenderType>(currentType);

			ReplaceRenderSystem();
		}

		ImGui::Checkbox("Alpha Blend", &m_useAlphaBlend);
		ImGui::Checkbox("Draw Only When Rect Valid", &m_drawOnlyWhenRectValid);
	}

	void UIImage::Save(json& j) const
	{
		UIElement::Save(j);

		j["TexturePath"] = m_textureFilePath;
		j["OpaquePSFilePath"] = m_opaquePSFilePath;
		j["CutoutPSFilePath"] = m_cutoutPSFilePath;
		j["TransparentPSFilePath"] = m_transparentPSFilePath;
		j["RenderType"] = m_renderType;
		j["AlphaBlend"] = m_useAlphaBlend;
		j["SkipInvalidRect"] = m_drawOnlyWhenRectValid;
	}

	void UIImage::Load(const json& j)
	{
		UIElement::Load(j);

		JsonGet(j, "TexturePath", m_textureFilePath);
		JsonGet(j, "OpaquePSFilePath", m_opaquePSFilePath);
		JsonGet(j, "CutoutPSFilePath", m_cutoutPSFilePath);
		JsonGet(j, "TransparentPSFilePath", m_transparentPSFilePath);
		JsonGet(j, "RenderType", m_renderType);
		JsonGet(j, "AlphaBlend", m_useAlphaBlend);
		JsonGet(j, "SkipInvalidRect", m_drawOnlyWhenRectValid);
		Refresh();
	}

	std::string UIImage::GetType() const
	{
		return "UIImage";
	}

	void UIImage::ReplaceRenderSystem()
	{
		SystemManager::Get().GetRenderSystem().Unregister(this);
		SystemManager::Get().GetRenderSystem().Register(this);
	}
	void UIImage::Refresh()
	{
		SetTexture(m_textureFilePath);

		m_opaquePS = ResourceManager::Get().GetOrCreatePixelShader(m_opaquePSFilePath);
		m_cutoutPS = ResourceManager::Get().GetOrCreatePixelShader(m_cutoutPSFilePath);
		m_transparentPS = ResourceManager::Get().GetOrCreatePixelShader(m_transparentPSFilePath);
	}
}