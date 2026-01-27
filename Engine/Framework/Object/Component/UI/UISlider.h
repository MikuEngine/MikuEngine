#pragma once

#include "Framework/Object/Component/UI/UIElement.h"
#include "Framework/Object/Component/UI/UIInteractable.h"

namespace engine
{
	class UIImage;
	class RectTransform;

	class UISlider : public UIElement, public UIInteractable
	{
		REGISTER_COMPONENT(UISlider, UIElement)

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

		bool HitTestPoint(const Vector2& p) const override;

	public:
		float GetValue() const;
		void SetValue(float v, bool notify = true);

		void SetDirection(Direction dir);
		void SetFillMode(FillMode mode);

		void SetSprites(const std::string& track,
						const std::string& fill,
						const std::string& handle);

		void SetOnValueChanged(ValueChangedCallback cb);

		void SetUseFill(bool use) { m_useFill = use; }
		bool GetUseFill() const { return m_useFill; }

		void ForceUpdateVisuals() { RefreshVisuals(); UpdateVisuals(); m_dirty = false; }

	private:
		void CreateVisuals();
		bool RefreshVisuals();
		void UpdateVisuals();

	public:
		bool IsInteractable() const override { return true; }
		bool IsDragEnabled() const override { return true; }

		void OnMouseDown(const Vector2& mousePos, int mouseButton) override;
		void OnMouseUp(const Vector2& mousePos, int mouseButton) override;

		void OnBeginDrag(const Vector2& mousePos, int mouseButton) override;
		void OnDrag(const Vector2& mousePos, const Vector2& delta, int mouseButton) override;
		void OnEndDrag(const Vector2& mousePos, int mouseButton) override;

		void OnMouseCancel(const Vector2& mousePos, int mouseButton) override;

	public:
		// Component
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;

	private:
		void SetValueFromMouse(const Vector2& mousePos);
		bool IsMouseOnHandle(const Vector2& mousePos) const;

	private:
		ValueChangedCallback m_onValueChanged;

		UIImage* m_background = nullptr;
		UIImage* m_fill = nullptr;
		UIImage* m_handle = nullptr;

		Direction m_direction = Direction::LeftToRight;
		FillMode m_fillMode = FillMode::PixelMask;

		Vector2 m_mousePos = { 0.0f, 0.0f };

		float m_value = 0.5f;

		bool m_dragging = false;
		bool m_dragFromHandle = false;
		bool m_dirty = true;

		bool m_useFill = true;

		std::string m_bgSprite;
		std::string m_fillSprite;
		std::string m_handleSprite;

		Vector4 m_bgColor = Vector4(1, 1, 1, 1);
		Vector4 m_fillColor = Vector4(1, 1, 1, 1);
		Vector4 m_handleColor = Vector4(1, 1, 1, 1);
	};
}
