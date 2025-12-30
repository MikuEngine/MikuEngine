#include "pch.h"
#include "SceneManager.h"

#include "Framework/Scene/Scene.h"

namespace engine
{
    SceneManager::SceneManager() = default;
    SceneManager::~SceneManager() = default;

    void SceneManager::Initialize()
    {
        m_scene = std::make_unique<Scene>();
        m_scene->SetName("SampleScene");
        m_scene->ResetToDefaultScene();
    }

    void SceneManager::Shutdown()
    {
        m_scene.reset();
    }

    void SceneManager::ChangeScene(const std::string& name)
    {
        m_nextSceneName = name;
        m_isSceneChanged = true;
    }

    void SceneManager::CheckSceneChanged()
    {
        if (m_isSceneChanged)
        {
            m_scene->SetName(m_nextSceneName);
            m_scene->Load();

            m_isSceneChanged = false;
        }
    }

    Scene* SceneManager::GetScene() const
    {
        return m_scene.get();
    }

    void SceneManager::ProcessPendingAdds(bool isPlaying)
    {
        m_scene->ProcessPendingAdds(isPlaying);
    }

    void SceneManager::ProcessPendingKills()
    {
        m_scene->ProcessPendingKills();
    }
}