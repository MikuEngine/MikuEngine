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

	DirectX::XMMATRIX UIElement::BuildClipMatrix(const RectTransform& rt, const Canvas& c) const
	{
		auto& gd = GraphicsDevice::Get();
		auto vp = gd.GetViewport();

		const Vector2 scale = c.GetUIScale();
		const Vector2 offset = c.GetUIOffset();

		const UIRect& r = rt.GetWorldRect();

		// 1) UI 로컬(캔버스 기준) -> 화면 픽셀
		const float pxX = r.x * scale.x + offset.x;
		const float pxY = r.y * scale.y + offset.y;
		const float pxW = r.w * scale.x;
		const float pxH = r.h * scale.y;

		// 2) pivot 픽셀 위치
		const Vector2 pivot01 = rt.GetPivot();
		const float pxPivotX = pxX + pxW * pivot01.x;
		const float pxPivotY = pxY + pxH * pivot01.y;

		// 3) pivot - center 오프셋 (픽셀)
		const float dxPx = (pivot01.x - 0.5f) * pxW;
		const float dyPx = (0.5f - pivot01.y) * pxH; // y축 방향 보정(화면 down, NDC up)

		const float rot = rt.GetLocalRotationZRad();

		// 4) "픽셀 공간"에서 로컬 변환 구성
		//    (unit quad가 원점 중심(-0.5~0.5)이라고 가정: 기존 코드와 동일 가정)
		const DirectX::XMMATRIX M_localPx =
			DirectX::XMMatrixScaling(pxW, pxH, 1.0f) *
			DirectX::XMMatrixTranslation(-dxPx, -dyPx, 0.0f) *
			DirectX::XMMatrixRotationZ(rot) *
			DirectX::XMMatrixTranslation(pxPivotX, pxPivotY, 0.0f);

		// 5) 픽셀 -> 클립 변환(마지막에만 적용)
		//    (0,0) -> (-1,+1), (W,H) -> (+1,-1)
		const DirectX::XMMATRIX M_pxToClip =
			DirectX::XMMatrixScaling(2.0f / vp.Width, -2.0f / vp.Height, 1.0f) *
			DirectX::XMMatrixTranslation(-1.0f, 1.0f, 0.0f);

		return DirectX::XMMatrixTranspose(M_localPx * M_pxToClip);
	}

}