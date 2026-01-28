#include "EnginePCH.h"
#include "UIClickArea.h"

#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/Component/UI/UIScrollView.h"

namespace engine
{
	namespace
	{
		static engine::UIScrollView* FindScrollViewInParents(engine::GameObject* from)
		{
			if (!from) return nullptr;
			engine::Transform* t = from->GetTransform();
			while (t)
			{
				engine::GameObject* go = t->GetGameObject();
				if (go)
				{
					if (auto* sv = go->GetComponent<engine::UIScrollView>())
						return sv;
				}
				t = t->GetParent();
			}
			return nullptr;
		}
	}

	void UIClickArea::Initialize()
	{
		UIElement::Initialize();
	}

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

	void UIClickArea::OnScroll(const Vector2& mousePos, float wheelDelta)
	{
		if (auto* sv = FindScrollViewInParents(GetGameObject()))
		{
			if (sv->HitTestPoint(mousePos))
				sv->OnScroll(mousePos, wheelDelta);
		}
	}

	void UIClickArea::Save(json& j) const
	{
		UIElement::Save(j);
		j["Interactable"] = m_interactable;
	}

	void UIClickArea::Load(const json& j)
	{
		UIElement::Load(j);
		JsonGet(j, "Interactable", m_interactable);
	}
}