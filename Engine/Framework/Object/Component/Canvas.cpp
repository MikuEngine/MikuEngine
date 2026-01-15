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

		j["ReferenceResolution"] = { m_referenceResolution.x, m_referenceResolution.y };
		j["SortingOrder"] = m_sortingOrder;
	}
	void Canvas::Load(const json& j)
	{
		Component::Load(j);

		if (j.contains("ReferenceResolution") &&
			j["ReferenceResolution"].is_array() &&
			j["ReferenceResolution"].size() == 2)
		{
			m_referenceResolution.x = j["ReferenceResolution"][0].get<float>();
			m_referenceResolution.y = j["ReferenceResolution"][1].get<float>();
		}

		JsonGet(j, "SortingOrder", m_sortingOrder);
	}
	std::string Canvas::GetType() const
	{
		return "Canvas";
	}
}