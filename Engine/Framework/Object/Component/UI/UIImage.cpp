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

#include "Framework/Asset/MaterialData.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/RectTransform.h"
#include "Framework/Object/Component/UI/UIButton.h"

#include "Framework/Object/Component/Canvas.h"

namespace engine
{
	void UIImage::Initialize()
	{
		UIElement::Initialize();

		if (!m_texture) m_texture = ResourceManager::Get().GetDefaultTexture(DefaultTextureType::White);
		if (!m_noiseTex) m_noiseTex = ResourceManager::Get().GetDefaultTexture(DefaultTextureType::White);
		if (!m_rampTex)  m_rampTex = ResourceManager::Get().GetDefaultTexture(DefaultTextureType::White);

		m_vsFilePath = "Resource/Shader/Vertex/UIQuad_VS.hlsl";
		m_psFilePath = "Resource/Shader/Pixel/UIQuad_PS.hlsl";
		m_outlinePSFilePath = "Resource/Shader/Pixel/UIQuadOutline_PS.hlsl";

		m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);
		m_ps = ResourceManager::Get().GetOrCreatePixelShader(m_psFilePath);
		m_outlinePS = ResourceManager::Get().GetOrCreatePixelShader(m_outlinePSFilePath);

		m_vertexBuffer = ResourceManager::Get().GetGeometryVertexBuffer("DefaultQuad");
		m_indexBuffer = ResourceManager::Get().GetGeometryIndexBuffer("DefaultQuad");

		m_inputLayout = m_vs->GetOrCreateInputLayout<PositionTexCoordVertex>();
		m_sampler = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);

		m_blend = ResourceManager::Get().GetDefaultBlendState(DefaultBlendType::AlphaBlend);
		m_depthNone = ResourceManager::Get().GetDefaultDepthStencilState(DefaultDepthStencilType::None);

		m_uiCB = ResourceManager::Get().GetOrCreateConstantBuffer("UIElement", sizeof(CbUIElement));

		SystemManager::Get().GetRenderSystem().Register(this);
    }

	void UIImage::DrawUI() const
	{
		if (!IsActive() || !GetGameObject())
			return;

		auto* rt = GetRectTransform();
		if (!rt) return;

		auto& gd = GraphicsDevice::Get();
		auto dc = gd.GetDeviceContext();
		const D3D11_VIEWPORT vp = gd.GetViewport();

		Canvas* c = GetCanvasInParent();
		if (!c) return;

		// ===== ref 기준 rect 계산 =====
		const Vector2 ref = c->GetReferenceResolution();
		UIRect rootRect{ 0.0f, 0.0f, ref.x, ref.y };
		const UIRect rect = rt->GetWorldRectResolved(rootRect);
		if (rect.w <= 0.0f || rect.h <= 0.0f)
			return;

		const Vector2 scale = c->GetUIScale();
		const Vector2 offset = c->GetUIOffset();

		// ===== ref → pixel =====
		const float pxX = offset.x + rect.x * scale.x;
		const float pxY = offset.y + rect.y * scale.y;
		const float pxW = rect.w * scale.x;
		const float pxH = rect.h * scale.y;

		const float cx = pxX + pxW * 0.5f;
		const float cy = pxY + pxH * 0.5f;

		const float tx = (cx / vp.Width) * 2.0f - 1.0f;
		const float ty = 1.0f - (cy / vp.Height) * 2.0f;

		const float sx = (pxW / vp.Width) * 2.0f;
		const float sy = (pxH / vp.Height) * 2.0f;

		if (!m_texture || !m_vertexBuffer || !m_indexBuffer)
			return;

		// ===== clipRect (ref → pixel) : 단 한 번 =====
		Vector4 clipPx = m_clipRect;

		if (m_maskMode == MaskMode::Rect)
		{
			clipPx = Vector4(
				offset.x + m_clipRect.x * scale.x,
				offset.y + m_clipRect.y * scale.y,
				offset.x + m_clipRect.z * scale.x,
				offset.y + m_clipRect.w * scale.y
			);
		}

		if (clipPx.z <= clipPx.x || clipPx.w <= clipPx.y)
		{
			clipPx = Vector4(0, 0, vp.Width, vp.Height);
		}

		// ===== IA =====
		dc->IASetInputLayout(m_inputLayout->GetRawInputLayout());
		{
			UINT stride = m_vertexBuffer->GetBufferStride();
			UINT offsetVB = 0;
			ID3D11Buffer* vb = m_vertexBuffer->GetRawBuffer();
			dc->IASetVertexBuffers(0, 1, &vb, &stride, &offsetVB);
			dc->IASetIndexBuffer(m_indexBuffer->GetRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
			dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}

		// ===== State =====
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

		// ===== Texture / Sampler =====
		{
			ID3D11ShaderResourceView* srv = m_texture->GetRawSRV();
			dc->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, &srv);

			ID3D11ShaderResourceView* srvN = (m_noiseTex ? m_noiseTex->GetRawSRV() : nullptr);
			dc->PSSetShaderResources(static_cast<UINT>(TextureSlot::UINoise), 1, &srvN);

			ID3D11ShaderResourceView* srvR = (m_rampTex ? m_rampTex->GetRawSRV() : nullptr);
			dc->PSSetShaderResources(static_cast<UINT>(TextureSlot::UIRamp), 1, &srvR);

			auto samp = m_sampler ? m_sampler->GetSamplerState().GetAddressOf() : nullptr;
			if (samp)
				dc->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, samp);
		}

		// ===== Outline Pass =====
		if (m_outlineEnabled && m_outlineThickness > 0.0f)
		{
			const float t = m_outlineThickness;

			const float expandU = (t / pxW); // px 기준 -> uv 비율
			const float expandV = (t / pxH);

			const float o_pxW = pxW + m_outlineThickness * 2.0f;
			const float o_pxH = pxH + m_outlineThickness * 2.0f;

			const float o_sx = (o_pxW / vp.Width) * 2.0f;
			const float o_sy = (o_pxH / vp.Height) * 2.0f;

			CbUIElement cbUI{};
			cbUI.clip = DirectX::XMMatrixTranspose(
				DirectX::XMMatrixScaling(o_sx, o_sy, 1.0f) *
				DirectX::XMMatrixTranslation(tx, ty, 0.0f)
			);

			cbUI.color = Vector4(1, 1, 1, 1);
			cbUI.uv = Vector4(
				m_uv.x - expandU * m_uv.z,                 // uOffset'
				m_uv.y - expandV * m_uv.w,                 // vOffset'
				m_uv.z * (1.0f + expandU * 2.0f),          // uScale'
				m_uv.w * (1.0f + expandV * 2.0f)           // vScale'
			);

			cbUI.clipRect = clipPx;
			cbUI.maskMode = static_cast<uint32_t>(m_maskMode);
			cbUI.mask0 = m_mask0;
			cbUI.mask1 = m_mask1;
			cbUI.outlineEnabled = 1.0f;
			cbUI.outlineThickness = m_outlineThickness;
			cbUI.outlineColor = m_outlineColor;

			dc->UpdateSubresource(m_uiCB->GetRawBuffer(), 0, nullptr, &cbUI, 0, 0);
			dc->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::UIElement), 1, m_uiCB->GetBuffer().GetAddressOf());
			dc->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::UIElement), 1, m_uiCB->GetBuffer().GetAddressOf());

			dc->VSSetShader(m_vs->GetRawShader(), nullptr, 0);
			dc->PSSetShader(m_outlinePS->GetRawShader(), nullptr, 0);

			dc->DrawIndexed(m_indexBuffer->GetIndexCount(), 0, 0);
		}

		// ===== Texture Pass =====
		{
			CbUIElement cbUI{};
			cbUI.clip = DirectX::XMMatrixTranspose(
				DirectX::XMMatrixScaling(sx, sy, 1.0f) *
				DirectX::XMMatrixTranslation(tx, ty, 0.0f)
			);

			cbUI.color = m_color;
			cbUI.uv = m_uv;
			cbUI.clipRect = clipPx;
			cbUI.maskMode = static_cast<uint32_t>(m_maskMode);
			cbUI.mask0 = m_mask0;
			cbUI.mask1 = m_mask1;
			cbUI.outlineEnabled = m_outlineEnabled;
			cbUI.outlineThickness = m_outlineThickness;
			cbUI.outlineColor = m_outlineColor;

			cbUI.effectMode = m_effectMode;
			cbUI.effectFlags = m_effectFlags;
			cbUI.time = engine::Time::UnscaledTime();

			cbUI.effect0 = m_effect0;
			cbUI.effect1 = m_effect1;
			cbUI.effect2 = m_effect2;

			dc->UpdateSubresource(m_uiCB->GetRawBuffer(), 0, nullptr, &cbUI, 0, 0);
			dc->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::UIElement), 1, m_uiCB->GetBuffer().GetAddressOf());
			dc->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::UIElement), 1, m_uiCB->GetBuffer().GetAddressOf());

			dc->VSSetShader(m_vs->GetRawShader(), nullptr, 0);
			dc->PSSetShader(m_ps->GetRawShader(), nullptr, 0);

			dc->DrawIndexed(m_indexBuffer->GetIndexCount(), 0, 0);
		}

		// ===== SRV unbind =====
		{
			ID3D11ShaderResourceView* nullSRV = nullptr;
			dc->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, &nullSRV);
			dc->PSSetShaderResources(static_cast<UINT>(TextureSlot::UINoise), 1, &nullSRV);
			dc->PSSetShaderResources(static_cast<UINT>(TextureSlot::UIRamp), 1, &nullSRV);
		}
	}

	void UIImage::SetTexture(const std::string& textureFilePath)
	{
		if (textureFilePath.empty()) return;

		m_textureFilePath = textureFilePath;

		if (textureFilePath == "None")
		{
			m_texture = ResourceManager::Get().GetDefaultTexture(DefaultTextureType::White);
			return;
		}

		std::shared_ptr<engine::Texture> tex = ResourceManager::Get().GetOrCreateTexture(textureFilePath);
		if (!tex)
		{
			LOG_PRINT("FATAL: Texture not found: %s", textureFilePath.c_str());
			return;
		}

		m_texture = tex;
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

	void UIImage::SetColor(const Vector4& color)
	{
		m_color = color;
	}

	const Vector4& UIImage::GetColor() const
	{
		return m_color;
	}

	void UIImage::SetMaskMode(MaskMode mode)
	{
		if (m_maskMode == mode) return;

		m_maskMode = mode;
		m_dirty = true;
	}

	void UIImage::SetOutline(bool enable, float thickness, const Vector4& color)
	{
		m_outlineEnabled = enable;
		m_outlineThickness = thickness;
		m_outlineColor = color;
	}

	bool UIImage::HasRenderType(RenderType type) const
	{
		return type == RenderType::Screen;
	}

	void UIImage::Draw(RenderType type) const
	{
		UIElement::Draw(type);
	}

	DirectX::BoundingBox UIImage::GetBounds() const
	{
		return UIElement::GetBounds();
	}

	void UIImage::OnGui()
	{
		UIElement::OnGui();

		if (GameObject* go = GetGameObject())
		{
			if (auto* btn = go->GetComponent<UIButton>())
			{
				ImGui::Separator();
				ImGui::TextDisabled("This Image is controlled by UIButton (SpriteSwap/ Tint).");
				return;
			}
		}

		ImGui::Text("Texture: %s", std::filesystem::path(m_textureFilePath).filename().string().c_str());
		std::string selectedTex;

		static std::vector<std::string> texExtensions{ ".png", ".jpg", ".tga"};

		if (DrawFileSelector("Select Texture", "Resource/Texture", texExtensions, selectedTex))
		{
			SetTexture(selectedTex);
		}
		ImGui::Spacing();

		ImGui::ColorEdit4("Color", &m_color.x);
		ImGui::Checkbox("Alpha Blend", &m_useAlphaBlend);
	}

	void UIImage::Save(json& j) const
	{
		UIElement::Save(j);

		j["TexturePath"] = m_textureFilePath;
		j["VSFilePath"] = m_vsFilePath;
		j["PSFilePath"] = m_psFilePath;
		j["OutlinePSFilePath"] = m_outlinePSFilePath;

		j["Color"] = m_color; 
		j["AlphaBlend"] = m_useAlphaBlend;

		j["ClipRect"] = m_clipRect;
		j["MaskMode"] = (int)m_maskMode;
		j["Mask0"] = m_mask0;
		j["Mask1"] = m_mask1;
	}

	void UIImage::Load(const json& j)
	{
		UIElement::Load(j);

		JsonGet(j, "TexturePath", m_textureFilePath);
		JsonGet(j, "VSFilePath", m_vsFilePath);
		JsonGet(j, "PSFilePath", m_psFilePath);
		JsonGet(j, "OutlinePSFilePath", m_outlinePSFilePath);

		JsonGet(j, "Color", m_color);
		JsonGet(j, "AlphaBlend", m_useAlphaBlend);
		
		JsonGet(j, "ClipRect", m_clipRect);

		int mm = (int)MaskMode::None;
		JsonGet(j, "MaskMode", mm);
		m_maskMode = (MaskMode)mm;

		JsonGet(j, "Mask0", m_mask0);
		JsonGet(j, "Mask1", m_mask1);

		Refresh();
	}

	void UIImage::Refresh()
	{
		SetTexture(m_textureFilePath);
	}
}