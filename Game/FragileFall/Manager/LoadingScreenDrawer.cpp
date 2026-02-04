#include "GamePCH.h"
#include "LoadingScreenDrawer.h"

#include <Core/Graphics/Device/GraphicsDevice.h>
#include <Core/Graphics/Resource/ResourceManager.h>
#include <Core/Graphics/Resource/Texture.h>
#include <Core/Graphics/Resource/PixelShader.h>
#include <Core/Graphics/Resource/VertexShader.h>
#include <Core/Graphics/Resource/SamplerState.h>
#include <Core/Graphics/Resource/ConstantBuffer.h>
#include <Core/Graphics/Resource/VertexBuffer.h>
#include <Core/Graphics/Resource/IndexBuffer.h>
#include <Core/Graphics/Resource/InputLayout.h>
#include <Core/Graphics/Resource/BlendState.h>
#include <Core/Graphics/Resource/DepthStencilState.h>
#include <Core/Graphics/Data/ShaderSlotTypes.h>
#include <Core/Graphics/Data/ConstantBufferTypes.h>
#include <Core/Graphics/Data/Vertex.h>
#include <Core/Graphics/Resource/DefaultResourceTypes.h>
#include <Core/System/MyTime.h>
#include <Common/Utility/CommonTypes.h>
#include <Framework/Asset/AssetManager.h>
#include <Framework/Asset/SpriteData.h>
#include <Framework/Asset/SpriteAnimationData.h>
#include <Framework/Asset/FontData.h>	

namespace game
{
	namespace
	{
		bool g_isFirstLoad = true;

		std::shared_ptr<engine::Texture> g_logoTexture;
		std::shared_ptr<engine::Texture> g_orbitTextTexture;
		std::shared_ptr<engine::PixelShader> g_blitPS;
		std::shared_ptr<engine::SamplerState> g_linearSampler;

		// UI quad 그리기용 (가운데 로고 + 궤도 텍스트)
		std::shared_ptr<engine::VertexShader> g_uiVS;
		std::shared_ptr<engine::PixelShader> g_uiPS;
		std::shared_ptr<engine::ConstantBuffer> g_uiCB;
		std::shared_ptr<engine::VertexBuffer> g_quadVB;
		std::shared_ptr<engine::IndexBuffer> g_quadIB;
		std::shared_ptr<engine::InputLayout> g_inputLayout;
		std::shared_ptr<engine::BlendState> g_blendAlpha;
		std::shared_ptr<engine::DepthStencilState> g_depthNone;
		std::shared_ptr<engine::FontData> g_loadingFont;

		float g_orbitAngle = 0.0f;

		constexpr float LOGO_SIZE_PX = 200.0f;
		constexpr float ORBIT_RADIUS_PX = 180.0f;
		constexpr float ORBIT_TEXT_WIDTH_PX = 80.0f;
		constexpr float ORBIT_TEXT_HEIGHT_PX = 32.0f;
		constexpr float ORBIT_SPEED_RAD_PER_SEC = 1.2f;

		constexpr float BAR_WIDTH_PX = 400.0f;
		constexpr float BAR_HEIGHT_PX = 20.0f;
		constexpr float BAR_Y_OFFSET_FROM_BOTTOM = 80.0f;

		// 씬 전환 로딩 화면 (위→아래: 애니메이션, 프로그레스 바, 로딩중...)
		std::shared_ptr<engine::Texture> g_sceneLoadAnimTexture;
		std::shared_ptr<engine::SpriteData> g_sceneLoadSpriteData;
		std::shared_ptr<engine::SpriteAnimationData> g_sceneLoadAnimData;
		float g_sceneLoadAnimTime = 0.0f;
		std::shared_ptr<engine::Texture> g_loadingTextTexture;
		constexpr float SCENE_LOAD_ANIM_Y_RATIO = 0.28f;
		constexpr float SCENE_LOAD_BAR_Y_RATIO = 0.52f;
		constexpr float SCENE_LOAD_TEXT_Y_RATIO = 0.72f;
		constexpr float SCENE_LOAD_ANIM_SIZE_PX = 100.0f;
		constexpr float SCENE_LOAD_TEXT_WIDTH_PX = 200.0f;
		constexpr float SCENE_LOAD_TEXT_HEIGHT_PX = 40.0f;

		std::shared_ptr<engine::Texture> g_whiteTexture;
	}

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

	static void EnsureResourcesLoaded()
	{
		static bool once = false;
		if (!once)
		{
			auto& rm = engine::ResourceManager::Get();

			g_logoTexture = rm.GetOrCreateTexture("Resource/Texture/Miku.png", engine::LifeScope::Global);
			g_orbitTextTexture = rm.GetOrCreateTexture("Resource/Texture/ExcutionTargetTemp.png", engine::LifeScope::Global);
			g_sceneLoadAnimTexture = rm.GetOrCreateTexture("Resource/Texture/Flame.png", engine::LifeScope::Global);
			g_sceneLoadSpriteData = engine::AssetManager::Get().GetOrCreateSpriteData("Resource/Data/SpriteSheet/Flame.spritedata", engine::LifeScope::Global);
			g_sceneLoadAnimData = engine::AssetManager::Get().GetOrCreateSpriteAnimationData("Resource/Data/SpriteAnim/Flame_a.animdata", engine::LifeScope::Global);
			if (g_sceneLoadSpriteData && g_sceneLoadAnimData)
				g_sceneLoadAnimData->SetupFramesIndex(g_sceneLoadSpriteData.get());
			g_loadingTextTexture = rm.GetOrCreateTexture("Resource/Texture/LoadingText.png", engine::LifeScope::Global);
			g_blitPS = rm.GetOrCreatePixelShader("Resource/Shader/Pixel/Blit_PS.hlsl");
			g_linearSampler = rm.GetDefaultSamplerState(engine::DefaultSamplerType::Linear);

			g_uiVS = rm.GetOrCreateVertexShader("Resource/Shader/Vertex/UIQuad_VS.hlsl");
			g_uiPS = rm.GetOrCreatePixelShader("Resource/Shader/Pixel/UIQuad_PS.hlsl");
			g_uiCB = rm.GetOrCreateConstantBuffer("LoadingScreenUI", sizeof(engine::CbUIElement));
			g_quadVB = rm.GetGeometryVertexBuffer("DefaultQuad");
			g_quadIB = rm.GetGeometryIndexBuffer("DefaultQuad");
			g_inputLayout = g_uiVS->GetOrCreateInputLayout<engine::PositionTexCoordVertex>();
			g_blendAlpha = rm.GetDefaultBlendState(engine::DefaultBlendType::AlphaBlend);
			g_depthNone = rm.GetDefaultDepthStencilState(engine::DefaultDepthStencilType::None);
			g_whiteTexture = rm.GetDefaultTexture(engine::DefaultTextureType::White);

			// 폰트생성
			g_loadingFont = std::make_shared<engine::FontData>();
			engine::FontData::Desc fd{};
			fd.ttfPath = "Resource/Font/malgun.ttf";
			fd.pixelSize = 40; // 생성할 텍스처의 퀄리티 기준
			fd.atlasWidth = 1024;
			fd.atlasHeight = 1024;
			fd.maxPages = 1;
			fd.atlasFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

			g_loadingFont->Initialize(engine::GraphicsDevice::Get().GetDevice().Get(), fd);

			once = true;
		}
	}

	static void DrawUIQuad(
		ID3D11DeviceContext* dc,
		float centerPxX, float centerPxY,
		float sizePxW, float sizePxH,
		const D3D11_VIEWPORT& vp,
		const engine::Vector4& color = engine::Vector4(1, 1, 1, 1),
		const engine::Vector4& uv = engine::Vector4(0, 0, 1, 1),
		uint32_t maskMode = 0)
	{
		const float tx = (centerPxX / vp.Width) * 2.0f - 1.0f;
		const float ty = 1.0f - (centerPxY / vp.Height) * 2.0f;
		const float sx = (sizePxW / vp.Width) * 2.0f;
		const float sy = (sizePxH / vp.Height) * 2.0f;

		engine::CbUIElement cbUI = {};
		cbUI.clip = DirectX::XMMatrixTranspose(
			DirectX::XMMatrixScaling(sx, sy, 1.0f) *
			DirectX::XMMatrixTranslation(tx, ty, 0.0f)
		);
		cbUI.color = color;
		cbUI.uv = uv;
		cbUI.clipRect = engine::Vector4(0, 0, vp.Width, vp.Height);
		cbUI.maskMode = 0;
		cbUI.outlineEnabled = 0.0f;

		cbUI.maskMode = maskMode;

		if (maskMode == 2) // UI_MASK_CIRCLE
		{
			// g_uiMask0.xy = 중심 좌표, z = 반지름
			// 반지름은 가로 크기의 절반을 사용합니다.
			cbUI.mask0 = engine::Vector4(centerPxX, centerPxY, sizePxW * 0.5f, 0.0f);
		}

		cbUI.outlineEnabled = 0.0f;

		dc->UpdateSubresource(g_uiCB->GetRawBuffer(), 0, nullptr, &cbUI, 0, 0);
		dc->VSSetConstantBuffers(static_cast<UINT>(engine::ConstantBufferSlot::UIElement), 1, g_uiCB->GetBuffer().GetAddressOf());
		dc->PSSetConstantBuffers(static_cast<UINT>(engine::ConstantBufferSlot::UIElement), 1, g_uiCB->GetBuffer().GetAddressOf());
		dc->DrawIndexed(g_quadIB->GetIndexCount(), 0, 0);
	}

	static void DrawTextQuad(
		ID3D11DeviceContext* dc,
		std::shared_ptr<engine::FontData> font,
		const std::string& text,
		float startPxX, float startPxY, // 텍스트 시작 위치
		float fontSizePx, // 폰트 크기 (높이 기준)
		const D3D11_VIEWPORT& vp,
		const engine::Vector4& color = engine::Vector4(1, 1, 1, 1),
		float scale = 1.0f) // Draw 함수의 scale 인자를 받아야 함
	{
		if (!font || text.empty()) return;

		const float fontScale = scale; // 폰트 생성 시 pixelSize를 이미 맞췄다면 scale만 적용
		const float asc = font->GetAscenderPx() * fontScale;
		const float lineH = font->GetLineHeightPx() * fontScale;

		const char* p = text.data();
		const char* end = p + text.size();

		float penX = startPxX;
		float baseLineY = startPxY + asc;

		while (p < end)
		{
			uint32_t cp = 0;
			if (!NextUtf8Codepoint(p, end, cp)) break;

			if (cp == '\n') {
				penX = startPxX;
				baseLineY += lineH;
				continue;
			}

			// Glyph 데이터 확보
			const auto& glyph = font->EnsureGlyph(dc, cp);
			if (glyph.IsEmptyBitmap()) {
				penX += glyph.advance * fontScale;
				continue;
			}

			// UIText의 좌표 계산 로직과 동일
			const float gx = penX + glyph.bearingX * fontScale;
			const float gy = baseLineY - glyph.bearingY * fontScale;
			const float gw = glyph.width * fontScale;
			const float gh = glyph.height * fontScale;

			// UV 좌표 계산
			const auto& fd = font->GetDesc();
			const engine::Vector4 uv(
				glyph.x / (float)fd.atlasWidth,
				glyph.y / (float)fd.atlasHeight,
				glyph.w / (float)fd.atlasWidth,
				glyph.h / (float)fd.atlasHeight
			);

			// 아틀라스 텍스처 바인딩 (글자마다 페이지가 다를 수 있음)
			ID3D11ShaderResourceView* srv = font->GetAtlasSRV(glyph.page);
			dc->PSSetShaderResources((UINT)engine::TextureSlot::Blit, 1, &srv);

			// 기존에 만든 DrawUIQuad 재활용
			DrawUIQuad(dc, gx + gw * 0.5f, gy + gh * 0.5f, gw, gh, vp, color, uv, 0);

			penX += glyph.advance * fontScale;
		}
	}

	static void DrawFirstLoadScreen(const D3D11_VIEWPORT& vp, float scale)
	{
		auto& gd = engine::GraphicsDevice::Get();
		auto* dc = gd.GetDeviceContext().Get();

		const float centerX = vp.Width * 0.5f;
		const float centerY = vp.Height * 0.5f;

		// IA
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dc->IASetInputLayout(g_inputLayout->GetRawInputLayout());
		{
			UINT stride = g_quadVB->GetBufferStride();
			UINT offset = 0;
			ID3D11Buffer* vb = g_quadVB->GetRawBuffer();
			dc->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
			dc->IASetIndexBuffer(g_quadIB->GetRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
		}

		// State
		{
			float blendFactor[4] = { 0, 0, 0, 0 };
			dc->OMSetBlendState(g_blendAlpha->GetBlendState().Get(), blendFactor, 0xffffffff);
			dc->OMSetDepthStencilState(g_depthNone->GetDepthStencilState().Get(), 0);
		}

		dc->VSSetShader(g_uiVS->GetRawShader(), nullptr, 0);
		dc->PSSetShader(g_uiPS->GetRawShader(), nullptr, 0);
		dc->PSSetSamplers(static_cast<UINT>(engine::SamplerSlot::Linear), 1, g_linearSampler->GetSamplerState().GetAddressOf());

		// 스케일링된 수치 계산
		const float sLogoSize = LOGO_SIZE_PX * scale;
		const float sOrbitRadius = ORBIT_RADIUS_PX * scale;
		const float sOrbitW = ORBIT_TEXT_WIDTH_PX * scale;
		const float sOrbitH = ORBIT_TEXT_HEIGHT_PX * scale;

		// 1) 가운데 동그란 로고
		dc->PSSetShaderResources(static_cast<UINT>(engine::TextureSlot::Blit), 1, g_logoTexture->GetSRV().GetAddressOf());
		DrawUIQuad(dc, centerX, centerY, sLogoSize, sLogoSize, vp, engine::Vector4(1, 1, 1, 1), engine::Vector4(0, 0, 1, 1), 2);

		// 2) 로고 외부를 도는 텍스트 이미지
		g_orbitAngle += engine::Time::DeltaTime() * ORBIT_SPEED_RAD_PER_SEC;
		const float orbitX = centerX + sOrbitRadius * cosf(g_orbitAngle);
		const float orbitY = centerY - sOrbitRadius * sinf(g_orbitAngle);

		dc->PSSetShaderResources(static_cast<UINT>(engine::TextureSlot::Blit), 1, g_orbitTextTexture->GetSRV().GetAddressOf());
		DrawUIQuad(dc, orbitX, orbitY, sOrbitW, sOrbitH, vp);

		// 2) 로고 외부를 도는 텍스트 이미지
		//if (g_loadingFont)
		//{
		//	std::string loadingStr = "NOW LOADING...";

		//	// 중앙 정렬을 위한 가로 폭 사전 계산
		//	float totalWidth = 0.0f;
		//	const char* p = loadingStr.data();
		//	const char* end = p + loadingStr.size();
		//	uint32_t cp;
		//	while (NextUtf8Codepoint(p, end, cp)) {
		//		totalWidth += g_loadingFont->EnsureGlyph(dc, cp).advance;
		//	}

		//	// 로고에서 일정 거리만큼 아래에 배치
		//	const float textStartX = centerX - (totalWidth * scale * 0.5f);
		//	const float textStartY = centerY + (sLogoSize * 0.5f) + (40.0f * scale);

		//	DrawTextQuad(dc, g_loadingFont, loadingStr, textStartX, textStartY, 40, vp, engine::Vector4(1, 1, 1, 1), scale);
		//}

		ID3D11ShaderResourceView* nullSRV = nullptr;
		dc->PSSetShaderResources(static_cast<UINT>(engine::TextureSlot::Blit), 1, &nullSRV);
	}

	static void DrawSceneTransitionScreen(float progress, const D3D11_VIEWPORT& vp, float scale)
	{
		auto& gd = engine::GraphicsDevice::Get();
		auto* dc = gd.GetDeviceContext().Get();

		const float centerX = vp.Width * 0.5f;
		const float animCenterY = vp.Height * SCENE_LOAD_ANIM_Y_RATIO;
		const float barCenterY = vp.Height * SCENE_LOAD_BAR_Y_RATIO;
		const float textCenterY = vp.Height * SCENE_LOAD_TEXT_Y_RATIO;

		// 스케일링된 크기 수치
		const float sAnimSize = SCENE_LOAD_ANIM_SIZE_PX * scale;
		const float sBarW = BAR_WIDTH_PX * scale;
		const float sBarH = BAR_HEIGHT_PX * scale;
		const float sTextW = SCENE_LOAD_TEXT_WIDTH_PX * scale;
		const float sTextH = SCENE_LOAD_TEXT_HEIGHT_PX * scale;

		const engine::Vector4 barBgColor(0.2f, 0.2f, 0.2f, 0.9f);
		const engine::Vector4 barFillColor(0.4f, 0.7f, 1.0f, 1.0f);

		// IA
		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dc->IASetInputLayout(g_inputLayout->GetRawInputLayout());
		{
			UINT stride = g_quadVB->GetBufferStride();
			UINT offset = 0;
			ID3D11Buffer* vb = g_quadVB->GetRawBuffer();
			dc->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
			dc->IASetIndexBuffer(g_quadIB->GetRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
		}

		float blendFactor[4] = { 0, 0, 0, 0 };
		dc->OMSetBlendState(g_blendAlpha->GetBlendState().Get(), blendFactor, 0xffffffff);
		dc->OMSetDepthStencilState(g_depthNone->GetDepthStencilState().Get(), 0);

		dc->VSSetShader(g_uiVS->GetRawShader(), nullptr, 0);
		dc->PSSetShader(g_uiPS->GetRawShader(), nullptr, 0);
		dc->PSSetSamplers(static_cast<UINT>(engine::SamplerSlot::Linear), 1, g_linearSampler->GetSamplerState().GetAddressOf());

		// 1) 맨 위: 애니메이션 (중앙) — 스프라이트 애니 있으면 재생, 없으면 단일 텍스처
		if (g_sceneLoadAnimTexture)
		{
			dc->PSSetShaderResources(static_cast<UINT>(engine::TextureSlot::Blit), 1, g_sceneLoadAnimTexture->GetSRV().GetAddressOf());

			if (g_sceneLoadSpriteData && g_sceneLoadAnimData && !g_sceneLoadAnimData->GetFrames().empty())
			{
				g_sceneLoadAnimTime += engine::Time::DeltaTime();
				float duration = g_sceneLoadAnimData->GetDuration();
				if (duration > 0.0f && g_sceneLoadAnimTime >= duration)
					g_sceneLoadAnimTime = fmodf(g_sceneLoadAnimTime, duration);

				const auto& frames = g_sceneLoadAnimData->GetFrames();
				size_t frameIndex = 0;
				for (size_t i = 0; i < frames.size(); ++i)
				{
					if (g_sceneLoadAnimTime >= frames[i].time)
						frameIndex = i;
					else
						break;
				}

				const engine::SpritePiece& piece = g_sceneLoadSpriteData->GetSpritePiece(frames[frameIndex].pieceIndex);
				const float sheetW = g_sceneLoadSpriteData->GetWidth();
				const float sheetH = g_sceneLoadSpriteData->GetHeight();
				if (sheetW > 0.0f && sheetH > 0.0f)
				{
					// CbUIElement.uv = offset.xy, scale.xy (SpriteRenderer/SpriteAnimator와 동일)
					const float uOffset = piece.x / sheetW;
					const float vOffset = piece.y / sheetH;
					const float uScale = piece.width / sheetW;
					const float vScale = piece.height / sheetH;
					const engine::Vector4 uv(uOffset, vOffset, uScale, vScale);
					DrawUIQuad(dc, centerX, animCenterY, sAnimSize, sAnimSize, vp, engine::Vector4(1, 1, 1, 1), uv);
				}
				else
					DrawUIQuad(dc, centerX, animCenterY, sAnimSize, sAnimSize, vp);
			}
			else
				DrawUIQuad(dc, centerX, animCenterY, sAnimSize, sAnimSize, vp);
		}

		// 2) 그 아래: 프로그레스 바
		dc->PSSetShaderResources(static_cast<UINT>(engine::TextureSlot::Blit), 1, g_whiteTexture->GetSRV().GetAddressOf());
		DrawUIQuad(dc, centerX, barCenterY, sBarW, sBarH, vp, barBgColor);

		const float clampedProgress = (progress < 0.0f) ? 0.0f : (progress > 1.0f) ? 1.0f : progress;
		const float fillWidth = (sBarW * clampedProgress < 0.001f) ? 0.001f : sBarW * clampedProgress;
		const float fillCenterX = centerX - (sBarW - fillWidth) * 0.5f;
		DrawUIQuad(dc, fillCenterX, barCenterY, fillWidth, sBarH, vp, barFillColor);

		// 3) 그 아래: '로딩중...' 텍스트 이미지
		if (g_loadingTextTexture)
		{
			dc->PSSetShaderResources(static_cast<UINT>(engine::TextureSlot::Blit), 1, g_loadingTextTexture->GetSRV().GetAddressOf());
			DrawUIQuad(dc, centerX, textCenterY, sTextW, sTextH, vp);
		}

		ID3D11ShaderResourceView* nullSRV = nullptr;
		dc->PSSetShaderResources(static_cast<UINT>(engine::TextureSlot::Blit), 1, &nullSRV);
	}

	void LoadingScreenDrawer::Draw(float progress)
	{
		EnsureResourcesLoaded();

		auto& gd = engine::GraphicsDevice::Get();
		const D3D11_VIEWPORT vp = gd.GetViewport();

		const float scale = vp.Height / 1080.0f;

		if (g_isFirstLoad)
		{
			DrawFirstLoadScreen(vp, scale);
		}
		else
		{
			DrawSceneTransitionScreen(progress, vp, scale);
		}
	}

	void LoadingScreenDrawer::OnFirstLoadFinished()
	{
		g_isFirstLoad = false;
	}

	void LoadingScreenDrawer::OnShutdown()
	{
		g_logoTexture.reset();
		g_orbitTextTexture.reset();
		g_sceneLoadAnimTexture.reset();
		g_sceneLoadSpriteData.reset();
		g_sceneLoadAnimData.reset();
		g_loadingTextTexture.reset();
		g_whiteTexture.reset();
		g_blitPS.reset();
		g_linearSampler.reset();
		g_uiVS.reset();
		g_uiPS.reset();
		g_uiCB.reset();
		g_quadVB.reset();
		g_quadIB.reset();
		g_inputLayout.reset();
		g_blendAlpha.reset();
		g_depthNone.reset();
	}
}
