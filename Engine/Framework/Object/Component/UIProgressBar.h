#pragma once
#include "Framework/Object/Component/UIElement.h"

namespace engine
{
	class UIImage;

	class UIProgressBar : public UIElement
	{
		REGISTER_COMPONENT(UIProgressBar, UIElement)

	public:
		enum class Direction
		{
			LeftToRight,
			RightToLeft,
			BottomToTop,
			TopToBottom,
		};

		enum class FillMode
		{
			PixelMask,    
			AnchorResize, 
		};

	public:
		UIProgressBar() = default;
		~UIProgressBar() override = default;

		void Initialize() override;
		void Update() override;
		void DrawUI() const override;

	public:
		float GetValue() const;
		void SetValue(float v);

		void SetDirection(Direction dir);
		void SetFillMode(FillMode mode);

		void SetSprites(const std::string& background, const std::string& fill);
		void SetColors(const Vector4& bg, const Vector4& fill);

	private:
		void CreateVisuals();
		void UpdateVisuals();
		bool RefreshVisuals();

	public:
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;

	private:
		UIImage* m_background = nullptr;
		UIImage* m_fill = nullptr;

		std::string m_bgSprite;
		std::string m_fillSprite;

		Vector4 m_bgColor = Vector4(1, 1, 1, 1);
		Vector4 m_fillColor = Vector4(1, 1, 1, 1);

		float m_value = 0.5f;

		Direction m_direction = Direction::LeftToRight;
		FillMode m_fillMode = FillMode::PixelMask;

		bool m_dirty = true;

		float m_barWidth = 100.0f;
		float m_barHeight = 100.0f;
	};
}