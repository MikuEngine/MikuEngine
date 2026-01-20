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

	private:
		bool m_interactable = true;
		bool m_wholeNumbers = false;

		float m_minValue = 0.0f;
		float m_maxValue = 1.0f;
		float m_value = 0.5f;

		bool   m_dragging = false;
		bool   m_draggingHandle = false;
		float  m_handleDragOffset = 0.0f;

		Direction m_direction = Direction::LeftToRight;

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

		Vector2 m_handlePivot = { 0.0f, 0.5f };

	public:
		void Initialize() override;
		void DrawUI() const override {}

	public:
		// Set
		void SetOnValueChanged(ValueChangedCallback cb);
		void SetRange(float minV, float maxV);
		void SetWholeNumbers(bool v);
		void SetValue(float v, bool notify = true);

		float GetValue() const;

		void SetSprites(const std::string& track,
						const std::string& fill,
						const std::string& handle);

	public:
		bool IsInteractable() const override;

		void OnMouseDown(const Vector2& mousePos, int mouseButton) override;
		void OnMouseUp(const Vector2& mousePos, int mouseButton) override;
		void OnBeginDrag(const Vector2& mousePos, int mouseButton) override;
		void OnDrag(const Vector2& mousePos, const Vector2& delta, int mouseButton) override;
		void OnEndDrag(const Vector2& mousePos, int mouseButton) override;

	public:
		// Component
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;
		std::string GetType() const override;

	private:
		void ApplyVisual();
		void CreateVisuals();
		void UpdateGraphics();

	private:
		// Util / Helper
		float Clamp(float v, float minV, float maxV);
		void SetValueFromMouse(const Vector2& mousePos, bool notify = true);
		bool IsMouseOnHandle(const Vector2& mousePos) const;
	};
}
