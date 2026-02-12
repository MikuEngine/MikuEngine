#include "GamePCH.h"
#include "SoundTrigger.h"

#include <Framework/System/SoundSystem.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Object/Component/SphereCollider.h>

namespace game
{
    void SoundTrigger::Start()
    {
		SetActivateSound(m_isActive);
    }

    void SoundTrigger::SetActivateSound(bool active)
    {
        m_isActive = active;

        // 현재 Collision Active false가 안되서 로직을 넣음
        if (m_isActive)
        {
            auto* collider = GetGameObject()->GetComponent<engine::Collider>();
            if (!collider)
            {
                auto* newCollider = GetGameObject()->AddComponent<engine::SphereCollider>();

				newCollider->SetRadius(m_radius);
				newCollider->SetIsTrigger(true);
            }
        }
        else
        {
            auto* collider = GetGameObject()->GetComponent<engine::Collider>();
            if (collider)
            {
                collider->Destroy();
            }
        }
    }

    void SoundTrigger::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        if (info.gameObject->GetName() == "Player")
        {
            if (!m_soundKey.empty())
            {
                engine::SoundSystem::Get().Play(m_soundKey, m_soundOption);
            }
        }
    }

    void SoundTrigger::OnGui()
    {
        ImGui::Checkbox("isSoundActive", &m_isActive);

        if (ImGui::DragFloat("Radius", &m_radius, 0.1f, 0.0f, 20.0f))
        {
            if (auto* collider = GetGameObject()->GetComponent<engine::SphereCollider>())
            {
                collider->SetRadius(m_radius);
            }
        }


        char soundKeyBuffer[256], soundOptionBuffer[256];
        strcpy_s(soundKeyBuffer, m_soundKey.c_str());
        strcpy_s(soundOptionBuffer, m_soundOption.c_str());

        if (ImGui::InputText("Sound Key", soundKeyBuffer, sizeof(soundKeyBuffer)))
        {
            m_soundKey = soundKeyBuffer;
        }
        if (ImGui::InputText("Sound Option", soundOptionBuffer, sizeof(soundOptionBuffer)))
        {
            m_soundOption = soundOptionBuffer;
        }
    }

    void SoundTrigger::Save(engine::json& j) const
    {
        Object::Save(j);

        j["isTriggerActive"] = m_isActive;
        j["Radius"] = m_radius;
        j["SoundKey"] = m_soundKey;
        j["SoundOption"] = m_soundOption;
    }

    void SoundTrigger::Load(const engine::json& j)
    {
        Object::Load(j);

		engine::JsonGet(j, "isTriggerActive", m_isActive);
		engine::JsonGet(j, "Radius", m_radius);
        engine::JsonGet(j, "SoundKey", m_soundKey);
        engine::JsonGet(j, "SoundOption", m_soundOption);
    }
}