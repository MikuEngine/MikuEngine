#pragma once

#include "Framework/Object/Component/UI/UIElement.h"
#include "Framework/System/System.h"

namespace engine
{
	class UIInteractable;
	class Canvas;
	class UIRect;

	enum class PointerPhase
	{
		None,              // 클릭 안 함
		PressedInRect,
		PressedOutRect,
	};

	struct MouseState
	{
		Vector2 position{ 0.0f, 0.0f };
		Vector2 delta{ 0.0f, 0.0f };

		bool leftDown = false;
		bool leftHeld = false;
		bool leftUp = false;
	};

	class UIEventSystem : public System<UIElement>
	{
	public:
		UIEventSystem() = default;
		~UIEventSystem() = default;

	public:
		void Register(UIElement* e) override;
		void Unregister(UIElement* e) override;

		void Update();
		
		void MarkDirty();
		void SetDebugThreshold(float pixels);

	private:
		MouseState BuildMouseStateFromInput();

		void RebuildCacheIfDirty();
		UIElement* HitTestTopmost(const Vector2& mousePos) const;

		void HandleHover(UIElement* target, const MouseState& mouse);
		void HandlePressDragRelease(UIElement* target, const MouseState& mouse);

	private:
		bool m_dirty = true;
		std::vector<UIElement*> m_sortedTopBottom;

		PointerPhase m_phase = PointerPhase::None;

		UIElement* m_hovered = nullptr;
		UIElement* m_pressed = nullptr;
		UIElement* m_dragTarget = nullptr;

		bool m_isDragging = false;
		Vector2 m_pressStartPos{ 0.0f, 0.0f };

		float m_dragThresholdPixels = 3.0f;
		Vector2 m_prevMousePos{ 0.0f, 0.0f };

	private:
		friend class System<UIElement>;
	};
}