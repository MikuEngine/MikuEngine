#include "EnginePCH.h"
#include "UIElement.h"

#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/Component/RectTransform.h"
#include "Framework/Object/Component/Canvas.h"

namespace engine
{
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

	RectTransform* UIElement::GetRectTransform() const
	{
		GameObject* go = GetGameObject();

		if (!go)
		{
			return nullptr;
		}

		if (auto* rt = dynamic_cast<RectTransform*>(go->GetTransform()))
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

	bool UIElement::CanDraw() const
	{
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

	void UIElement::OnGui()
	{
		ImGui::Text("UIElement (Screen Renderer)");
		ImGui::Text("Canvas: %s", GetCanvasInParent() ? "Yes" : "No");
		ImGui::Text("RectTransform: %s", GetRectTransform() ? "Yes" : "No");
	}

	std::string UIElement::GetType() const
	{
		return "UIElement";
	}
}