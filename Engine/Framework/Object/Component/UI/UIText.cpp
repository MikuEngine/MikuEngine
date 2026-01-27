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

#include "Framework/Asset/FontData.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/Object/Component/RectTransform.h"
#include "Framework/Object/Component/Canvas.h"

// TODO : 여기에 마스크 모드에 따른 클리핑 함수 추가

namespace engine
{
	static bool NextUtf8Codepoint(const char*& p, const char* end, uint32_t& outCp)
	{
		if (p >= end) return false;

		const unsigned char c0 = (unsigned char)*p;

		if (c0 < 0x80) { outCp = c0; ++p; return true; }

		auto need = [&](int n) { return (p + n) <= end; };

		if ((c0 & 0xE0) == 0xC0)
		{
			if (!need(2)) { outCp = '?'; ++p; return true; }
			unsigned char c1 = (unsigned char)p[1];
			if ((c1 & 0xC0) != 0x80) { outCp = '?'; ++p; return true; }
			outCp = ((c0 & 0x1F) << 6) | (c1 & 0x3F);
			p += 2; return true;
		}

		if ((c0 & 0xF0) == 0xE0)
		{
			if (!need(3)) { outCp = '?'; ++p; return true; }
			unsigned char c1 = (unsigned char)p[1];
			unsigned char c2 = (unsigned char)p[2];
			if (((c1 & 0xC0) != 0x80) || ((c2 & 0xC0) != 0x80)) { outCp = '?'; ++p; return true; }
			outCp = ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
			p += 3; return true;
		}

		if ((c0 & 0xF8) == 0xF0)
		{
			if (!need(4)) { outCp = '?'; ++p; return true; }
			unsigned char c1 = (unsigned char)p[1];
			unsigned char c2 = (unsigned char)p[2];
			unsigned char c3 = (unsigned char)p[3];
			if (((c1 & 0xC0) != 0x80) || ((c2 & 0xC0) != 0x80) || ((c3 & 0xC0) != 0x80)) { outCp = '?'; ++p; return true; }
			outCp = ((c0 & 0x07) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
			p += 4; return true;
		}

		outCp = '?'; ++p; return true;
	}

	void UIText::Initialize()
	{
		UIElement::Initialize();

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

	void UIText::DrawUI() const
	{
		if (!IsActive() || !GetGameObject()) return;

		auto* rt = GetRectTransform();
		if (!rt) return;

		if (!m_font) return;

		if (!m_vs || !m_ps || !m_vertexBuffer || !m_indexBuffer || !m_uiCB) return;

		auto& gd = GraphicsDevice::Get();
		auto dc = gd.GetDeviceContext();
		const D3D11_VIEWPORT vp = gd.GetViewport();

		Canvas* c = GetCanvasInParent();
		if (!c) return;

		const Vector2 ref = c->GetReferenceResolution();

		UIRect rootRect{ 0.0f, 0.0f, ref.x, ref.y };
		const UIRect rect = rt->GetWorldRectResolved(rootRect);
		if (rect.w <= 0.0f || rect.h <= 0.0f) return;

		const Vector2 s = c->GetUIScale();
		const Vector2 o = c->GetUIOffset();

		const auto& fd = m_font->GetDesc();
		const float atlasW = static_cast<float>(fd.atlasWidth);
		const float atlasH = static_cast<float>(fd.atlasHeight);

		const float asc = m_font->GetAscenderPx();
		const float lineH = m_font->GetLineHeightPx() * m_lineSpacingMul;

		const char* textBegin = m_text.data();
		const char* textEnd = textBegin + m_text.size();

		// ----------------------------
		// 1) Prepass: 줄 폭 계산 (레퍼런스 좌표계 기준)
		// ----------------------------
		std::vector<float> lineWidths;
		lineWidths.reserve(8);

		{
			const char* p = textBegin;
			float w = 0.0f;

			while (p < textEnd)
			{
				uint32_t cp = 0;
				if (!NextUtf8Codepoint(p, textEnd, cp))
					break;

				if (cp == '\n')
				{
					lineWidths.push_back(w);
					w = 0.0f;
					continue;
				}

				if (cp == '\t')
				{
					w += (m_fontPixelSize * 0.5f) * 4.0f;
					continue;
				}

				const FontGlyph& g = m_font->EnsureGlyph(dc.Get(), cp);
				w += (g.advance + m_letterSpacingPx);
			}

			// 마지막 줄
			lineWidths.push_back(w);
		}

		const int lineCount = (int)lineWidths.size();
		const float blockH = lineH * (float)lineCount;

		// ----------------------------
		// 2) 정렬 계산
		// ----------------------------
		float yOffset = 0.0f;
		switch (m_alignV)
		{
		case UITextAlignV::Top:    yOffset = 0.0f; break;
		case UITextAlignV::Middle: yOffset = (rect.h - blockH) * 0.5f; break;
		case UITextAlignV::Bottom: yOffset = (rect.h - blockH); break;
		}
		if (yOffset < 0.0f) yOffset = 0.0f; // 텍스트가 더 크면 상단 고정

		auto CalcLineStartX = [&](float lw) -> float
			{
				switch (m_alignH)
				{
				case UITextAlignH::Left:   return rect.x;
				case UITextAlignH::Center: return rect.x + (rect.w - lw) * 0.5f;
				case UITextAlignH::Right:  return rect.x + (rect.w - lw);
				}
				return rect.x;
			};

		int lineIndex = 0;
		float penX = CalcLineStartX(lineCount > 0 ? lineWidths[0] : 0.0f);
		float baseLineY = rect.y + yOffset + asc;

		// ----------------------------
		// 3) IA / Shader / State 세팅
		// ----------------------------
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

		// Sampler
		{
			auto samp = m_sampler ? m_sampler->GetSamplerState().GetAddressOf() : nullptr;
			if (samp)
				dc->PSSetSamplers(static_cast<UINT>(SamplerSlot::Linear), 1, samp);
		}

		dc->RSSetState(nullptr);
		dc->RSSetViewports(1, &vp);

		// ----------------------------
		// 4) Draw loop
		// ----------------------------
		const char* p = textBegin;

		while (p < textEnd)
		{
			uint32_t cp = 0;
			if (!NextUtf8Codepoint(p, textEnd, cp))
				break;

			if (cp == '\n')
			{
				lineIndex = std::min(lineIndex + 1, lineCount - 1);
				penX = CalcLineStartX(lineWidths[lineIndex]);
				baseLineY += lineH;
				continue;
			}

			if (cp == '\t')
			{
				penX += (m_fontPixelSize * 0.5f) * 4.0f;
				continue;
			}

			const FontGlyph& g = m_font->EnsureGlyph(dc.Get(), cp);
			const float adv = g.advance + m_letterSpacingPx;

			if (g.IsEmptyBitmap())
			{
				penX += adv;
				continue;
			}

			// 레퍼런스 좌표계 (rootRect/ref)에서 글리프 사각형
			const float gxL = penX + g.bearingX;
			const float gyL = baseLineY - g.bearingY;
			const float gwL = g.width;
			const float ghL = g.height;

			// 최종 뷰포트 픽셀 좌표로 변환 (Canvas scale/offset)
			const float gx = o.x + gxL * s.x;
			const float gy = o.y + gyL * s.y;
			const float gw = gwL * s.x;
			const float gh = ghL * s.y;

			// NDC 변환
			const float cx = gx + gw * 0.5f;
			const float cy = gy + gh * 0.5f;

			const float tx = (cx / vp.Width) * 2.0f - 1.0f;
			const float ty = 1.0f - (cy / vp.Height) * 2.0f;

			const float sx = (gw / vp.Width) * 2.0f;
			const float sy = (gh / vp.Height) * 2.0f;

			// ConstantBuffer
			{
				CbUIElement cbUI{};
				cbUI.clip = DirectX::XMMatrixTranspose(
					DirectX::XMMatrixScaling(sx, sy, 1.0f) *
					DirectX::XMMatrixTranslation(tx, ty, 0.0f)
				);

				cbUI.color = m_color;

				const float u0 = g.x / atlasW;
				const float v0 = g.y / atlasH;
				const float su = (g.w / atlasW);
				const float sv = (g.h / atlasH);
				cbUI.uv = Vector4(u0, v0, su, sv);

				// TODO(원하면): rect 기반 clipRect로 바꾸기
				cbUI.clipRect = Vector4(0, 0, vp.Width, vp.Height);
				cbUI.maskMode = 1;

				dc->UpdateSubresource(m_uiCB->GetRawBuffer(), 0, nullptr, &cbUI, 0, 0);
				dc->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::UIElement), 1, m_uiCB->GetBuffer().GetAddressOf());
				dc->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::UIElement), 1, m_uiCB->GetBuffer().GetAddressOf());
			}

			// Atlas SRV
			{
				ID3D11ShaderResourceView* srv = m_font->GetAtlasSRV(g.page);
				dc->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, &srv);
			}

			dc->DrawIndexed(m_indexBuffer->GetIndexCount(), 0, 0);

			penX += adv;
		}

		// SRV unbind
		{
			ID3D11ShaderResourceView* nullSRV = nullptr;
			dc->PSSetShaderResources(static_cast<UINT>(TextureSlot::Blit), 1, &nullSRV);
		}
	}

	void UIText::SetText(const std::string& utf8)
	{
		m_text = utf8;
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

	void UIText::SetFontPath(const std::string& ttfPath)
	{
		m_fontPath = ttfPath;
		RefreshFont();
	}

	const std::string& UIText::GetFontPath() const
	{
		return m_fontPath;
	}

	void UIText::SetFontPixelSize(int px)
	{
		m_fontPixelSize = std::max(4, px);
		RefreshFont();
	}

	int UIText::GetFontPixelSize() const
	{
		return m_fontPixelSize;
	}

	void UIText::SetAlphaBlend(bool enable)
	{
		m_useAlphaBlend = enable;
	}

	bool UIText::IsAlphaBlend() const
	{
		return m_useAlphaBlend;
	}

	void UIText::SetLetterSpacing(float px)
	{
		m_letterSpacingPx = px;
	}

	float UIText::GetLetterSpacing() const
	{
		return m_letterSpacingPx;
	}

	void UIText::SetLineSpacing(float mul)
	{
		m_lineSpacingMul = mul;
	}

	float UIText::GetLineSpacing() const
	{
		return m_lineSpacingMul;
	}

	bool UIText::HasRenderType(RenderType type) const
	{
		return type == RenderType::Screen;
	}

	void UIText::Draw(RenderType type) const
	{
		UIElement::Draw(type);
	}

	DirectX::BoundingBox UIText::GetBounds() const
	{
		return UIElement::GetBounds();
	}

	void UIText::OnGui()
	{
		UIElement::OnGui();

		ImGui::Text("Font: %s", std::filesystem::path(m_fontPath).filename().string().c_str());

		std::string selected;
		static std::vector<std::string> exts{ ".ttf", ".otf", ".ttc" };

		if (DrawFileSelector("Select Font", "Resource/Font", exts, selected))
		{
			SetFontPath(selected);
		}

		ImGui::InputTextMultiline("Text", &m_text);
		ImGui::ColorEdit4("Color", &m_color.x);

		//ImGui::Checkbox("Alpha Blend", &m_useAlphaBlend);
		
		ImGui::SliderInt("Font Px", &m_fontPixelSize, 1, 256);
		if (ImGui::Button("Rebuild Font"))
			RefreshFont();

		ImGui::Separator();
		const char* hItems[] = { "Left", "Center", "Right" };
		int h = (int)m_alignH;
		if (ImGui::Combo("Align H", &h, hItems, IM_ARRAYSIZE(hItems)))
			m_alignH = (UITextAlignH)h;

		const char* vItems[] = { "Top", "Middle", "Bottom" };
		int v = (int)m_alignV;
		if (ImGui::Combo("Align V", &v, vItems, IM_ARRAYSIZE(vItems)))
			m_alignV = (UITextAlignV)v;

		ImGui::SliderFloat("Letter Spacing(px)", &m_letterSpacingPx, -5.0f, 20.0f);
		ImGui::SliderFloat("Line Spacing(mul)", &m_lineSpacingMul, 0.5f, 3.0f);
	}

	void UIText::Save(json& j) const
	{
		UIElement::Save(j);

		j["Text"] = m_text;
		j["FontPath"] = m_fontPath;
		j["FontPx"] = m_fontPixelSize;

		j["Color"] = m_color;
		j["AlphaBlend"] = m_useAlphaBlend;

		j["AlignH"] = (int)m_alignH;
		j["AlignV"] = (int)m_alignV;

		j["LetterSpacingPx"] = m_letterSpacingPx;
		j["LineSpacingMul"] = m_lineSpacingMul;
	}

	void UIText::Load(const json& j)
	{
		UIElement::Load(j);

		JsonGet(j, "Text", m_text);
		JsonGet(j, "FontPath", m_fontPath);
		JsonGet(j, "FontPx", m_fontPixelSize);

		JsonGet(j, "Color", m_color);
		JsonGet(j, "AlphaBlend", m_useAlphaBlend);

		int ah = (int)UITextAlignH::Left;
		int av = (int)UITextAlignV::Top;

		JsonGet(j, "AlignH", ah);
		JsonGet(j, "AlignV", av);

		m_alignH = (UITextAlignH)ah;
		m_alignV = (UITextAlignV)av;

		JsonGet(j, "LetterSpacingPx", m_letterSpacingPx);
		JsonGet(j, "LineSpacingMul", m_lineSpacingMul);

		RefreshFont();
	}

	void UIText::RefreshFont()
	{
		if (m_fontPath.empty() || m_fontPath == "None")
		{
			m_font.reset();
			return;
		}

		auto device = GraphicsDevice::Get().GetDevice();

		if (!device)
		{
			m_font.reset();
			return;
		}

		std::shared_ptr<FontData> font = std::make_shared<FontData>();

		FontData::Desc d{};
		d.ttfPath = m_fontPath;
		d.pixelSize = m_fontPixelSize;
		d.atlasWidth = 1024;
		d.atlasHeight = 1024;
		d.padding = 1;
		d.atlasFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		d.maxPages = 4;

		if (!font->Initialize(device.Get(), d))
		{
			m_font.reset();
			return;
		}

		m_font = std::move(font);
	}
}