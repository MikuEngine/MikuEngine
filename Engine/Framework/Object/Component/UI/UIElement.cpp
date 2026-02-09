#include "EnginePCH.h"
#include "UIElement.h"

#include "Core/Graphics/Device/GraphicsDevice.h"

#include "Framework/System/RenderSystem.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/UIEventSystem.h"

#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/Component/RectTransform.h"
#include "Framework/Object/Component/Canvas.h"
#include "Framework/Object/Component/Component.h"

namespace engine
{
	namespace
	{
		static bool PointInRect(const Vector2& p, const UIRect& r)
		{
			return (p.x >= r.x && p.x <= r.x + r.w &&
				p.y >= r.y && p.y <= r.y + r.h);
		}
	}

	UIElement::~UIElement()
	{
		SystemManager::Get().GetUIEventSystem().Unregister(this);
		SystemManager::Get().GetUIEventSystem().MarkDirty();
	}

	void UIElement::Initialize()
	{
		SystemManager::Get().GetUIEventSystem().Register(this);
		SystemManager::Get().GetUIEventSystem().MarkDirty();
	}

	void UIElement::OnEnable()
	{
		SystemManager::Get().GetUIEventSystem().Register(this);
		SystemManager::Get().GetUIEventSystem().MarkDirty();
	}

	void UIElement::OnDisable()
	{
		SystemManager::Get().GetUIEventSystem().Unregister(this);
		SystemManager::Get().GetUIEventSystem().MarkDirty();
	}

	bool UIElement::HitTestPoint(const Vector2& screenPos) const
	{
		RectTransform* rt = GetRectTransform();
		if (!rt) return false;

		Canvas* c = GetCanvasInParent();
		if (!c) return false;

		const Vector2 scale = c->GetUIScale();
		const Vector2 offset = c->GetUIOffset();

		Vector2 p;
		p.x = (screenPos.x - offset.x) / scale.x;
		p.y = (screenPos.y - offset.y) / scale.y;

		auto& gd = GraphicsDevice::Get();
		auto vp = gd.GetViewport();
		UIRect rootRect{ 0,0,vp.Width,vp.Height };

		return
			p.x >= rt->GetWorldRect().x &&
			p.y >= rt->GetWorldRect().y &&
			p.x <= rt->GetWorldRect().x + rt->GetWorldRect().w &&
			p.y <= rt->GetWorldRect().y + rt->GetWorldRect().h;
	}

	bool UIElement::HasRenderType(RenderType type) const
	{
		return type == RenderType::Screen;
	}

	void UIElement::Draw(RenderType type) const
	{
		if (type != RenderType::Screen)
		{
			return;
		}

		if (!CanDraw())
		{
			return;
		}

		DrawUI();
	}

	DirectX::BoundingBox UIElement::GetBounds() const
	{
		DirectX::BoundingBox box{};

		RectTransform* rt = GetRectTransform();
		if (rt == nullptr)
		{
			box.Center = { 0.f, 0.f, 0.f };
			box.Extents = { 0.f, 0.f, 0.f };
			return box;
		}

		const UIRect& r = rt->GetWorldRect();

		const float cx = r.x + r.w * 0.5f;
		const float cy = r.y + r.h * 0.5f;

		box.Center = { cx, cy, 0.0f };
		box.Extents = { r.w * 0.5f, r.h * 0.5f, 0.01f };

		return box;
	}

	void UIElement::SetLayer(int layer)
	{
		if (m_layer == layer) return;
		m_layer = layer;
		SystemManager::Get().GetRenderSystem().MarkScreenDirty();
	}

	void UIElement::SetOrderInLayer(int order)
	{
		if (m_orderInLayer == order) return;
		m_orderInLayer = order;
		SystemManager::Get().GetRenderSystem().MarkScreenDirty();
	}

	int UIElement::GetLayer()
	{
		return m_layer;
	}

	int UIElement::GetOrderInLayer()
	{
		return m_orderInLayer;
	}

	RectTransform* UIElement::GetRectTransform() const
	{
		GameObject* go = GetGameObject();

		if (!go)
		{
			return nullptr;
		}

		if (auto* rt = go->GetTransform()->As<RectTransform>())
		{
			return rt;
		}

		return go->GetComponent<RectTransform>();
	}

	Canvas* UIElement::GetCanvasInParent() const
	{
		GameObject* go = GetGameObject();
		if (!go)
		{
			return nullptr;
		}

		while (go)
		{
			if (auto* canvas = go->GetComponent<Canvas>())
			{
				return canvas;
			}

			Transform* tr = go->GetTransform();
			if (!tr)
			{
				break;
			}

			Transform* parent = tr->GetParent();
			go = parent ? parent->GetGameObject() : nullptr;
		}

		return nullptr;
	}

	void UIElement::OnGui()
	{
		int layer = m_layer;
		if (ImGui::DragInt("Layer", &layer, 1.0f))
		{
			SetLayer(layer);
		}

		int order = m_orderInLayer;
		if (ImGui::DragInt("Order In Layer", &order, 1.0f))
		{
			SetOrderInLayer(order);
		}

		ImGui::Checkbox("Raycast Target", &m_raycastTarget);
	}

	void UIElement::Save(json& j) const
	{
		Component::Save(j);

		j["Layer"] = m_layer;
		j["OrderInLayer"] = m_orderInLayer;
		j["RaycastTarget"] = m_raycastTarget;
	}

	void UIElement::Load(const json& j)
	{
		Component::Load(j);

		JsonGet(j, "Layer", m_layer);
		JsonGet(j, "OrderInLayer", m_orderInLayer);
		JsonGet(j, "RaycastTarget", m_raycastTarget);
	}

	bool UIElement::CanDraw() const
	{
		if (!GetCanvasInParent()) return false;

		GameObject* go = GetGameObject();
		if (!go)
		{
			return false;
		}

		if (!IsActive() || !go->IsActive())
		{
			return false;
		}

		if (!GetRectTransform())
		{
			return false;
		}

		return true;
	}

	float UIElement::Clamp01(float v)
	{
		if (v < 0.0f) return 0.0f;
		if (v > 1.0f) return 1.0f;
		return v;
	}
}