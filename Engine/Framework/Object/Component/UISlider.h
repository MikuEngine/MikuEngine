#pragma once

#include "Framework/Object/Component/UIElement.h"
#include "Framework/Object/Component/UIInteractable.h"

namespace engine
{
	class UIImage;

	class UISlider : public UIElement, public UIInteractable
	{
		REGISTER_COMPONENT(UISlider)
	public:
		using ValueChangedCallback = std::function<void(float)>;

		enum class Direction
		{
			LeftToRight,
			RightToLeft,
			BottomToTop,
			TopToBottom,
		};

		enum class FillMode
		{
			PixelMask,		// Mask
			AnchorResize,	// Ratio
		};

	public:
		UISlider() = default;
		~UISlider() override = default;

		void Initialize() override;
		void Update() override;
		void DrawUI() const override {}

	public:
		float GetValue() const;
		void SetValue(float v, bool notify = true);

		void SetRange(float minV, float maxV);

		void SetSprites(const std::string& track,
						const std::string& fill,
						const std::string& handle);

		void SetDirection(Direction dir);
		void SetFillMode(FillMode mode);

		void SetOnValueChanged(ValueChangedCallback cb);

	private:
		void CreateVisuals();
		void UpdateVisuals();

		void OnBeginDrag(const Vector2& mousePos, int mouseButton) override;
		void OnDrag(const Vector2& mousePos, const Vector2& delta, int mouseButton) override;
		void OnEndDrag(const Vector2& mousePos, int mouseButton) override;

		void OnMouseDown(const Vector2& mousePos, int mouseButton) override;
		void OnMouseUp(const Vector2& mousePos, int mouseButton) override;

	public:
		bool IsInteractable() const override;

	public:
		// Component
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;
		std::string GetType() const override;

	private:
		// Util / Helper
		float Clamp(float v, float minV, float maxV);
		void SetValueFromMouse(const Vector2& mousePos, bool notify = true);
		bool IsMouseOnHandle(const Vector2& mousePos) const;

	private:
		float m_minValue = 0.0f;
		float m_maxValue = 1.0f;
		float m_value = 0.5f;

		float m_handleDragOffset = 0.0f;
		Vector2 m_handlePivot = {0.0f, 0.0f};

		bool m_interactable = false;
		bool m_dragging = false;
		bool m_draggingHandle = false;

		Direction m_direction = Direction::LeftToRight;
		FillMode m_fillMode = FillMode::PixelMask;

		ValueChangedCallback m_onValueChanged;

		UIImage* m_track = nullptr;
		UIImage* m_fill = nullptr;
		UIImage* m_handle = nullptr;

		std::string m_trackSprite;
		std::string m_fillSprite;
		std::string m_handleSprite;

		Vector4 m_trackColor = Vector4(1, 1, 1, 1);
		Vector4 m_fillColor = Vector4(1, 1, 1, 1);
		Vector4 m_handleColor = Vector4(1, 1, 1, 1);
	};
}
