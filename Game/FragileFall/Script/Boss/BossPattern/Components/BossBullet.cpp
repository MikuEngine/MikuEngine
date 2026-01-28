#include "GamePCH.h"
#include "BossBullet.h"

#include <Framework/Object/Component/Collider.h>
#include <Framework/Physics/CollisionSystem.h>

#include "Script/CharacterScript/Player/PlayerControllerScript.h"

namespace game
{
    void BossBullet::Awake()
    {
    }

    void BossBullet::Start()
    {
    }

    void BossBullet::Update()
    {
        if (m_isDestroyed) return;

        float deltaTime = engine::Time::DeltaTime();
        m_elapsedTime += deltaTime;

        // 수명 체크
        if (m_elapsedTime >= m_lifetime)
        {
            if (GetGameObject())
            {
                GetGameObject()->Destroy();
            }
            m_isDestroyed = true;
            return;
        }

        // 이동
        auto* transform = GetGameObject()->GetTransform();
        if (transform)
        {
            engine::Vector3 currentPos = transform->GetWorldPosition();
            engine::Vector3 newPos = currentPos + m_direction * m_speed * deltaTime;
            transform->SetLocalPosition(newPos);
        }
    }

    void BossBullet::Setup(const engine::Vector3& direction, float speed, float damage, float lifetime)
    {
        // 방향 정규화
        float length = direction.Length();
        if (length > 0.001f)
        {
            m_direction = direction / length;
        }
        else
        {
            m_direction = engine::Vector3(0.0f, 0.0f, 1.0f);  // 기본 방향
        }

        m_speed = speed;
        m_damage = damage;
        m_lifetime = lifetime;
        m_elapsedTime = 0.0f;
        m_isDestroyed = false;
    }

    void BossBullet::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        if (m_isDestroyed)
        {
            return;
        }

        // 플레이어와 충돌 체크
        if (info.gameObject)
        {
            auto* player = info.gameObject->GetComponent<PlayerControllerScript>();
            if (player)
            {
                // TODO: 플레이어에게 데미지 주기
                // player->TakeDamage(static_cast<int>(m_damage));

                // 탄환 파괴
                GetGameObject()->Destroy();

                m_isDestroyed = true;
            }
        }
    }

    void BossBullet::OnGui()
    {
        ImGui::DragFloat("Speed", &m_speed, 0.1f, 0.1f, 100.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("Damage", &m_damage, 0.1f, 0.1f, 100.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("Lifetime", &m_lifetime, 0.1f, 0.1f, 100.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
    }

    void BossBullet::Save(engine::json& j) const
    {
        Object::Save(j);

        j["Speed"] = m_speed;  // 이동 속도
        j["Damage"] = m_damage;  // 데미지
        j["Lifetime"] = m_lifetime;  // 수명 (초)
    }

    void BossBullet::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "Speed", m_speed);
        engine::JsonGet(j, "Damage", m_damage);
        engine::JsonGet(j, "Lifetime", m_lifetime);
    }
}
