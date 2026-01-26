#include "GamePCH.h"
#include "TestScript.h"

#include "Test2.h"

#include <Framework/Object/Component/Renderer/StaticMeshRenderer.h>

namespace game
{
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

        if (engine::Input::IsKeyPressed(engine::Keys::D1))
        {
            auto renderer = GetGameObject()->GetComponent<engine::StaticMeshRenderer>();
            renderer->SetObstacleAlpha(true, 0.5f);
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
}