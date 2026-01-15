#include "EnginePCH.h"
#include "Canvas.h"

namespace engine
{
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

	void Canvas::OnGui()
	{
		ImGui::DragFloat2("Reference Resolution", &m_referenceResolution.x, 1.0f, 1.0f, 16384.0f);
		ImGui::DragInt("Sorting Order", &m_sortingOrder, 1.0f);
	}

	void Canvas::Save(json& j) const
	{
		Component::Save(j);

		j["LockRectTransform"] = m_lockRectTransformInEditor;
		j["ReferenceResolution"] = m_referenceResolution;
		j["SortingOrder"] = m_sortingOrder;
	}

	void Canvas::Load(const json& j)
	{
		Component::Load(j);

		JsonGet(j, "LockRectTransform", m_lockRectTransformInEditor);
		JsonGet(j, "ReferenceResolution", m_referenceResolution);
		JsonGet(j, "SortingOrder", m_sortingOrder);
	}

	std::string Canvas::GetType() const
	{
		return "Canvas";
	}
}