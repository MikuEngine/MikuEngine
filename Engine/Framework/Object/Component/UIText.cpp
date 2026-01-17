#include "EnginePCH.h"
#include "UIText.h"

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

#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/Object/Component/RectTransform.h"

namespace engine
{
	void UIText::Initialize()
	{
		UIElement::Initialize();

		m_vs = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/UIQuad_VS.hlsl");
		m_ps = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/UIQuad_PS.hlsl");

		m_vertexBuffer = ResourceManager::Get().GetGeometryVertexBuffer("DefaultQuad");
		m_indexBuffer = ResourceManager::Get().GetGeometryIndexBuffer("DefaultQuad");

		m_inputLayout = m_vs->GetOrCreateInputLayout<PositionTexCoordVertex>();
		m_sampler = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);

		m_blend = ResourceManager::Get().GetDefaultBlendState(DefaultBlendType::AlphaBlend);
		m_depthNone = ResourceManager::Get().GetDefaultDepthStencilState(DefaultDepthStencilType::None);

		m_uiCB = ResourceManager::Get().GetOrCreateConstantBuffer("UIElement", sizeof(CbUIElement));

		RefreshFont();

		SystemManager::Get().GetRenderSystem().Register(this);
	}

	void UIText::DrawUI() const
	{
		auto* rt = GetRectTransform();
		if (!rt || !m_font || !m_font->atlas) return;

		auto& gd = GraphicsDevice::Get();
		auto dc = gd.GetDeviceContext();
		const D3D11_VIEWPORT vp = gd.GetViewport();

		UIRect rootRect{ 0.0f, 0.0f, vp.Width, vp.Height };
		const UIRect rect = rt->GetWorldRectResolved(rootRect);

		if (m_drawOnlyWhenRectValid)
		{
			if (rect.w <= 0.0f || rect.h <= 0.0f) return;
		}

		// IA
		dc->IASetInputLayout(m_inputLayout->GetRawInputLayout());
		{
			UINT stride = m_vertexBuffer->GetBufferStride();
			UINT offset = 0;
			ID3D11Buffer* vb = m_vertexBuffer->GetRawBuffer();
			dc->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
			dc->IASetIndexBuffer(m_indexBuffer->GetRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
			dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}
		dc->VSSetShader(m_vs->GetRawShader(), nullptr, 0);
		dc->PSSetShader(m_ps->GetRawShader(), nullptr, 0);

		// State
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

		// Texture/Sampler
		{
			ID3D11ShaderResourceView* srv = m_font->atlas->GetRawSRV();
			dc->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, &srv);

			auto samp = m_sampler ? m_sampler->GetSamplerState().GetAddressOf() : nullptr;
			if (samp)
				dc->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, samp);
		}

		dc->RSSetState(nullptr);
		dc->RSSetViewports(1, &vp);

		// SRV unbind
		{
			ID3D11ShaderResourceView* nullSRV = nullptr;
			dc->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, &nullSRV);
		}
	}

	void UIText::SetText(const std::string& text)
	{
		m_text = text;
	}

	const std::string& UIText::GetText() const
	{
		return m_text;
	}

	void UIText::SetColor(const Vector4& color)
	{
		m_color = color;
	}

	const Vector4& UIText::GetColor() const
	{
		return m_color;
	}

	void UIText::SetFont(const std::string& fontMetaPath)
	{
		m_fontMetaPath = fontMetaPath;
		RefreshFont();
	}

	const std::string& UIText::GetFontPath() const
	{
		return m_fontMetaPath;
	}

	void UIText::SetAlphaBlend(bool enable)
	{
		m_useAlphaBlend = enable;
	}

	bool UIText::IsAlphaBlend() const
	{
		return m_useAlphaBlend;
	}

	bool UIText::HasRenderType(RenderType type) const
	{
		return false;
	}

	void UIText::Draw(RenderType type) const
	{
	}

	DirectX::BoundingBox UIText::GetBounds() const
	{
		return DirectX::BoundingBox();
	}

	void UIText::OnGui()
	{
		//ImGui::InputTextMultiline("Text");
		ImGui::DragFloat("Font Scale", &m_fontScale, 0.01f, 0.1f, 10.0f);
		ImGui::DragFloat("Letter Spacing", &m_letterSpacing, 0.1f, -50.0f, 200.0f);
		ImGui::DragFloat("Line Spacing", &m_lineSpacing, 0.1f, -50.0f, 200.0f);
		ImGui::ColorEdit4("Color", &m_color.x);
		ImGui::Checkbox("Alpha Blend", &m_useAlphaBlend);
		ImGui::Checkbox("Draw Only When Rect Valid", &m_drawOnlyWhenRectValid);
	}

	void UIText::Save(json& j) const
	{
		UIElement::Save(j);
		j["Text"] = m_text;
		j["Color"] = m_color;
		j["FontMetaPath"] = m_fontMetaPath;
		j["FontScale"] = m_fontScale;
		j["LetterSpacing"] = m_letterSpacing;
		j["LineSpacing"] = m_lineSpacing;
		j["AlphaBlend"] = m_useAlphaBlend;
		j["SkipInvalidRect"] = m_drawOnlyWhenRectValid;
		j["WordWrap"] = m_wordWrap;
		j["AlignH"] = (int)m_alignH;
		j["AlignV"] = (int)m_alignV;
	}

	void UIText::Load(const json& j)
	{
		UIElement::Load(j);
		JsonGet(j, "Text", m_text);
		JsonGet(j, "Color", m_color);
		JsonGet(j, "FontMetaPath", m_fontMetaPath);
		JsonGet(j, "FontScale", m_fontScale);
		JsonGet(j, "LetterSpacing", m_letterSpacing);
		JsonGet(j, "LineSpacing", m_lineSpacing);
		JsonGet(j, "AlphaBlend", m_useAlphaBlend);
		JsonGet(j, "SkipInvalidRect", m_drawOnlyWhenRectValid);
		JsonGet(j, "WordWrap", m_wordWrap);

		//JsonGet(j, "Align", )

	    RefreshFont();
	}

	std::string UIText::GetType() const
	{
		return "UIText";
	}

	void UIText::RefreshFont()
	{

	}

	//void UIText::DrawGlyphQuad(const UIRect& glyphRectPx, const Vector4& uv01, const Vector4& color) const
	//{
	//}
}