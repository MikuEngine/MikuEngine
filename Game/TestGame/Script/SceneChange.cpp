#include "GamePCH.h"
#include "SceneChange.h"

#include "Framework/Scene/SceneManager.h"

namespace game
{
    void SceneChange::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::D1))
        {
            engine::SceneManager::Get().ChangeScene("DontDestroyTest1");
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D2))
        {
            engine::SceneManager::Get().ChangeScene("DontDestroyTest2");
        }
    }

    void SceneChange::OnGui()
    {
    }

    void SceneChange::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void SceneChange::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    std::string SceneChange::GetType() const
    {
        return "SceneChange";
    }
}