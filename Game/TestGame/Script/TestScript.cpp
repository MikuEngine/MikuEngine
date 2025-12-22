#include "pch.h"
#include "TestScript.h"

#include "Framework/Object/Component/Transform.h"

namespace game
{
    void TestScript::Start()
    {
        LOG_PRINT("TestScript Start");

        LOG_INFO("asdf");
    }

    void TestScript::Update()
    {
        if (engine::Input::IsKeyHeld(DirectX::Keyboard::Left))
        {
            engine::Vector3 position = GetTransform()->GetLocalPosition();

            position += engine::Vector3::Left * engine::Time::DeltaTime() * 10;

            GetTransform()->SetLocalPosition(position);
        }

        if (engine::Input::IsKeyHeld(DirectX::Keyboard::Right))
        {
            engine::Vector3 position = GetTransform()->GetLocalPosition();

            position -= engine::Vector3::Left * engine::Time::DeltaTime() * 10;

            GetTransform()->SetLocalPosition(position);
        }
    }
}