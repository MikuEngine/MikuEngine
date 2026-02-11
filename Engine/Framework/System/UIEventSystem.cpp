#include "EnginePCH.h"
#include "UIEventSystem.h"

#include "Core/System/Input.h"
#include "Core/Graphics/Device/GraphicsDevice.h"

#include "Framework/Object/GameObject/GameObject.h"

#include "Framework/Object/Component/UI/UIElement.h"
#include "Framework/Object/Component/UI/UIInteractable.h"
#include "Framework/Object/Component/RectTransform.h"
#include "Framework/Object/Component/UI/UIScrollView.h"
#include "Framework/Object/Component/Canvas.h"

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

		static bool IsDescendantOf(RectTransform* rt, RectTransform* ancestor)
		{
			if (!rt || !ancestor) return false;

			Transform* cur = rt;
			while (cur)
			{
				if (cur == ancestor) return true;
				cur = cur->GetParent();
			}
			return false;
		}

		static UIScrollView* FindScrollViewInParents(RectTransform* rt)
		{
			Transform* cur = rt;
			while (cur)
			{
				if (auto* go = cur->GetGameObject())
				{
					if (auto* sv = go->GetComponent<UIScrollView>())
						return sv;
				}
				cur = cur->GetParent();
			}
			return nullptr;
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

		if (m_hovered == e) m_hovered = nullptr;
		if (m_pressed == e)   m_pressed = nullptr;
		if (m_dragTarget == e)  m_dragTarget = nullptr;
	}

	void UIEventSystem::Update()
	{
		MouseState mouse = BuildMouseStateFromInput();

		auto IsInvalid = [](UIElement* e)
			{
				return (e == nullptr) || (!e->IsActive()) || (!e->m_raycastTarget) || (e->GetRectTransform() == nullptr);
			};

		if (IsInvalid(m_hovered))  m_hovered = nullptr;
		if (IsInvalid(m_pressed)) { m_pressed = nullptr; m_phase = PointerPhase::None; }

		RebuildCacheIfDirty();

		UIElement* newHover = HitTestTopmost(mouse.position);

		if (!mouse.leftHeld)
			HandleHover(newHover, mouse);

		HandleScroll(newHover, mouse);

		HandlePressDragRelease(newHover, mouse);

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

	void UIEventSystem::ResetPointerState(bool sendCancel)
	{
		if (m_hovered)
		{
			if (UIInteractable* it = AsInteractable(m_hovered))
				it->OnMouseExit(m_prevMousePos);
			m_hovered = nullptr;
		}

		if (m_pressed)
		{
			if (sendCancel)
			{
				if (UIInteractable* it = AsInteractable(m_pressed))
					it->OnMouseCancel(m_prevMousePos, 0);
			}
			m_pressed = nullptr;
		}

		m_phase = PointerPhase::None;
	}

	MouseState UIEventSystem::BuildMouseStateFromInput()
	{
		MouseState m{};

		m.position = Input::GetMousePosition();
		m.delta = m.position - m_prevMousePos;

		m.leftDown = Input::IsMousePressed(Input::Buttons::LEFT);
		m.leftHeld = Input::IsMouseHeld(Input::Buttons::LEFT);
		m.leftUp = Input::IsMouseReleased(Input::Buttons::LEFT);

		m.wheelDelta = Input::GetMouseWheelDelta();

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
				if (a == nullptr) return false; // a는 b보다 앞이 아님
				if (b == nullptr) return true;  // a는 b보다 앞임

				Canvas* ca = a->GetCanvasInParent();
				Canvas* cb = b->GetCanvasInParent();

				const int sa = ca ? ca->GetSortingOrder() : 0;
				const int sb = cb ? cb->GetSortingOrder() : 0;
				if (sa != sb) return sa < sb;

				if (a->GetOrderInLayer() != b->GetOrderInLayer())
					return a->GetOrderInLayer() < b->GetOrderInLayer();

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

			if (!e->HitTestPoint(mousePos)) continue;

			if (UIScrollView* sv = FindScrollViewInParents(rt))
			{
				RectTransform* vpRT = sv->GetViewportRT();
				if (vpRT && IsDescendantOf(rt, vpRT))
				{
					const UIRect vp = sv->GetViewPortWorldRect();

					Canvas* c = sv->GetCanvasInParent();
					if (!c) continue;

					const Vector2 scale = c->GetUIScale();
					const Vector2 offset = c->GetUIOffset();

					// mouse pixel → ref
					Vector2 p;
					p.x = (mousePos.x - offset.x) / scale.x;
					p.y = (mousePos.y - offset.y) / scale.y;

					if (!PointInRect(vp, p))
						continue;
				}
			}

			return e;
		}

		return nullptr;
	}

	void UIEventSystem::HandleHover(UIElement* target, const MouseState& mouse)
	{
		if (target == m_hovered) return;

		if (m_hovered)
		{
			if (UIInteractable* it = AsInteractable(m_hovered))
			{
				it->OnMouseExit(mouse.position);
			}
		}

		m_hovered = target;

		if (m_hovered)
		{
			if (UIInteractable* it = AsInteractable(m_hovered))
			{
				it->OnMouseEnter(mouse.position);
			}
		}
	}

	void UIEventSystem::HandlePressDragRelease(UIElement* target, const MouseState& mouse)
	{
		//bool rectIn = (target != nullptr) && target->HitTestPoint(mouse.position);
		bool pressedIn = (m_pressed != nullptr) && m_pressed->HitTestPoint(mouse.position);

		// MouseDown
		if (mouse.leftDown)
		{
			m_pressed = target;

			const bool downIn = (m_pressed != nullptr) && m_pressed->HitTestPoint(mouse.position);
			m_phase = downIn ? PointerPhase::PressedInRect : PointerPhase::PressedOutRect;

			m_isDragging = false;
			m_pressStartPos = mouse.position;

			if (m_pressed)
			{
				if (UIInteractable* it = AsInteractable(m_pressed))
				{
					if (it->IsDragEnabled())
						m_dragTarget = m_pressed;

					it->OnMouseDown(mouse.position, 0);
				}
			}

			return;
		}

		// MouseHeld
		if (mouse.leftHeld && m_pressed)
		{
			if (m_dragTarget)
			{
				const Vector2 d = mouse.position - m_pressStartPos;
				const float distSq = d.x * d.x + d.y * d.y;
				const float th = m_dragThresholdPixels;
				const float thSq = th * th;

				if (!m_isDragging && distSq >= thSq)
				{
					m_isDragging = true;
					if (UIInteractable* it = AsInteractable(m_dragTarget))
						it->OnBeginDrag(mouse.position, 0);
				}

				if (m_isDragging)
				{
					if (UIInteractable* it = AsInteractable(m_dragTarget))
						it->OnDrag(mouse.position, mouse.delta, 0);
					return;
				}
			}

			if (pressedIn)
			{
				if (m_phase == PointerPhase::PressedOutRect)
				{
					// 밖 -> 안 : Pressed 복귀
					m_phase = PointerPhase::PressedInRect;
					if (UIInteractable* it = AsInteractable(m_pressed))
						it->OnMouseDown(mouse.position, 0);
				}
			}
			else
			{
				if (m_phase == PointerPhase::PressedInRect)
				{
					// 안 -> 밖 : Cancel
					m_phase = PointerPhase::PressedOutRect;
					if (UIInteractable* it = AsInteractable(m_pressed))
						it->OnMouseCancel(mouse.position, 0);
				}
			}

			return;
		}

		// MouseUp
		if (mouse.leftUp)
		{
			if (m_pressed)
			{
				if (UIInteractable* it = AsInteractable(m_pressed))
					it->OnMouseUp(mouse.position, 0);

				if (m_isDragging && m_dragTarget)
				{
					if (UIInteractable* it = AsInteractable(m_dragTarget))
						it->OnEndDrag(mouse.position, 0);
				}
				else
				{
					if (m_phase == PointerPhase::PressedInRect)
					{
						if (UIInteractable* it = AsInteractable(m_pressed))
							it->OnMouseClick(mouse.position, 0);
					}
					else
					{
						if (UIInteractable* it = AsInteractable(m_pressed))
							it->OnMouseCancel(mouse.position, 0);
					}
				}
			}

			m_pressed = nullptr;
			m_dragTarget = nullptr;
			m_isDragging = false;
			m_phase = PointerPhase::None;
		}
	}

	void UIEventSystem::HandleScroll(UIElement* target, const MouseState& mouse)
	{
		if (!target) return;
		if (std::fabs(mouse.wheelDelta) < 1e-6f) return;

		UIElement* cur = target;

		while (cur)
		{
			if (UIInteractable* it = AsInteractable(cur))
			{
				if (it->IsScrollEnabled())
				{
					it->OnScroll(mouse.position, mouse.wheelDelta);
					return;
				}
			}

			// 2) 부모로 이동
			GameObject* go = cur->GetGameObject();
			Transform* tr = go ? go->GetTransform() : nullptr;
			Transform* parent = tr ? tr->GetParent() : nullptr;

			if (!parent) break;

			GameObject* pgo = parent->GetGameObject();
			cur = pgo ? pgo->GetComponent<UIElement>() : nullptr;
		}
	}
}