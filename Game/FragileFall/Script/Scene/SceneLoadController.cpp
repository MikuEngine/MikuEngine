#include "GamePCH.h"
#include "SceneLoadController.h"

#include <Framework/Scene/Scene.h>
#include <Framework/Scene/SceneManager.h>

namespace
{
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

    static float Clamp01(float v)
    {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }
}

namespace game
{
    void SceneLoadController::Awake()
    {
        CacheRefs();

        if (m_overlayGO)
            m_overlayGO->SetActive(false);

        if (m_bar)
            m_bar->SetValue(0.0f);

        m_prevLoading = false;
    }

    void SceneLoadController::Start()
    {
        
    }

    void SceneLoadController::Update()
    {
        CacheRefs();

        auto& sm = engine::SceneManager::Get();
        const bool isLoading = (sm.GetSceneState() == engine::SceneState::Loading);

        if (isLoading && !m_prevLoading)
        {
            if (m_overlayGO)
                m_overlayGO->SetActive(true);

            if (m_bar)
            {
                // 로딩 시작 시 0으로 초기화
                m_bar->SetValue(0.0f);
            }
        }

        // Loading 종료(Active 복귀)
        if (!isLoading && m_prevLoading)
        {
            if (m_bar)
            {
                m_bar->SetValue(1.0f);
            }

            if (m_overlayGO)
                m_overlayGO->SetActive(false);
        }

        m_prevLoading = isLoading;
    }

    void SceneLoadController::RequestChangeScene(const std::string& sceneName)
    {
        CacheRefs();

        if (m_overlayGO)
            m_overlayGO->SetActive(true);

        if (m_bar)
            m_bar->SetValue(0.0f);

        engine::SceneManager::Get().ChangeScene(sceneName);
    }

    void SceneLoadController::OnGui()
    {

    }

    void SceneLoadController::Save(engine::json& j) const
    {
        Object::Save(j);
        j["OverlayObjectName"] = m_overlayObjectName;
    }

    void SceneLoadController::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "OverlayObjectName", m_overlayObjectName);
    }

    void SceneLoadController::CacheRefs()
    {
        if (!m_overlayGO)
            m_overlayGO = engine::GameObject::Find(m_overlayObjectName);

        if (!m_overlayGO)
        {
            m_bar = nullptr;
            return;
        }

        if (!m_bar)
        {
            m_bar = m_overlayGO->GetComponent<engine::UIProgressBar>();
            if (!m_bar)
                m_bar = FindProgressBarInChildren(m_overlayGO->GetTransform());
        }
    }
}