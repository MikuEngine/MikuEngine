#include "EnginePCH.h"
#include "Canvas.h"

#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/Component/RectTransform.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"

namespace engine
{
	namespace
	{
		void RecalcTree(Transform* tr, const UIRect& parentRect)
		{
			if (!tr) return;

			UIRect myRect = parentRect;

			if (tr->Is<RectTransform>())
			{
				auto rt = static_cast<RectTransform*>(tr);


				rt->Recalculate(parentRect);

				myRect = rt->GetWorldRect();
			}

			const auto& children = tr->GetChildren();
			for (auto* child : children)
				RecalcTree(child, myRect);
		}
	}

	const Vector2& Canvas::GetReferenceResolution() const
	{
		return m_referenceResolution;
	}

	void Canvas::SetReferenceResolution(const Vector2& resolution)
	{
		m_referenceResolution = resolution;
	}

	bool Canvas::IsRectTransformLockedInEditor() const
	{
		return m_lockRectTransformInEditor;
	}

	void Canvas::SetRectTransformLockedInEditor(bool lock)
	{
		m_lockRectTransformInEditor = lock;
	}

	int Canvas::GetSortingOrder() const
	{
		return m_sortingOrder;
	}

	void Canvas::SetSortingOrder(int order)
	{
		m_sortingOrder = order;
	}

	void Canvas::ReclaulateLayout(float viewportW, float viewportH)
	{
		const float refW = (m_referenceResolution.x > 1.0f) ? m_referenceResolution.x : viewportW;
		const float refH = (m_referenceResolution.y > 1.0f) ? m_referenceResolution.y : viewportH;

		const float sx = viewportW / refW;
		const float sy = viewportH / refH;

		const float scale = std::min(sx, sy);

		const float uiW = refW * scale;
		const float uiH = refH * scale;

		m_uiOffset.x = (viewportW - uiW) * 0.5f;
		m_uiOffset.y = (viewportH - uiH) * 0.5f;
		m_uiScale = { scale, scale };

		UIRect rootRect;
		rootRect.x = 0.0f;
		rootRect.y = 0.0f;
		rootRect.w = refW;
		rootRect.h = refH;

		GameObject* go = GetGameObject();
		if (!go) return;

		Transform* tr = go->GetTransform();
		if (!tr) return;

		RecalcTree(tr, rootRect);
	}

	void Canvas::OnGui()
	{
		ImGui::DragInt("Sorting Order", &m_sortingOrder, 1.0f);
	}

	void Canvas::Save(json& j) const
	{
		Component::Save(j);

		j["LockRectTransform"] = m_lockRectTransformInEditor;
		j["SortingOrder"] = m_sortingOrder;
	}

	void Canvas::Load(const json& j)
	{
		Component::Load(j);

		JsonGet(j, "LockRectTransform", m_lockRectTransformInEditor);
		JsonGet(j, "SortingOrder", m_sortingOrder);

		SystemManager::Get().GetRenderSystem().MarkScreenDirty();
	}
}