#include "EnginePCH.h"
#include "RectTransform.h"

#include <imgui.h>
#include <algorithm>

#include "Common/Utility/JsonHelper.h"

#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Transform.h"

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

    RectTransform* RectTransform::FindPrentRectTransform() const
    {
        Transform* p = GetParent();

        while (p)
        {
            GameObject* go = p->GetGameObject();

            if (go)
            {
                if (auto* ptr = go->GetComponent<RectTransform>())
                    return ptr;
            }

            p = p->GetParent();
        }

        return nullptr;
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
        m_pivot = ClampVec2(pivot, 0.0f, 1.0f);
        MarkUIDirty();
    }

    void RectTransform::SetAnchorMin(const Vector2& anchorMin)
    {
        m_anchorMin = ClampVec2(anchorMin, 0.0f, 1.0f);
        MarkUIDirty();
    }

    void RectTransform::SetAnchorMax(const Vector2& anchorMax)
    {
        m_anchorMax = ClampVec2(anchorMax, 0.0f, 1.0f);
        MarkUIDirty();
    }

    void RectTransform::MarkUIDirty(bool v)
    {
        m_uiDirty = v;
    }

    void RectTransform::Recalculate(const UIRect& parentRect)
    {
        Vector2 parentPos = parentRect.Pos();
        Vector2 parentSize = parentRect.Size();

        Vector2 aMin = { parentPos.x + parentSize.x * m_anchorMin.x,
                         parentPos.y + parentSize.y * m_anchorMin.y };

        Vector2 aMax = { parentPos.x + parentSize.x * m_anchorMax.x, 
                         parentPos.y + parentSize.y * m_anchorMax.y };
        
        Vector2 anchorSize = { aMax.x - aMin.x , aMax.y - aMin.y };
        Vector2 anchorCenter = { (aMin.x + aMax.x) * 0.5f, (aMin.y + aMax.y) * 0.5f };

        Vector2 size = { std::max(0.0f, anchorSize.x + m_width), std::max(0.0f, anchorSize.y + m_height)};
        
        const Vector2 pivotPos = anchorCenter + m_anchoredPosition;
        const Vector2 topLeft = { pivotPos.x - size.x * m_pivot.x,
                                  pivotPos.y - size.y * m_pivot.y };

        m_worldRect.x = topLeft.x;
        m_worldRect.y = topLeft.y;
        m_worldRect.w = size.x;
        m_worldRect.h = size.y;

        MarkUIDirty(false);
    }

    UIRect& RectTransform::GetWorldRectResolved(const UIRect& rootRect)
    {
        UIRect parentRect = rootRect;

        if (RectTransform* prt = FindPrentRectTransform())
        {
            parentRect = prt->GetWorldRectResolved(rootRect);
        }

        if (IsUIDirty())
        {
            Recalculate(parentRect);
        }

        return m_worldRect;
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