#include "EnginePCH.h"
#include "SceneManager.h"

#include "Framework/Scene/Scene.h"
#include "Framework/Asset/PreloadManager.h"
#include "Framework/Asset/AssetManager.h"
#include "Core/Graphics/Resource/ResourceManager.h"
#include "Framework/System/SoundSystem.h"
#include "Framework/System/LoadingScreenRenderer.h"
#include "Framework/System/SystemManager.h"
#include "Framework/Physics/PhysicsSystem.h"
#include "Framework/Physics/CollisionSystem.h"

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

    void SceneManager::ChangeScene(const std::string& name, bool async)
    {
        m_nextSceneName = name;
        m_loadSceneAsync = async;
        m_isSceneChanged = true;
    }

    void SceneManager::CheckSceneChanged(bool isPlaying)
    {
        if (m_isSceneChanged && m_sceneState == SceneState::Active)
        {
            m_scene->SetName(m_nextSceneName);

            ResourceManager::Get().CleanupSceneScope();
            AssetManager::Get().CleanupSceneScope();

            m_isSceneChanged = false;
            m_sceneState = SceneState::Loading;
            
            // CollisionSystem의 활성 쌍 클리어
            SystemManager::Get().GetCollisionSystem().ClearActivePairs();
            
            // PhysX 씬 먼저 정리 (GameObject 파괴 전에 PxActor 해제)          
            m_scene->ClearPhysicsScene();

            if (isPlaying)
            {
                SoundSystem::Get().StopSceneSounds();
                SoundSystem::Get().OnGameStart();

                m_scene->Clear(true);

                if (m_loadSceneAsync)
                {
                    if (!PreloadManager::Get().IsGlobalLoaded())
                    {
                        m_pendingSceneAfterGlobal = m_nextSceneName;
                        PreloadManager::Get().LoadGlobalAsync();
                    }
                    else
                    {
                        PreloadManager::Get().LoadSceneResourceAsync(m_nextSceneName);
                    }
                }
                else
                {
                    PreloadManager::Get().LoadGlobalSync();
                    PreloadManager::Get().LoadSceneResourceSync(m_nextSceneName);
                }
            }
            else
            {
                m_scene->Clear(false);

                PreloadManager::Get().LoadSceneResourceSync(m_nextSceneName);
            }
        }
    }

    void SceneManager::ProcessResourceLoading()
    {
        if (m_sceneState == SceneState::Loading)
        {
            if (!PreloadManager::Get().IsLoading())
            {
                if (!m_pendingSceneAfterGlobal.empty())
                {
                    PreloadManager::Get().LoadSceneResourceAsync(m_pendingSceneAfterGlobal);
                    m_pendingSceneAfterGlobal.clear();
                    return;
                }

                m_scene->Load();

                // 물리 씬 생성 (씬 로드 후)
                SystemManager::Get().GetPhysicsSystem().CreateScenePhysics();

                m_sceneState = SceneState::Active;

                m_isSceneStarted = false;

                CallOnSceneLoaded();
            }
        }
    }

    void SceneManager::RenderLoadingScreen()
    {
        if (m_sceneState == SceneState::Loading)
        {
            float progress = PreloadManager::Get().GetProgress();
            LoadingScreenRenderer::Get().Render(progress);
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

    void SceneManager::RegisterOnSceneLoaded(std::function<void()>&& callback)
    {
        m_onSceneLoadedCallbacks.push_back(std::move(callback));
    }

    void SceneManager::CallOnSceneLoaded()
    {
        for (auto& callback : m_onSceneLoadedCallbacks)
        {
            std::invoke(callback);
        }

        m_onSceneLoadedCallbacks.clear();

        // 추가적으로 하고싶은 작업
    }

    void SceneManager::RegisterOnSceneStart(std::function<void()>&& callback)
    {
        m_onSceneStartCallbacks.push_back(std::move(callback));
    }

    void SceneManager::CallOnSceneStart()
    {
        if (!m_isSceneStarted)
        {
            for (auto& callback : m_onSceneStartCallbacks)
            {
                std::invoke(callback);
            }

            m_onSceneStartCallbacks.clear();

            // 추가적으로 하고싶은 작업

            m_isSceneStarted = true;
        }
    }
}