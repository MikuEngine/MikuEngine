#include "GamePCH.h"
#include "UIFollowWorldTarget.h"

#include <Core/Graphics/Device/GraphicsDevice.h>

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Canvas.h>
#include <Framework/Object/Component/RectTransform.h>
#include <Framework/Object/Component/UI/UIImage.h>

#include <Framework/Object/Component/Camera.h>

namespace game
{
    namespace
    {
        static engine::Canvas* FindCanvasInParents(engine::GameObject* go)
        {
            if (!go) return nullptr;

            engine::Transform* t = go->GetTransform();
            while (t)
            {
                engine::GameObject* cur = t->GetGameObject();
                if (cur)
                {
                    if (auto* c = cur->GetComponent<engine::Canvas>())
                        return c;
                }

                t = t->GetParent();
            }

            return nullptr;
        }
    }

    void UIFollowWorldTarget::Awake()
    {
        auto* go = GetGameObject();
        if (!go) return;

        m_rt = go->GetComponent<engine::RectTransform>();
        
        PrepareAnchorOnce();
        RebindTarget();

        if (auto* camGO = engine::GameObject::Find("MainCamera"))
            m_camera = camGO->GetComponent<engine::Camera>();
        
        m_cameraCached = (m_camera != nullptr);

        if (m_rt)
            m_parentRT = m_rt->FindPrentRectTransform();

        m_cachedVpW = -1.f;
        m_cachedVpH = -1.f;
        m_cachedVisible = true;
    }

    void UIFollowWorldTarget::Update()
    {

        if (!m_rt || !m_cameraCached || !m_target)
        {
            SetVisible(false);
            return;
        }

        if (!m_visible)
        {
            SetVisible(false);
            return;
        }

        engine::Canvas* c = FindCanvasInParents(GetGameObject());

        if (!c)
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
        const float vpW = vp.Width;
        const float vpH = vp.Height;

        // NDC -> Screen(px)
        engine::Vector2 screenPx;
        screenPx.x = (ndcX * 0.5f + 0.5f) * vpW;
        screenPx.y = (-ndcY * 0.5f + 0.5f) * vpH;

        // Screen(px) -> Canvas Ref
        const engine::Vector2 scale = c->GetUIScale();
        const engine::Vector2 offset = c->GetUIOffset();

        engine::Vector2 screenRef;
        screenRef.x = (screenPx.x - offset.x) / scale.x;
        screenRef.y = (screenPx.y - offset.y) / scale.y;

        const engine::Vector2 ref = c->GetReferenceResolution();
        const engine::UIRect refRoot{ 0.f, 0.f, ref.x, ref.y };

        const engine::UIRect parentRect = (m_parentRT)
            ? m_parentRT->GetWorldRectResolved(refRoot)
            : refRoot;

        const engine::Vector2 finalPos(
            screenRef.x - parentRect.x,
            screenRef.y - parentRect.y
        );

        SetVisible(true);
        m_rt->SetAnchoredPosition(finalPos);
    }

    void UIFollowWorldTarget::OnGui()
    {
        ImGui::InputText("Target", &m_targetName); ImGui::SameLine();
        if (ImGui::Button("Rebind"))
        {
            RebindTarget();
        }

        ImGui::DragFloat3("Offset", &m_offset.x);
        //ImGui::Checkbox("Hide When Offscreen", &m_hideWhenOffscreen);
        ImGui::Checkbox("Visible", &m_visible);
    }

    void UIFollowWorldTarget::Save(engine::json& j) const
    {
        Object::Save(j);
        j["TargetName"] = m_targetName;
        j["Offset"] = m_offset;
        //j["HideWhenOffscreen"] = m_hideWhenOffscreen;
    }

    void UIFollowWorldTarget::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "TargetName", m_targetName);
        engine::JsonGet(j, "Offset", m_offset);
        //engine::JsonGet(j, "HideWhenOffscreen", m_hideWhenOffscreen);

        m_target = nullptr;
        m_lastBoundName.clear();
        RebindTarget();
    }

    void UIFollowWorldTarget::RebindTarget()
    {
        if (m_lastBoundName == m_targetName && m_target != nullptr) return;
            
        m_target = nullptr;
        m_lastBoundName = m_targetName;

        if (m_targetName.empty()) return;
            
        m_target = engine::GameObject::Find(m_targetName);
    }

    void UIFollowWorldTarget::PrepareAnchorOnce()
    {
        if (!m_rt || m_anchorsPrepared) return;

        m_rt->SetAnchorMin({ 0.0f, 0.0f });
        m_rt->SetAnchorMax({ 0.0f, 0.0f });
        m_rt->SetPivot({ 0.5f, 0.5f });

        m_anchorsPrepared = true;
    }

    void UIFollowWorldTarget::SetVisible(bool v)
    {
        auto* go = GetGameObject();
        if (!go) return;

        if (m_cachedVisible == v) return;

        m_cachedVisible = v;

        go->SetActive(v);
    }
}