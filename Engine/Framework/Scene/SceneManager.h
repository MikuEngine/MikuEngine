#pragma once

#include <functional>

#include "Common/Utility/Singleton.h"

namespace engine
{
    enum class SceneState
    {
        Active,
        Loading
    };

    class Scene;

    class SceneManager :
        public Singleton<SceneManager>
    {
    private:
        std::unique_ptr<Scene> m_scene;

        std::string m_nextSceneName;
        SceneState m_sceneState = SceneState::Active;
        bool m_isSceneChanged = false;

    private:
        SceneManager();
        ~SceneManager();

    public:
        void Initialize();
        void Shutdown();

    public:
        void ChangeScene(const std::string& name);
        void CheckSceneChanged(bool isPlaying);
        void ProcessResourceLoading();
        void RenderLoadingScreen();
        Scene* GetScene() const;
        void ProcessPendingAdds(bool isPlaying);
        void ProcessPendingKills();

        SceneState GetSceneState() const;

    private:
        friend class Singleton<SceneManager>;
    };
}