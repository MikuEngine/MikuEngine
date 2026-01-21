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

        // 모든 콜백은 호출된 후에 다 비워짐.
        // 그래서 매 Scene 마다 지속적으로 호출하고 싶다면 Scene을 변경하는 부분에서 콜백 등록이 필요함.
        // 주의할 점은 캡쳐하는 변수는 무조건 생명 보장되어야함
        // Object를 상속받는 객체라면 Ptr<>로 감싸서 참조가 아닌 값으로 넘겨서 유효한지 확인 가능함
        std::vector<std::function<void()>> m_onSceneLoadedCallbacks;
        std::vector<std::function<void()>> m_onSceneStartCallbacks;


        std::string m_nextSceneName;
        SceneState m_sceneState = SceneState::Active;
        bool m_isSceneChanged = false;
        bool m_isSceneStarted = false;

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

        void RegisterOnSceneLoaded(std::function<void()>&& callback);

        // 초기 Scene 로드가 끝난 후
        void CallOnSceneLoaded();

        void RegisterOnSceneStart(std::function<void()>&& callback);

        // 초기 Scene의 GameObject, Component의 Initialize, Awake가 끝난 후
        // Script의 Start가 불리기 직전
        void CallOnSceneStart();

    private:
        friend class Singleton<SceneManager>;
    };
}