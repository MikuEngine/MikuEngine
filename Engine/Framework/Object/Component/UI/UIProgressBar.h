#pragma once
#include "Framework/Object/Component/UI/UIElement.h"

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

		enum class Shape
		{
			Linear,
			Radial,   // 원형/링 포함
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

		Shape GetShape() const { return m_shape; }
		void SetShape(Shape s) { if (m_shape == s) return; m_shape = s; m_dirty = true; }

		// Radial 옵션
		void SetStartAngleRad(float rad) { m_startAngleRad = rad; m_dirty = true; }
		void SetClockwise(bool cw) { m_clockwise = cw; m_dirty = true; }

		// 0.0 = 원, (0~1) = 링 (값이 클수록 안쪽 구멍이 커짐)
		void SetInnerRadius01(float t) { m_innerRadius01 = Clamp01(t); m_dirty = true; }

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
		Shape m_shape = Shape::Linear;

		bool m_dirty = true;

		float m_barWidth = 100.0f;
		float m_barHeight = 100.0f;

	private:
		// 라디얼 전용
		float m_startAngleRad = -DirectX::XM_PIDIV2;  // 12시 시작 기본
		bool  m_clockwise = true;
		float m_innerRadius01 = 0.6f;
	};
}