#include "GamePCH.h"
#include "UIFollowTarget.h"

#include <Core/Graphics/Device/GraphicsDevice.h>

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/UI/UIImage.h>
#include <Framework/Object/Component/RectTransform.h>

#include <Framework/Object/Component/Camera.h>

namespace game
{
    void UIFollowTarget::Awake()
    {
        auto* go = GetGameObject();
        if (!go) return;

        m_rt = go->GetComponent<engine::RectTransform>();
        m_img = go->GetComponent<engine::UIImage>();
        
        PrepareAnchorOnce();
        RebindTarget();

        if (auto* camGO = engine::GameObject::Find("MainCamera"))
            m_camera = camGO->GetComponent<engine::Camera>();
        
        m_cameraCached = (m_camera != nullptr);

        if (m_rt)
            m_parentRT = m_rt->FindPrentRectTransform();

        m_cachedVisible = true;
    }

    void UIFollowTarget::Update()
    {
        if (!m_rt || !m_cameraCached || !m_target)
        {
            SetVisible(false);
            return;
        }

        engine::Matrix view = m_camera->GetView();
        engine::Matrix proj = m_camera->GetProjection();

        const engine::Vector3 worldPos =
            m_target->GetTransform()->GetWorldPosition() + m_offset;

        engine::Vector4 pos(worldPos.x, worldPos.y, worldPos.z, 1.0f);
    
        engine::Vector4 v = engine::Vector4::Transform(pos, view);
        engine::Vector4 clip = engine::Vector4::Transform(v, proj);

        if (clip.w <= 0.0001f)
        {
            SetVisible(false);
            return;
        }

        // NDC
        const float invW = 1.0f / clip.w;
        const float ndcX = clip.x * invW;
        const float ndcY = clip.y * invW;

        const auto& vp = engine::GraphicsDevice::Get().GetViewport();

        engine::Vector2 screen;
        screen.x = (ndcX * 0.5f + 0.5f) * vp.Width;
        screen.y = (-ndcY * 0.5f + 0.5f) * vp.Height;

        engine::UIRect rootRect{ 0.f, 0.f, vp.Width, vp.Height };

        engine::UIRect parentRect = rootRect;
        if (engine::RectTransform* prt = m_rt->FindPrentRectTransform())
        {
            parentRect = prt->GetWorldRectResolved(rootRect);
        }

        const engine::Vector2 finalPos(screen.x - parentRect.x, screen.y - parentRect.y);

        SetVisible(true);
        m_rt->SetAnchoredPosition(finalPos);
    }

    void UIFollowTarget::OnGui()
    {
        ImGui::InputText("Target", &m_targetName); ImGui::SameLine();
        if (ImGui::Button("Rebind"))
        {
            RebindTarget();
        }

        ImGui::DragFloat3("Offset", &m_offset.x);
        ImGui::Checkbox("Visible", &m_visible);
    }

    void UIFollowTarget::Save(engine::json& j) const
    {
        Object::Save(j);
        j["TargetName"] = m_targetName;
        j["Offset"] = m_offset;
    }

    void UIFollowTarget::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "TargetName", m_targetName);
        engine::JsonGet(j, "Offset", m_offset);
    }

    void UIFollowTarget::RebindTarget()
    {
        if (m_lastBoundName == m_targetName && m_target != nullptr)
            return;

        m_target = nullptr;
        m_lastBoundName = m_targetName;

        if (m_targetName.empty())
            return;

        m_target = engine::GameObject::Find(m_targetName);
    }

    void UIFollowTarget::PrepareAnchorOnce()
    {
        if (!m_rt || m_anchorsPrepared) return;

        m_rt->SetAnchorMin({ 0.0f, 0.0f });
        m_rt->SetAnchorMax({ 0.0f, 0.0f });
        m_rt->SetPivot({ 0.5f, 0.5f });

        m_anchorsPrepared = true;
    }

    void UIFollowTarget::SetVisible(bool v)
    {
        if (!m_img) return;

        m_visible = v;

        engine::Vector4 c = m_img->GetColor();
        c.w = m_visible ? 1.f : 0.f;
        m_img->SetColor(c);
    }
}