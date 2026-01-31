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
		std::string m_textureFilePath;
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

	public:
		UIImage() = default;
		~UIImage() override = default;

	public:
		void Initialize() override;
		void DrawUI() const override;

	public:
		void SetTexture(const std::string& textureFilePath);
		const std::string& GetTexturePath() const;

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