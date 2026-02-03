#pragma once

#include "Framework/Object/Component/UI/UIElement.h"

namespace engine
{
	class Texture;
	class ConstantBuffer;
	class VertexShader;
	class PixelShader;
	class InputLayout;
	class VertexBuffer;
	class IndexBuffer;
	class SamplerState;
	class BlendState;
	class DepthStencilState;

	class UIImage : public UIElement
	{
		REGISTER_COMPONENT(UIImage, UIElement)

	private:
		std::string m_textureFilePath = "None";
		std::string m_vsFilePath;
		std::string m_psFilePath;
		std::string m_outlinePSFilePath;
		std::shared_ptr<Texture> m_texture;

		std::shared_ptr<VertexShader> m_vs;
		std::shared_ptr<PixelShader>  m_ps;
		std::shared_ptr<PixelShader>  m_outlinePS;
		std::shared_ptr<InputLayout>  m_inputLayout;

		std::shared_ptr<VertexBuffer> m_vertexBuffer;
		std::shared_ptr<IndexBuffer> m_indexBuffer;

		std::shared_ptr<SamplerState> m_sampler;
		std::shared_ptr<BlendState> m_blend;
		std::shared_ptr<DepthStencilState> m_depthNone;

		std::shared_ptr<ConstantBuffer> m_uiCB;

		Vector4 m_color = Vector4(1, 1, 1, 1);
		Vector4 m_uv = Vector4(0, 0, 1, 1);

		bool m_useAlphaBlend = true;
		bool m_dirty = true;

		MaskMode m_maskMode = MaskMode::Rect;
		Vector4  m_clipRect = Vector4(0, 0, 0, 0);

		Vector4 m_mask0 = Vector4(0, 0, 0, 0);
		Vector4 m_mask1 = Vector4(0, 0, 0, 0);

		bool m_outlineEnabled = false;
		float m_outlineThickness = 0.0f;
		Vector4 m_outlineColor = { 0.0f, 0.0f, 0.0f, 0.0f };

		enum class EffectMode
		{

		};

		uint32_t m_effectMode = 0;
		uint32_t m_effectFlags = 0;

		Vector4 m_effect0 = { 0,0,0,0 };
		Vector4 m_effect1 = { 0,0,0,0 };
		Vector4 m_effect2 = { 0,0,0,0 };

		std::shared_ptr<Texture> m_noiseTex;
		std::shared_ptr<Texture> m_rampTex;
		std::string m_noisePath = "Resource/Texture/Noise.png";
		std::string m_rampPath = "Resource/Texture/Ramp.png";

	public:
		UIImage() = default;
		~UIImage() override = default;

	public:
		void Initialize() override;
		void DrawUI() const override;

	public:
		void SetTexture(const std::string& textureFilePath);
		const std::string& GetTexturePath() const;

		void SetRampTexture(const std::string& textureFilePath);
		void SetNoiseTexture(const std::string& textureFilePath);

		void SetAlphaBlend(bool enable);
		bool IsAlphaBlend() const;
		
		void SetColor(const Vector4& color);
		const Vector4& GetColor() const;

		void SetUV(const Vector4& uv) { m_uv = uv; m_dirty = true; }
		const Vector4& GetUV() const { return m_uv; }

		void SetMaskMode(MaskMode mode);
		MaskMode GetMaskMode() const { return m_maskMode; }

		void SetClipRect(const Vector4& r) { m_clipRect = r; m_dirty = true; }
		const Vector4& GetClipRect() const { return m_clipRect; }

		void SetMask0(const Vector4& v) { m_mask0 = v; m_dirty = true; }
		void SetMask1(const Vector4& v) { m_mask1 = v; m_dirty = true; }
		const Vector4& GetMask0() const { return m_mask0; }
		const Vector4& GetMask1() const { return m_mask1; }

		void SetOutline(bool enabe, float thickness, const Vector4& color);

	public:
		// Effect
		void ClearEffect();
		uint32_t GetEffectMode() { return m_effectMode; }

		void SetEffect0(const Vector4& v) { m_effect0 = v; m_dirty = true; }
		void SetEffect1(const Vector4& v) { m_effect1 = v; m_dirty = true; }
		void SetEffect2(const Vector4& v) { m_effect2 = v; m_dirty = true; }

		// 특정 컴포넌트(x, y, z, w)만 수정하고 싶을 때 편리한 래퍼
		void SetEffectParam0X(float x) { m_effect0.x = x; m_dirty = true; }

		void SetEffectScanLine(float density, float speed, float opacity);
		void SetEffectGlowPulse(float speed, float minIntensity, float maxIntensity);
		void SetEffectPixelate(float pixelSize);
		void SetEffectHoverTransition(bool isHover);
		void SetEffectAbyssalDecay();
		void SetEffectStaticNoise(float intensity);

	public:
		bool HasRenderType(RenderType type) const override;
		void Draw(RenderType type) const override;
		DirectX::BoundingBox GetBounds() const override;

	public:
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;

	private:
		void Refresh();
	};
}