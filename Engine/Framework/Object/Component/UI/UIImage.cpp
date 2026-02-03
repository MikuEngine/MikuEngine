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

	void UIImage::SetRampTexture(const std::string& textureFilePath)
	{
		if (textureFilePath.empty()) return;

		m_rampPath = textureFilePath;

		if (textureFilePath == "None")
		{
			m_rampTex = ResourceManager::Get().GetDefaultTexture(DefaultTextureType::White);
			return;
		}

		std::shared_ptr<engine::Texture> tex = ResourceManager::Get().GetOrCreateTexture(textureFilePath);
		if (!tex)
		{
			LOG_PRINT("FATAL: Texture not found: %s", textureFilePath.c_str());
			return;
		}

		m_rampTex = tex;
	}

	void UIImage::SetNoiseTexture(const std::string& textureFilePath)
	{
		if (textureFilePath.empty()) return;

		m_noisePath = textureFilePath;

		if (textureFilePath == "None")
		{
			m_noiseTex = ResourceManager::Get().GetDefaultTexture(DefaultTextureType::White);
			return;
		}

		std::shared_ptr<engine::Texture> tex = ResourceManager::Get().GetOrCreateTexture(textureFilePath);
		if (!tex)
		{
			LOG_PRINT("FATAL: Texture not found: %s", textureFilePath.c_str());
			return;
		}

		m_noiseTex = tex;
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

	// ================================
	// Effect API
	// ================================

	void UIImage::ClearEffect()
	{
		m_effectMode = 0;
		m_effectFlags = 0;
		m_effect0 = Vector4(0, 0, 0, 0);
		m_effect1 = Vector4(0, 0, 0, 0);
		m_effect2 = Vector4(0, 0, 0, 0);
	}

	void UIImage::SetEffectScanLine(float density, float speed, float opacity)
	{
		m_effectMode = 1; // UI_FX_SCANLINE
		m_effect0 = Vector4(density, speed, opacity, 0.0f);
	}

	void UIImage::SetEffectGlowPulse(float speed, float minIntensity, float maxIntensity)
	{
		m_effectMode = 2; // UI_FX_GLOW_PULSE
		m_effect0 = Vector4(speed, minIntensity, maxIntensity, 0.0f);
	}

	void UIImage::SetEffectPixelate(float pixelSize)
	{
		m_effectMode = 3; // UI_FX_PIXELATE
		// pixelSize가 커질수록 픽셀이 뭉쳐 보임 (추천: 8.0 ~ 64.0)
		m_effect0 = Vector4(pixelSize, 0.0f, 0.0f, 0.0f);
	}

	void UIImage::SetEffectHoverTransition(bool isHover)
	{
		m_effectMode = 4; // UI_FX_HOVER_TRANSITION
	}

	void UIImage::SetEffectAbyssalDecay()
	{
		m_effectMode = 5; // UI_FX_ABYSSAL_DECAY
		SetNoiseTexture(m_noisePath);
	}

	void UIImage::SetEffectStaticNoise(float intensity)
	{
		m_effectMode = 6; // UI_FX_STATIC_NOISE
		m_effect0.x = intensity;
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

		// Use Effect toggle
		ImGui::TextDisabled("Effect (Optional)");

		static std::vector<std::string> rampExtensions{ ".png", ".jpg", ".tga" };
		static std::vector<std::string> noiseExtensions{ ".png", ".jpg", ".tga" };

		bool useEffect = (m_effectMode != 0);
		if (ImGui::Checkbox("Use Effect", &useEffect))
		{
			if (!useEffect)
			{
				ClearEffect(); // effectMode = 0
			}
			else
			{
				m_effectMode = 1; // 기본값: FlameProgress (임시)
			}
		}

		const char* effectNames[] = {
		"None",               // 0
		"Scanline",           // 1
		"Glow Pulse",         // 2
		"Pixelate",           // 3
		"Hover Transition",   // 4
		"Abyssal Decay",      // 5
		"Static Noise",       // 6
		"Flame Bar (Progress)"// 10
		};

		// 현재 m_effectMode에 맞는 인덱스 찾기 (10번 같은 경우 예외처리)
		int effectIdx = 0;
		if (m_effectMode >= 1 && m_effectMode <= 6) effectIdx = (int)m_effectMode;
		else if (m_effectMode == 10) effectIdx = 7; // Flame Bar

		if (ImGui::Combo("Effect Type", &effectIdx, effectNames, IM_ARRAYSIZE(effectNames)))
		{
			ClearEffect(); // 모드 변경 시 파라미터 초기화
			if (effectIdx == 7) m_effectMode = 10;
			else m_effectMode = (uint32_t)effectIdx;
		}

		if (m_effectMode != 0)
		{
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Effect Settings");

			// [2] 효과별 맞춤형 파라미터 노출
			switch (m_effectMode)
			{
			case 1: // Scanline
				ImGui::DragFloat("Density", &m_effect0.x, 1.0f, 1.0f, 500.0f);
				ImGui::DragFloat("Speed", &m_effect0.y, 0.1f, -50.0f, 50.0f);
				ImGui::DragFloat("Opacity", &m_effect0.z, 0.01f, 0.0f, 1.0f);
				break;

			case 2: // Glow Pulse
				ImGui::DragFloat("Pulse Speed", &m_effect0.x, 0.1f, 0.0f, 20.0f);
				ImGui::DragFloat("Min Intensity", &m_effect0.y, 0.01f, 0.0f, 2.0f);
				ImGui::DragFloat("Max Intensity", &m_effect0.z, 0.01f, 0.0f, 5.0f);
				break;

			case 3: // Pixelate
				ImGui::DragFloat("Pixel Size", &m_effect0.x, 1.0f, 1.0f, 512.0f);
				break;

			case 4: // Hover Transition
				ImGui::Text("Animation Start: %.2f", m_effect1.x);
				if (ImGui::Button("Test Trigger")) m_effect1.x = engine::Time::UnscaledTime();
				break;

			case 5: // Abyssal Decay
				ImGui::Text("Uses Noise Texture");
				break;

			case 6: // Static Noise
				ImGui::DragFloat("Noise Intensity", &m_effect0.x, 0.01f, 0.0f, 1.0f);
				break;

			case 10: // Flame Bar
				ImGui::DragFloat("Feather", &m_effect0.x, 0.001f, 0.0f, 0.1f);
				ImGui::DragFloat("Head Width", &m_effect0.y, 0.001f, 0.0f, 0.2f);
				ImGui::DragFloat("Flame Intensity", &m_effect2.y, 0.1f, 0.0f, 10.0f);
				break;
			}

			// [3] 텍스처 설정 (필요한 경우에만 노출)
			if (m_effectMode == 5 || m_effectMode == 10)
			{
				ImGui::Separator();
				ImGui::Text("Resource Settings");

				// Noise 텍스처 선택 UI...
				ImGui::Text("Noise: %s", std::filesystem::path(m_noisePath).filename().string().c_str());
				ImGui::SameLine();
				if (ImGui::Button("None##NoiseTex"))
				{
					SetNoiseTexture("None");
				}
				
				std::string selectedNoise;
				if (DrawFileSelector("Select Noise", "Resource/Texture", noiseExtensions, selectedNoise))
					SetNoiseTexture(selectedNoise);

				if (m_effectMode == 10) // FlameBar 전용 Ramp
				{
					ImGui::Text("Ramp: %s", std::filesystem::path(m_rampPath).filename().string().c_str());
					std::string selectedRamp;
					if (DrawFileSelector("Select Ramp", "Resource/Texture", rampExtensions, selectedRamp))
						SetRampTexture(selectedRamp);
				}
			}
		}

		ImGui::Spacing();

		if (GameObject* go = GetGameObject())
		{
			if (auto* btn = go->GetComponent<UIButton>())
			{
				ImGui::Separator();
				ImGui::TextDisabled("This Image is controlled by UIButton (SpriteSwap/ Tint).");
				return;
			}
		}

		static std::vector<std::string> texExtensions{ ".png", ".jpg", ".tga"};

		ImGui::Text("Texture: %s", std::filesystem::path(m_textureFilePath).filename().string().c_str());
		std::string selectedTex;

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

		j["EffectMode"] = m_effectMode;
		j["Effect0"] = m_effect0;
		j["Effect1"] = m_effect1;
		j["Effect2"] = m_effect2;

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

		JsonGet(j, "EffectMode", m_effectMode);
		JsonGet(j, "Effect0", m_effect0);
		JsonGet(j, "Effect1", m_effect1);
		JsonGet(j, "Effect2", m_effect2);

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
		SetNoiseTexture(m_noisePath);
		SetRampTexture(m_rampPath);
	}
}