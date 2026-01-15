#include "EnginePCH.h"
#include "UIImage.h"

#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Resource/Texture.h"
#include "Core/Graphics/Resource/VertexShader.h"
#include "Core/Graphics/Resource/PixelShader.h"
#include "Core/Graphics/Resource/InputLayout.h"
#include "Core/Graphics/Resource/VertexBuffer.h"
#include "Core/Graphics/Resource/IndexBuffer.h"
#include "Core/Graphics/Resource/SamplerState.h"
#include "Core/Graphics/Resource/BlendState.h"
#include "Core/Graphics/Resource/DepthStencilState.h"
#include "Core/Graphics/Data/ShaderSlotTypes.h"
#include "Core/Graphics/Device/GraphicsDevice.h"

#include "Framework/Object/Component/RectTransform.h"

namespace engine
{
	void UIImage::Initialize()
	{
		UIElement::Initialize();

		m_vs = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/FullscreenQuad_VS.hlsl");
		m_inputLayout = m_vs->GetOrCreateInputLayout<PositionTexCoordVertex>();

		m_sampler = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);
		m_blend = ResourceManager::Get().GetDefaultBlendState(DefaultBlendType::AlphaBlend);
		m_depthNone = ResourceManager::Get().GetDefaultDepthStencilState(DefaultDepthStencilType::None);
	}

	void UIImage::DrawUI() const
	{
		if (!IsActive() || !GetGameObject())
			return;

		// RectTransform 필요
		auto* rt = dynamic_cast<RectTransform*>(GetTransform());
		if (!rt)
			return;

		const UIRect& rect = rt->GetWorldRect();

		if (m_drawOnlyWhenRectValid)
		{
			if (rect.w <= 0.0f || rect.h <= 0.0f)
				return;
		}

		// 텍스처 없으면 스킵
		if (!m_texture)
			return;

		auto& gd = GraphicsDevice::Get();
		auto dc = gd.GetDeviceContext();

		// 기존 게임 뷰포트 저장 후 UI용으로 변경
		const D3D11_VIEWPORT oldVp = gd.GetViewport();

		D3D11_VIEWPORT uiVp{};
		uiVp.TopLeftX = rect.x;
		uiVp.TopLeftY = rect.y;
		uiVp.Width = rect.w;
		uiVp.Height = rect.h;
		uiVp.MinDepth = 0.0f;
		uiVp.MaxDepth = 1.0f;

		dc->RSSetViewports(1, &uiVp);

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
		dc->PSSetShader(m_ps->GetRawShader(), nullptr, 0);

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

			auto samp = m_sampler ? m_sampler->GetSamplerState().GetAddressOf() : nullptr;
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

		// 뷰포트 복구
		dc->RSSetViewports(1, &oldVp);
	}

	void UIImage::SetTexture(const std::string& filePath)
	{
		m_textureFilePath = filePath;
		m_texture = ResourceManager::Get().GetOrCreateTexture(filePath);
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
		if (type != RenderType::Screen)
			return;

		DrawUI();
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
		ImGui::Checkbox("AlphaBlend", &m_useAlphaBlend);
		ImGui::Checkbox("Skip when rect invalid", &m_drawOnlyWhenRectValid);
	}

	void UIImage::Save(json& j) const
	{
		UIElement::Save(j);

		j["TexturePath"] = m_textureFilePath;
		j["AlphaBlend"] = m_useAlphaBlend;
		j["SkipInvalidRect"] = m_drawOnlyWhenRectValid;
	}

	void UIImage::Load(const json& j)
	{
		UIElement::Load(j);

		JsonGet(j, "TexturePath", m_textureFilePath);
		JsonGet(j, "AlphaBlend", m_useAlphaBlend);
		JsonGet(j, "SkipInvalidRect", m_drawOnlyWhenRectValid);

		if (!m_textureFilePath.empty())
		{
			m_texture = ResourceManager::Get().GetOrCreateTexture(m_textureFilePath);
		}
	}

	std::string UIImage::GetType() const
	{
		return "UIImage";
	}
}