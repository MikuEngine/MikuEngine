#include "EnginePCH.h"
#include "RectTransform.h"

#include <imgui.h>
#include <algorithm>

#include "Common/Utility/JsonHelper.h"

namespace engine
{
    static Vector2 ClampVec2(const Vector2& v, float min, float max)
    {
        return { std::clamp(v.x, min, max), std::clamp(v.y, min, max) };
    }

    const Vector2& RectTransform::GetAnchoredPosition()
    {
        return m_anchoredPosition;
    }
    float RectTransform::GetWidth() const
    {
        return m_width;
    }
    float RectTransform::GetHeight() const
    {
        return m_height;
    }
    Vector2 RectTransform::GetSize() const
    {
        return {m_width, m_height};
    }
    const Vector2& RectTransform::GetPivot() const
    {
        return m_pivot;
    }
    const Vector2& RectTransform::GetAnchorMin() const
    {
        return m_anchorMin;
    }
    const Vector2& RectTransform::GetAnchorMax() const
    {
        return m_anchorMax;
    }
    const UIRect& RectTransform::GetWorldRect() const
    {
        return m_worldRect;
    }

    bool RectTransform::IsUIDirty() const
    {
        return m_uiDirty;
    }

    void RectTransform::SetAnchoredPosition(const Vector2& pos)
    {
        m_anchoredPosition = pos;
        MarkUIDirty();
    }

    void RectTransform::SetWidth(float w)
    {
        m_width = std::max(0.0f, w);
        MarkUIDirty();
    }

    void RectTransform::SetHeight(float h)
    {
        m_height = std::max(0.0f, h);
        MarkUIDirty();
    }

    void RectTransform::SetSize(float w, float h)
    {
        m_width = std::max(0.0f, w);
        m_height = std::max(0.0f, h);
        MarkUIDirty();
    }

    void RectTransform::SetPivot(const Vector2& pivot)
    {
        ClampVec2(pivot, 0.0f, 1.0f);
        MarkUIDirty();
    }

    void RectTransform::SetAnchorMin(const Vector2& anchorMin)
    {
        ClampVec2(anchorMin, 0.0f, 1.0f);
    }

    void RectTransform::SetAnchorMax(const Vector2& anchorMax)
    {
        ClampVec2(anchorMax, 0.0f, 1.0f);
    }

    void RectTransform::MarkUIDirty(bool v)
    {
        m_uiDirty = v;
    }

    void RectTransform::Recalculate(const UIRect& parentRect)
    {
        const Vector2 size{ m_width, m_height };

        const Vector2 parentTopLeft = parentRect.Pos();
        const Vector2 pivotPos = parentTopLeft + m_anchoredPosition;
        const Vector2 topLeft = pivotPos - Vector2(size.x * m_pivot.x, size.y * m_pivot.y);

        m_worldRect.x = topLeft.x;
        m_worldRect.y = topLeft.y;
        m_worldRect.w = size.x;
        m_worldRect.h = size.y;

        MarkUIDirty(false);
    }

    void RectTransform::OnGui()
    {
        if (ImGui::DragFloat2("AnchoredPosition", &m_anchoredPosition.x, 0.1f))
        {
            MarkUIDirty();
        }

        if (ImGui::DragFloat("Width", &m_width, 0.1f, 0.0f))
        {
            m_width = std::max(0.0f, m_width);
            MarkUIDirty();
        }

        if (ImGui::DragFloat("Height", &m_height, 0.1f, 0.0f))
        {
            m_height = std::max(0.0f, m_height);
            MarkUIDirty();
        }

        if (ImGui::DragFloat2("AnchorMin", &m_anchorMin.x, 0.01f, 0.0f, 1.0f))
        {
            m_anchorMin = ClampVec2(m_anchorMin, 0.0f, 1.0f);
            MarkUIDirty();
        }

        if (ImGui::DragFloat2("AnchorMax", &m_anchorMax.x, 0.01f, 0.0f, 1.0f))
        {
            m_anchorMax = ClampVec2(m_anchorMax, 0.0f, 1.0f);
            MarkUIDirty();
        }

        if (ImGui::DragFloat2("Pivot", &m_pivot.x, 0.01f, 0.0f, 1.0f))
        {
            m_pivot = ClampVec2(m_pivot, 0.0f, 1.0f);
            MarkUIDirty();
        }
    }

    void RectTransform::Save(json& j) const
    {
        Object::Save(j);

        j["AnchoredPosition"] = m_anchoredPosition;
        j["Width"] = m_width;
        j["Height"] = m_height;

        j["Pivot"] = m_pivot;
        j["AnchorMin"] = m_anchorMin;
        j["AnchorMax"] = m_anchorMax;
    }

    void RectTransform::Load(const json& j)
    {
        JsonGet(j, "AnchoredPosition", m_anchoredPosition);
        JsonGet(j, "Pivot", m_pivot);
        JsonGet(j, "AnchorMin", m_anchorMin);
        JsonGet(j, "AnchorMax", m_anchorMax);
        JsonGet(j, "Width", m_width);
        JsonGet(j, "Height", m_height);
    }

    std::string RectTransform::GetType() const
    {
        return "RectTransform";
    }
}