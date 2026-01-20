#include "EnginePCH.h"
#include "SceneManager.h"

#include "Framework/Scene/Scene.h"
#include "Framework/Asset/PreloadManager.h"
#include "Framework/Asset/AssetManager.h"
#include "Core/Graphics/Resource/ResourceManager.h"

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
        if (m_isSceneChanged && m_sceneState == SceneState::Active)
        {
            m_scene->SetName(m_nextSceneName);

            ResourceManager::Get().CleanupSceneScope();
            AssetManager::Get().CleanupSceneScope();

            m_scene->Clear();

            m_isSceneChanged = false;
            m_sceneState = SceneState::Loading;

            PreloadManager::Get().LoadSceneResourceAsync(m_nextSceneName);
        }
    }

    void SceneManager::ProcessResourceLoading()
    {
        if (m_sceneState == SceneState::Loading)
        {
            if (!PreloadManager::Get().IsLoading())
            {
                m_scene->Load();

                m_sceneState = SceneState::Active;
            }
        }
    }

    void SceneManager::RenderLoadingScreen()
    {
        if (m_sceneState == SceneState::Loading)
        {
            float progress = PreloadManager::Get().GetProgress();

            LOG_PRINT("loading progress: {}", progress);
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

    SceneState SceneManager::GetSceneState() const
    {
        return m_sceneState;
    }
}