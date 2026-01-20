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

#include "Framework/Object/Component/RectTransform.h"

namespace engine
{
	void UIImage::Initialize()
	{
		UIElement::Initialize();

		if (m_texture == nullptr)
		{
			m_texture = ResourceManager::Get().GetDefaultTexture(DefaultTextureType::White);
			m_textureFilePath = "None";
		}

		m_vsFilePath = "Resource/Shader/Vertex/UIQuad_VS.hlsl";
		m_psFilePath = "Resource/Shader/Pixel/UIQuad_PS.hlsl";

		m_vs = ResourceManager::Get().GetOrCreateVertexShader(m_vsFilePath);
		m_ps = ResourceManager::Get().GetOrCreatePixelShader(m_psFilePath);

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

		UIRect rootRect{ 0.0f, 0.0f, vp.Width, vp.Height };

		const UIRect rect = rt->GetWorldRectResolved(rootRect);

		if (m_drawOnlyWhenRectValid)
		{
			if (rect.w <= 0.0f || rect.h <= 0.0f)
				return;
		}

		if (!m_texture || !m_vertexBuffer || !m_indexBuffer) return;


		dc->IASetInputLayout(m_inputLayout->GetRawInputLayout());

		// IA
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
			float blendFactor[4] = { 0, 0, 0, 0 };

			if (m_useAlphaBlend && m_blend)
				dc->OMSetBlendState(m_blend->GetBlendState().Get(), blendFactor, 0xffffffff);
			else
				dc->OMSetBlendState(nullptr, blendFactor, 0xffffffff);

			if (m_depthNone)
				dc->OMSetDepthStencilState(m_depthNone->GetDepthStencilState().Get(), 0);
			else
				dc->OMSetDepthStencilState(nullptr, 0);
		}

		// ConstantBuffer
		{
			const float cx = rect.x + rect.w * 0.5f;
			const float cy = rect.y + rect.h * 0.5f;

			const float tx = (cx / vp.Width) * 2.0f - 1.0f;
			const float ty = 1.0f - (cy / vp.Height) * 2.0f;

			// 픽셀 크기 -> NDC 크기
			const float sx = (rect.w / vp.Width) * 2.0f;
			const float sy = (rect.h / vp.Height) * 2.0f;

			CbUIElement cbUI{};
			cbUI.clip = DirectX::XMMatrixTranspose(
				DirectX::XMMatrixScaling(sx, sy, 1.0f) *
				DirectX::XMMatrixTranslation(tx, ty, 0.0f)
			);

			cbUI.color = m_color;
			cbUI.uv = Vector4(0, 0, 1, 1);
			cbUI.clipRect = Vector4(0, 0, vp.Width, vp.Height);
			cbUI.maskMode = 1;	// rect

			dc->UpdateSubresource(m_uiCB->GetRawBuffer(), 0, nullptr, &cbUI, 0, 0);
			dc->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::UIElement), 1, m_uiCB->GetBuffer().GetAddressOf());
			dc->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::UIElement), 1, m_uiCB->GetBuffer().GetAddressOf());
		}

		// Texture/Sampler
		{
			ID3D11ShaderResourceView* srv = m_texture->GetRawSRV();
			dc->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, &srv);

			auto samp = m_sampler ? m_sampler->GetSamplerState().GetAddressOf() : nullptr;
			if (samp)
				dc->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, samp);
		}

		dc->RSSetState(nullptr);
		dc->RSSetViewports(1, &vp);

		dc->DrawIndexed(m_indexBuffer->GetIndexCount(), 0, 0);

		// SRV unbind
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

		ImGui::Text("Texture: %s", std::filesystem::path(m_textureFilePath).filename().string().c_str());
		std::string selectedTex;

		static std::vector<std::string> texExtensions{ ".png", ".jpg", ".tga" };
		static std::string hlslExtension{ ".hlsl" };

		if (DrawFileSelector("Select Texture", "Resource/Texture", texExtensions, selectedTex))
		{
			SetTexture(selectedTex);
		}
		ImGui::Spacing();

		ImGui::ColorEdit4("Color", &m_color.x);
		ImGui::Checkbox("Alpha Blend", &m_useAlphaBlend);
		ImGui::Checkbox("Draw Only When Rect Valid", &m_drawOnlyWhenRectValid);
	}

	void UIImage::Save(json& j) const
	{
		UIElement::Save(j);

		j["TexturePath"] = m_textureFilePath;
		j["VSFilePath"] = m_vsFilePath;
		j["PSFilePath"] = m_psFilePath;

		j["Color"] = m_color; 
		j["AlphaBlend"] = m_useAlphaBlend;
		j["SkipInvalidRect"] = m_drawOnlyWhenRectValid;
	}

	void UIImage::Load(const json& j)
	{
		UIElement::Load(j);

		JsonGet(j, "TexturePath", m_textureFilePath);
		JsonGet(j, "VSFilePath", m_vsFilePath);
		JsonGet(j, "PSFilePath", m_psFilePath);

		JsonGet(j, "Color", m_color);
		JsonGet(j, "AlphaBlend", m_useAlphaBlend);
		JsonGet(j, "SkipInvalidRect", m_drawOnlyWhenRectValid);
		Refresh();
	}

	std::string UIImage::GetType() const
	{
		return "UIImage";
	}

	void UIImage::Refresh()
	{
		SetTexture(m_textureFilePath);
	}
}