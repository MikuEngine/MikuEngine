#include "GamePCH.h"
#include "BulletPlayer.h"

#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Collider.h>

#include "Script/CharacterScript/Monster/TempMonster.h"
#include "Script/CharacterScript/Monster/MonsterScript.h"
#include "Script/Boss/BossPattern/Components/BossPillar.h"
#include "Script/Boss/BossScript.h"

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 초기화 (Factory에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void BulletPlayer::Setup(std::unique_ptr<IBulletMovement> movement, float lifetime)
    {
        m_movement = std::move(movement);
        m_lifetime = lifetime;
    }

    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void BulletPlayer::Start()
    {
        m_elapsedTime = 0.0f;

        // Rigidbody에 초기 속도 설정
        if (m_movement)
        {
            if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
            {
                rb->SetLinearVelocity(m_movement->GetVelocity());
            }
        }
    }

    void BulletPlayer::Update()
    {
        // 죽는 중이면 타이머만 체크
        if (m_isDying)
        {
            m_deathTimer += engine::Time::DeltaTime();
            if (m_deathTimer >= m_deathDelay)
            {
                GetGameObject()->Destroy();
            }
            return;
        }

        // 생존 시간 누적
        float dt = engine::Time::DeltaTime();
        m_elapsedTime += dt;

        // 수명 체크
        if (m_elapsedTime >= m_lifetime)
        {
            GetGameObject()->Destroy();
            return;
        }

        // 화면 밖 체크 (간단한 범위 체크)
        engine::Vector3 pos = GetTransform()->GetWorldPosition();
        float boundary = 50.0f;
        if (std::abs(pos.x) > boundary || std::abs(pos.z) > boundary)
        {
            GetGameObject()->Destroy();
            return;
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백 (Push 방식)
    // ═══════════════════════════════════════════════════════════════
    void BulletPlayer::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        if (m_isDying) return;
        if (!info.gameObject) return;

        // MonsterScript와 충돌했는지 확인 (우선 체크)
        if (auto* monster = info.gameObject->GetComponent<MonsterScript>())
        {
            monster->TakeDamage(m_damage);

            // dying 상태로 전환
            m_isDying = true;
            m_deathTimer = 0.0f;

            // 속도 정지
            if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
            {
                rb->SetLinearVelocity(engine::Vector3::Zero);
            }
            return;
        }

        if (auto* pillar = info.gameObject->GetComponent<BossPillar>())
        {
            pillar->TakeDamage(m_damage);

            // dying 상태로 전환
            m_isDying = true;
            m_deathTimer = 0.0f;

            // 속도 정지
            if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
            {
                rb->SetLinearVelocity(engine::Vector3::Zero);
            }
            return;
        }

        if (auto* boss = info.gameObject->GetComponent<BossScript>())
        {
            boss->TakeDamage(m_damage);

            // dying 상태로 전환
            m_isDying = true;
            m_deathTimer = 0.0f;

            // 속도 정지
            if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
            {
                rb->SetLinearVelocity(engine::Vector3::Zero);
            }
            return;
        }

        // TempMonster와 충돌했는지 확인 (레거시 호환)
        if (auto* tempMonster = info.gameObject->GetComponent<TempMonster>())
        {
            tempMonster->OnHit();

            // dying 상태로 전환
            m_isDying = true;
            m_deathTimer = 0.0f;

            // 속도 정지
            if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
            {
                rb->SetLinearVelocity(engine::Vector3::Zero);
            }
        }
    }
}
