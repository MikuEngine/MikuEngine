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

		void AddOnClick(ClickCallback&& cb);
		void AddOnHover(HoverCallback&& cb);

		void SetInteractable(bool v);
		bool IsInteractable() const;

		// Input
		void OnMouseEnter(const Vector2&) override;
		void OnMouseExit(const Vector2&) override;
		void OnMouseClick(const Vector2&, int mouseButton) override;

		// etc...
		void OnMouseDown(const Vector2&, int) override {}
		void OnMouseUp(const Vector2&, int) override {}
		void OnMouseOver(const Vector2&) override {}
		void OnMouseCancel(const Vector2&, int) override {}

		void DrawUI() const override {}

		void Save(json& j) const override
		{
			UIElement::Save(j);
			j["Interactable"] = m_interactable;
		}

		void Load(const json& j) override
		{
			UIElement::Load(j);
			JsonGet(j, "Interactable", m_interactable);
		}

	private:
		bool m_interactable = true;
		bool m_hovered = false;
		std::vector<ClickCallback> m_onClick;
		std::vector<HoverCallback> m_onHover;
	};
}