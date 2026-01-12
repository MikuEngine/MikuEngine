#include "GamePCH.h"
#include "Rotator.h"

#include "TestScript.h"

namespace game
{
    void Rotator::Awake()
    {
        auto gameObject = CreateGameObject("TestScript Awake");
        gameObject->AddComponent<TestScript>();

        //gameObject->Destroy();
    }

    void Rotator::Start()
    {
        LOG_PRINT("Rotator Start");
    }

    void Rotator::Update()
    {
        GetTransform()->Rotate(engine::Vector3::UnitZ, 90.0f * m_speed * engine::Time::DeltaTime());

        if (engine::Input::IsKeyPressed(engine::Keys::G))
        {
            auto gameObject = CreateGameObject("TestScript Update");
            gameObject->AddComponent<TestScript>();

            //gameObject->Destroy();
        }
    }

    void Rotator::OnGui()
    {
        ImGui::InputFloat("Rotate Speed", &m_speed);
    }

    void Rotator::Save(engine::json& j) const
    {
        Object::Save(j);

        j["Speed"] = m_speed;
    }

    void Rotator::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "Speed", m_speed);
    }

    std::string Rotator::GetType() const
    {
        return "Rotator";
    }
}