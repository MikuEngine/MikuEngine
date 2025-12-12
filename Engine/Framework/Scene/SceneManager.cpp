#include "pch.h"
#include "SceneManager.h"

#include "Framework/Scene/Scene.h"

namespace engine
{
    SceneManager& SceneManager::Get()
    {
        static SceneManager s_instance;

        return s_instance;
    }

    void SceneManager::Shutdown()
    {
        if (m_currentScene != nullptr)
        {
            m_currentScene->Exit();
        }

        m_scenes.clear();
    }

    void SceneManager::CreateScene(std::string_view name)
    {
        if (m_scenes.find(name.data()) == m_scenes.end())
        {
            std::unique_ptr<Scene> newScene = std::make_unique<Scene>();
            m_scenes.emplace(name, std::move(newScene));
        }
    }

    void SceneManager::ChangeScene(std::string_view name)
    {
    }

    void SceneManager::CheckSceneChanged()
    {
    }
}