#pragma once

#include <functional>

#include "Common/Utility/Singleton.h"

namespace engine
{
    class Scene;

    class SceneManager :
        public Singleton<SceneManager>
    {
    private:
        std::unique_ptr<Scene> m_scene;

        std::string m_nextSceneName;
        bool m_isSceneChanged = false;

    private:
        SceneManager();
        ~SceneManager();

    public:
        void Initialize();
        void Shutdown();

    public:
        void ChangeScene(const std::string& name);
        void CheckSceneChanged();
        Scene* GetScene() const;
        void ProcessPendingAdds(bool isPlaying);
        void ProcessPendingKills();

    private:
        friend class Singleton<SceneManager>;
    };
}