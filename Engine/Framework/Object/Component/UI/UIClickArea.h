#pragma once
#include "Framework/Object/Component/UI/UIElement.h"
#include "Framework/Object/Component/UI/UIInteractable.h"

namespace engine
{
	class UIClickArea : public UIElement, public UIInteractable
	{
		REGISTER_COMPONENT(UIClickArea, UIElement)

	public:
		using ClickCallback = std::function<void(int)>;
		using HoverCallback = std::function<void(bool)>;

		using BeginDragCallback = std::function<void(const Vector2&, int)>;
		using DragCallback = std::function<void(const Vector2&, const Vector2&, int)>; // (pos, delta, button)
		using EndDragCallback = std::function<void(const Vector2&, int)>;

	public:
		void Initialize() override;
		void DrawUI() const override {}

		void AddOnClick(ClickCallback&& cb);
		void AddOnClick(void* owner, ClickCallback&& cb);
		void AddOnHover(HoverCallback&& cb);

		void UnbindOnClick(void* owner);
		std::vector<std::pair<void*, size_t>> m_ownerTracker;

		void AddOnBeginDrag(BeginDragCallback&& cb);
		void AddOnDrag(DragCallback&& cb);
		void AddOnEndDrag(EndDragCallback&& cb);

		void SetInteractable(bool v);
		bool IsInteractable() const override;

		// Input
		void OnMouseEnter(const Vector2&) override;
		void OnMouseExit(const Vector2&) override;
		void OnMouseClick(const Vector2&, int mouseButton) override;
		void OnScroll(const Vector2& mousePos, float wheelDelta) override;
		bool IsScrollEnabled() const override { return true; }

		// etc...
		void OnMouseDown(const Vector2&, int) override {}
		void OnMouseUp(const Vector2&, int) override {}
		void OnMouseOver(const Vector2&) override {}
		void OnMouseCancel(const Vector2&, int) override {}

		void OnBeginDrag(const Vector2& mousePos, int mouseButton) override;
		void OnDrag(const Vector2& mousePos, const Vector2& delta, int mouseButton) override;
		void OnEndDrag(const Vector2& mousePos, int mouseButton) override;
		bool IsDragEnabled() const override { return true; }

		void Save(json& j) const override;
		void Load(const json& j) override;

	private:
		bool m_interactable = true;
		bool m_hovered = false;
		std::vector<ClickCallback> m_onClick;
		std::vector<HoverCallback> m_onHover;

		std::vector<BeginDragCallback> m_onBeginDrag;
		std::vector<DragCallback>      m_onDrag;
		std::vector<EndDragCallback>   m_onEndDrag;
	};
}