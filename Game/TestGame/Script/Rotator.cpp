#include "GamePCH.h"
#include "Rotator.h"

namespace game
{
    void Rotator::Update()
    {
        GetTransform()->Rotate(engine::Vector3::UnitZ, 90.0f * m_speed * engine::Time::DeltaTime());
    }

    void Rotator::OnGui()
    {
        ImGui::InputFloat("Rotate Speed", &m_speed);
    }

    void Rotator::Save(engine::json& j) const
    {
        j["Type"] = GetType();
        j["Speed"] = m_speed;
    }

    void Rotator::Load(const engine::json& j)
    {
        engine::JsonGet(j, "Speed", m_speed);
    }

    std::string Rotator::GetType() const
    {
        return "Rotator";
    }
}