#include "GamePCH.h"
#include "LoadingOverlay.h"

namespace
{
    // 자식 포함 재귀 탐색으로 UIProgressBar 찾기
    static engine::UIProgressBar* FindProgressBarInChildren(engine::Transform* t)
    {
        if (!t) return nullptr;

        if (auto* go = t->GetGameObject())
        {
            if (auto* bar = go->GetComponent<engine::UIProgressBar>())
                return bar;
        }

        for (engine::Transform* c : t->GetChildren())
        {
            if (auto* found = FindProgressBarInChildren(c))
                return found;
        }

        return nullptr;
    }
}

namespace game
{
    void LoadingOverlay::Awake()
    {
        auto* go = GetGameObject();
        if (!go) return;

        m_bar = go->GetComponent<engine::UIProgressBar>();

        if (!m_bar)
            m_bar = FindProgressBarInChildren(go->GetTransform());

        Hide();
        SetProgress(0.0f);
    }

    void LoadingOverlay::Show()
    {
        m_visible = true;

        if (auto* go = GetGameObject())
            go->SetActive(true);
    }

    void LoadingOverlay::Hide()
    {
        m_visible = false;

        if (auto* go = GetGameObject())
            go->SetActive(false);
    }

    void LoadingOverlay::SetProgress(float t)
    {
        if (!m_bar) return;

        m_bar->SetValue(m_progress);
    }

    void LoadingOverlay::OnGui()
    {
        ImGui::Checkbox("Visible", &m_visible);
        ImGui::DragFloat("Progress", &m_progress);
    }

    void LoadingOverlay::Save(engine::json& j) const
    {
        Object::Save(j);
        j["Visible"] = m_visible;
        j["Progress"] = m_progress;
    }

    void LoadingOverlay::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "Visible", m_visible);
        engine::JsonGet(j, "Progress", m_progress);
    }
}