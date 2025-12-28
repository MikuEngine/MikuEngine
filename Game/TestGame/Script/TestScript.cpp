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
        j["Type"] = GetType();
        j["Speed"] = m_speed;
    }

    void TestScript::Load(const engine::json& j)
    {
        engine::JsonGet(j, "Speed", m_speed);
    }

    std::string TestScript::GetType() const
    {
        return "TestScript";
    }
}