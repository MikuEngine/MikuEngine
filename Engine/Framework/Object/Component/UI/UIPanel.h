#pragma once

#include "Framework/Object/Component/UI/UIElement.h"

namespace engine
{
	class UIImage;
	class RectTransform;
	class GameObject;

	enum class PanelBackgroundMode
	{
		None,
		Color,
		Texture,
	};

	class UIPanel : public UIElement
	{
		REGISTER_COMPONENT(UIPanel)
	private:
		bool m_autoCreateBackground = true;
		Vector4 m_padding = Vector4(0, 0, 0, 0);

		PanelBackgroundMode m_bgMode = PanelBackgroundMode::Color;
		Vector4 m_color = Vector4(1, 1, 1, 1);
		Vector4 m_uv = Vector4(0, 0, 1, 1);
		std::string m_bgTexturePath;

		bool m_useAlphaBlend = true;

		MaskMode m_maskMode = MaskMode::None;
		Vector4  m_clipRect = Vector4(0, 0, 0, 0);

		UIImage* m_background = nullptr;

		bool m_dirty = true;

	public:
		UIPanel() = default;
		~UIPanel() override = default;

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

		void SetMaskMode(MaskMode mode) { m_maskMode = mode; m_dirty = true; }
		MaskMode GetMaskMode() const { return m_maskMode; }

		void SetClipRect(const Vector4& r) { m_clipRect = r; m_dirty = true; }
		const Vector4& GetClipRect() const { return m_clipRect; }

	public:
		bool HasRenderType(RenderType type) const override;
		void Draw(RenderType type) const override;
		DirectX::BoundingBox GetBounds() const override;

	public:
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;
		std::string GetType() const override;
	};
}