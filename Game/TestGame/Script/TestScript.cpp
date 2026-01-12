#include "GamePCH.h"
#include "TestScript.h"

#include "Test2.h"


namespace game
{
    void TestScript::Awake()
    {
        auto gameObject = CreateGameObject("Test2");
        gameObject->AddComponent<Test2>();
    }

    void TestScript::Start()
    {
        LOG_PRINT("TestScript Start");

        LOG_INFO("asdf");
    }

    void TestScript::Update()
    {
        if (engine::Input::IsKeyHeld(engine::Keys::Left))
        {
            engine::Vector3 position = GetTransform()->GetLocalPosition();

            position += engine::Vector3::Left * engine::Time::DeltaTime() * m_speed;

            GetTransform()->SetLocalPosition(position);
        }

        if (engine::Input::IsKeyHeld(engine::Keys::Right))
        {
            engine::Vector3 position = GetTransform()->GetLocalPosition();

            position -= engine::Vector3::Left * engine::Time::DeltaTime() * m_speed;

            GetTransform()->SetLocalPosition(position);
        }
    }

    void TestScript::OnGui()
    {
        ImGui::InputFloat("Speed", &m_speed);
    }

    void TestScript::Save(engine::json& j) const
    {
        Object::Save(j);

        j["Speed"] = m_speed;
    }

    void TestScript::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "Speed", m_speed);
    }

    std::string TestScript::GetType() const
    {
        return "TestScript";
    }
}