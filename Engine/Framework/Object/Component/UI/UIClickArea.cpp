#include "EnginePCH.h"
#include "UIClickArea.h"

namespace engine
{
	void UIClickArea::AddOnClick(ClickCallback&& cb)
	{
		m_onClick.push_back(std::move(cb));
	}

	void UIClickArea::AddOnHover(HoverCallback&& cb)
	{
		m_onHover.push_back(std::move(cb));
	}

	void UIClickArea::SetInteractable(bool v)
	{
		m_interactable = v;
	}

	bool UIClickArea::IsInteractable() const
	{
		return m_interactable;
	}

	void UIClickArea::OnMouseEnter(const Vector2&)
	{
		if (!m_interactable) return;
		if (!m_hovered) m_hovered = true;
		for (auto& f : m_onHover) f(true);
	}

	void UIClickArea::OnMouseExit(const Vector2&)
	{
		if (!m_interactable) return;
		if (m_hovered) m_hovered = false;
		for (auto& f : m_onHover) f(false);
	}

	void UIClickArea::OnMouseClick(const Vector2&, int mouseButton)
	{
		if (!m_interactable) return;
		for (auto& f : m_onClick) f(mouseButton);
	}
}