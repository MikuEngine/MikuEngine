#pragma once

namespace engine
{
	// UI의 입력상태를 제어합니다.
	class UIInteractable
	{
	public:
		virtual ~UIInteractable() = default;

		virtual bool IsInteraactable() const { return true; }

		virtual void OnMouseEnter(const Vector2& mousePos) {}
		virtual void OnMouseExit(const Vector2& mousePos) {}
		
		// Hit
		virtual void OnMouseOver(const Vector2& mousePos) {}
		virtual void OnMouseDown(const Vector2& mousePos, int mouseButton) {}
		virtual void OnMouseUp(const Vector2& mousePos, int mouseButton) {}
		virtual void OnMouseClick(const Vector2& mousePos, int mouseButton) {}

		// Drag
		virtual void OnBeginDrag(const Vector2& mousePos, int mouseButton) {}
		virtual void OnDrag(const Vector2& mousePos, const Vector2& delta, int mouseButton) {}
		virtual void OnEndDrag(const Vector2& mousePos, int mouseButton) {}
	}; 
}