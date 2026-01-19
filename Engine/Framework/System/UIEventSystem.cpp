#include "EnginePCH.h"
#include "UIEventSystem.h"

#include "Core/System/Input.h"

#include "Framework/Object/Component/UIElement.h"
#include "Framework/Object/Component/RectTransform.h"
#include "Framework/Object/Component/Canvas.h"
#include "Framework/Object/Component/UIInteractable.h"

namespace engine
{
	namespace
	{
		bool PointInRect(const UIRect& r, const Vector2& p)
		{
			return (p.x >= r.x && p.y >= r.y && p.x <= r.x + r.w && p.y <= r.y + r.h);
		}

		UIInteractable* AsInteractable(UIElement* e)
		{
			return dynamic_cast<UIInteractable*>(e);
		}
	}

	void UIEventSystem::Register(UIElement* e)
	{
		System<UIElement>::Register(e);
		MarkDirty();
	}

	void UIEventSystem::Unregister(UIElement* e)
	{
		System<UIElement>::Unregister(e);
		MarkDirty();
	}

	void UIEventSystem::Update()
	{
		MouseState mouse = BuildMouseStateFromInput();

		RebuildCacheIfDirty();

		UIElement* newHover = HitTestTopmost(mouse.position);

		HandleHover(mouse, newHover);
		HandlePressDragRelease(mouse, newHover);

		m_prevMousePos = mouse.position;
	}

	void UIEventSystem::MarkDirty()
	{
		m_dirty = true;
	}

	void UIEventSystem::SetDebugThreshold(float pixels)
	{
		m_dragThresholdPixels = std::max(0.0f, pixels);
	}

	MouseState UIEventSystem::BuildMouseStateFromInput()
	{
		MouseState m{};

		m.position = Input::GetMousePosition();
		m.delta = m.position - m_prevMousePos;

		m.leftDown = Input::IsMousePressed(Input::Buttons::LEFT);
		m.leftHeld = Input::IsMouseHeld(Input::Buttons::LEFT);
		m.leftUp = Input::IsMouseReleased(Input::Buttons::LEFT);

		return m;
	}

	void UIEventSystem::RebuildCacheIfDirty()
	{
		if (!m_dirty) return;
		m_dirty = false;
		
		m_sortedTopBottom = m_components;

		std::stable_sort(m_sortedTopBottom.begin(), m_sortedTopBottom.end(),
			[](UIElement* a, UIElement* b)
			{
				Canvas* ca = a ? a->GetCanvasInParent() : nullptr;
				Canvas* cb = b ? b->GetCanvasInParent() : nullptr;

				const int sa = ca ? ca->GetSortingOrder() : 0;
				const int sb = cb ? cb->GetSortingOrder() : 0;
				if (sa != sb) return sa < sb;

				if (a->m_orderInLayer != b->m_orderInLayer)
					return a->m_orderInLayer < b->m_orderInLayer;

				return false;
			});
	}

	UIElement* UIEventSystem::HitTestTopmost(const Vector2& mousePos) const
	{
		for (int i = (int)m_sortedTopBottom.size() - 1; i >= 0; --i)
		{
			UIElement* e = m_sortedTopBottom[i];
			if (!e) continue;
			if (!e->IsActive()) continue;
			if (!e->m_raycastTarget) continue;

			RectTransform* rt = e->GetRectTransform();
			if (!rt) continue;

			const UIRect& r = rt->GetWorldRect();
			if (!PointInRect(r, mousePos)) continue;

			UIInteractable* it = AsInteractable(e);
			if (!it) continue;
			if (!it->IsInteractable()) continue;

			return e;
		}

		return nullptr;
	}

	void UIEventSystem::HandleHover(const MouseState& mouse, UIElement* newHover)
	{
		if (newHover == m_hovered) return;

		if (m_hovered)
		{
			if (UIInteractable* it = AsInteractable(m_hovered))
			{
				it->OnMouseExit(mouse.position);
			}
		}

		m_hovered = newHover;

		if (m_hovered)
		{
			if (UIInteractable* it = AsInteractable(m_hovered))
			{
				it->OnMouseEnter(mouse.position);
			}
		}
	}

	void UIEventSystem::HandlePressDragRelease(const MouseState& mouse, UIElement* newHover)
	{
		// Click
		if (mouse.leftDown)
		{
			m_pressed = newHover;
			m_pressStartPos = mouse.position;
			m_isDragging = false;
			m_dragTarget = nullptr;

			if (m_pressed)
			{
				if (UIInteractable* it = AsInteractable(m_pressed))
				{
					it->OnMouseDown(mouse.position, 0);
				}
			}
		}

		// Hold
		if (mouse.leftHeld)
		{
			if (m_pressed)
			{
				if (UIInteractable* it = AsInteractable(m_pressed))
				{
					it->OnMouseOver(mouse.position);
				}
			}

			if (!m_isDragging && m_pressed && !m_dragTarget)
			{
				float dist = (mouse.position - m_pressStartPos).Length();

				if (dist >= m_dragThresholdPixels)
				{
					m_isDragging = true;
					m_dragTarget = m_pressed;

					if (UIInteractable* it = AsInteractable(m_dragTarget))
					{
						it->OnBeginDrag(mouse.position, 0);
					}
				}
			}

			if (m_isDragging && m_dragTarget)
			{
				if (UIInteractable* it = AsInteractable(m_dragTarget))
					it->OnDrag(mouse.position, mouse.delta, 0);
			}
		}
		
		// Release
		if (mouse.leftUp)
		{
			if (m_pressed)
			{
				if (UIInteractable* it = AsInteractable(m_pressed))
				{
					it->OnMouseUp(mouse.position, 0);
				}
			}

			if (m_isDragging)
			{
				if (m_dragTarget)
				{
					if (UIInteractable* it = AsInteractable(m_dragTarget))
					{
						it->OnEndDrag(mouse.position, 0);
					}
				}
			}
			else
			{
				if (m_pressed && (m_pressed == newHover))
				{
					if (UIInteractable* it = AsInteractable(m_pressed))
					{
						it->OnMouseClick(mouse.position, 0);
					}
				}
			}

			m_pressed = nullptr;
			m_dragTarget = nullptr;
			m_isDragging = false;
		}
	}
}