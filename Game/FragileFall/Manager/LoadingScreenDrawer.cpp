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

	static void EnsureResourcesLoaded()
	{
		static bool once = false;
		if (!once)
		{
			auto& rm = engine::ResourceManager::Get();

			g_logoTexture = rm.GetOrCreateTexture("Resource/Texture/Earth.png", engine::LifeScope::Global);
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

			once = true;
		}
	}

	static void DrawUIQuad(
		ID3D11DeviceContext* dc,
		float centerPxX, float centerPxY,
		float sizePxW, float sizePxH,
		const D3D11_VIEWPORT& vp,
		const engine::Vector4& color = engine::Vector4(1, 1, 1, 1),
		const engine::Vector4& uv = engine::Vector4(0, 0, 1, 1))
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

		dc->UpdateSubresource(g_uiCB->GetRawBuffer(), 0, nullptr, &cbUI, 0, 0);
		dc->VSSetConstantBuffers(static_cast<UINT>(engine::ConstantBufferSlot::UIElement), 1, g_uiCB->GetBuffer().GetAddressOf());
		dc->PSSetConstantBuffers(static_cast<UINT>(engine::ConstantBufferSlot::UIElement), 1, g_uiCB->GetBuffer().GetAddressOf());
		dc->DrawIndexed(g_quadIB->GetIndexCount(), 0, 0);
	}

	static void DrawFirstLoadScreen()
	{
		auto& gd = engine::GraphicsDevice::Get();
		auto* dc = gd.GetDeviceContext().Get();
		const D3D11_VIEWPORT vp = gd.GetViewport();

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

		// 1) 가운데 동그란 로고
		dc->PSSetShaderResources(static_cast<UINT>(engine::TextureSlot::Blit), 1, g_logoTexture->GetSRV().GetAddressOf());
		DrawUIQuad(dc, centerX, centerY, LOGO_SIZE_PX, LOGO_SIZE_PX, vp);

		// 2) 로고 외부를 도는 텍스트 이미지
		g_orbitAngle += engine::Time::DeltaTime() * ORBIT_SPEED_RAD_PER_SEC;
		const float orbitX = centerX + ORBIT_RADIUS_PX * cosf(g_orbitAngle);
		const float orbitY = centerY - ORBIT_RADIUS_PX * sinf(g_orbitAngle);

		dc->PSSetShaderResources(static_cast<UINT>(engine::TextureSlot::Blit), 1, g_orbitTextTexture->GetSRV().GetAddressOf());
		DrawUIQuad(dc, orbitX, orbitY, ORBIT_TEXT_WIDTH_PX, ORBIT_TEXT_HEIGHT_PX, vp);

		ID3D11ShaderResourceView* nullSRV = nullptr;
		dc->PSSetShaderResources(static_cast<UINT>(engine::TextureSlot::Blit), 1, &nullSRV);
	}

	static void DrawSceneTransitionScreen(float progress)
	{
		auto& gd = engine::GraphicsDevice::Get();
		auto* dc = gd.GetDeviceContext().Get();
		const D3D11_VIEWPORT vp = gd.GetViewport();

		const float centerX = vp.Width * 0.5f;
		const float animCenterY = vp.Height * SCENE_LOAD_ANIM_Y_RATIO;
		const float barCenterY = vp.Height * SCENE_LOAD_BAR_Y_RATIO;
		const float textCenterY = vp.Height * SCENE_LOAD_TEXT_Y_RATIO;
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
				if (g_sceneLoadAnimData->IsLoop() && duration > 0.0f && g_sceneLoadAnimTime >= duration)
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
					const float uMin = piece.x / sheetW;
					const float vMin = piece.y / sheetH;
					const float uMax = (piece.x + piece.width) / sheetW;
					const float vMax = (piece.y + piece.height) / sheetH;
					const engine::Vector4 uv(uMin, vMin, uMax, vMax);
					DrawUIQuad(dc, centerX, animCenterY, SCENE_LOAD_ANIM_SIZE_PX, SCENE_LOAD_ANIM_SIZE_PX, vp, engine::Vector4(1, 1, 1, 1), uv);
				}
				else
					DrawUIQuad(dc, centerX, animCenterY, SCENE_LOAD_ANIM_SIZE_PX, SCENE_LOAD_ANIM_SIZE_PX, vp);
			}
			else
				DrawUIQuad(dc, centerX, animCenterY, SCENE_LOAD_ANIM_SIZE_PX, SCENE_LOAD_ANIM_SIZE_PX, vp);
		}

		// 2) 그 아래: 프로그레스 바
		dc->PSSetShaderResources(static_cast<UINT>(engine::TextureSlot::Blit), 1, g_whiteTexture->GetSRV().GetAddressOf());
		DrawUIQuad(dc, centerX, barCenterY, BAR_WIDTH_PX, BAR_HEIGHT_PX, vp, barBgColor);

		const float clampedProgress = (progress < 0.0f) ? 0.0f : (progress > 1.0f) ? 1.0f : progress;
		const float fillWidth = (BAR_WIDTH_PX * clampedProgress < 0.001f) ? 0.001f : BAR_WIDTH_PX * clampedProgress;
		const float fillCenterX = centerX - (BAR_WIDTH_PX - fillWidth) * 0.5f;
		DrawUIQuad(dc, fillCenterX, barCenterY, fillWidth, BAR_HEIGHT_PX, vp, barFillColor);

		// 3) 그 아래: '로딩중...' 텍스트 이미지
		if (g_loadingTextTexture)
		{
			dc->PSSetShaderResources(static_cast<UINT>(engine::TextureSlot::Blit), 1, g_loadingTextTexture->GetSRV().GetAddressOf());
			DrawUIQuad(dc, centerX, textCenterY, SCENE_LOAD_TEXT_WIDTH_PX, SCENE_LOAD_TEXT_HEIGHT_PX, vp);
		}

		ID3D11ShaderResourceView* nullSRV = nullptr;
		dc->PSSetShaderResources(static_cast<UINT>(engine::TextureSlot::Blit), 1, &nullSRV);
	}

	void LoadingScreenDrawer::Draw(float progress)
	{
		EnsureResourcesLoaded();

		if (g_isFirstLoad)
		{
			DrawFirstLoadScreen();
		}
		else
		{
			DrawSceneTransitionScreen(progress);
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
