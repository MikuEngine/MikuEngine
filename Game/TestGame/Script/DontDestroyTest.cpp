#include "GamePCH.h"
#include "DontDestroyTest.h"

#include "Framework/Scene/SceneManager.h"

namespace game
{
    static engine::Ptr<engine::GameObject> s_instance = nullptr;

    void DontDestroyTest::Awake()
    {
        // 이 방식은 Start 타이밍에 호출해주기 위한 Awake에서 등록하는 방식
        // DontDestroyOnLoaded랑 상관없이 그냥 Awake에서 등록하고
        // OnSceneStart 에서 호출됨을 보여주기 위함

        auto onStart = []()
            {
                LOG_PRINT("DontDestroyTest - onStart callback");
            };

        engine::SceneManager::Get().RegisterOnSceneStart(onStart);
    }

    void DontDestroyTest::Start()
    {
        if (s_instance == nullptr)
        {
            s_instance = GetGameObject();

            GetGameObject()->DontDestroyOnLoad();
        }
        else
        {
            GetGameObject()->Destroy();
        }
    }

    void DontDestroyTest::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::D1))
        {
            // 이 방식은 씬을 변경하기 전에 콜백을 등록해서
            // 다음 씬이 로드되면 호출되는 것을 보여주기 위함.

            auto onLoaded = []()
                {
                    LOG_PRINT("DontDestroyTest - onLoaded callback");
                };

            engine::SceneManager::Get().RegisterOnSceneLoaded(onLoaded);

            engine::SceneManager::Get().ChangeScene("DontDestroyTest1");
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D2))
        {
            // 이 방식은 GameObject가 DontDestroyOnLoad가 됐을 때
            // 씬이 변경되더라도 스크립트의 함수를 호출할 수 있는 것을 보여주기 위함.

            auto onStart = [self = engine::Ptr<DontDestroyTest>(this)]()
                {
                    self->OnStartFromScript();
                };

            engine::SceneManager::Get().RegisterOnSceneStart(onStart);

            engine::SceneManager::Get().ChangeScene("DontDestroyTest2");
        }
    }

    void DontDestroyTest::OnGui()
    {
    }

    void DontDestroyTest::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void DontDestroyTest::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    void DontDestroyTest::OnStartFromScript()
    {
        LOG_PRINT("DontDestroyTest - OnStartFromScript");
    }
}