#include "GamePCH.h"
#include "BulletPlayer.h"

#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Collider.h>
#include "Script/CharacterScript/Monster/MonsterScript.h"
#include "Script/Boss/BossPattern/Components/BossPillar.h"
#include "Script/Boss/BossPattern/Components/BossProjectile.h"
#include "Script/Boss/BossScript.h"

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 초기화 (Factory에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void BulletPlayer::Setup(std::unique_ptr<IBulletMovement> movement, float lifetime, float dmg, float range)
    {
        m_movement = std::move(movement);
        m_lifetime = lifetime;  // 하위 호환성용 (사용 안 함)
        m_range = range;
        m_damage = dmg;
    }

    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void BulletPlayer::Start()
    {
        m_elapsedTime = 0.0f;

        // 발사 위치와 방향 저장 (사거리 계산용)
        m_startPosition = GetTransform()->GetWorldPosition();
        if (m_movement)
        {
            engine::Vector3 velocity = m_movement->GetVelocity();
            if (velocity.LengthSquared() > 0.0001f)
            {
                m_direction = velocity;
                m_direction.Normalize();
            }
            
            // Rigidbody에 초기 속도 설정
            if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
            {
                rb->SetLinearVelocity(velocity);
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

        // 사거리 기반 생명주기 관리
        engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
        engine::Vector3 displacement = currentPos - m_startPosition;
        
        // 발사 방향으로의 이동 거리 계산 (투영)
        float distanceTraveled = displacement.Dot(m_direction);
        
        // 목적지보다 멀리 이동했으면 삭제
        if (distanceTraveled >= m_range)
        {
            GetGameObject()->Destroy();
            return;
        }

        // 화면 밖 체크 (간단한 범위 체크, 안전장치)
        float boundary = 100.0f;  // 사거리보다 큰 값으로 설정
        if (std::abs(currentPos.x) > boundary || std::abs(currentPos.z) > boundary)
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

        // BossProjectile과 충돌 시 결정화 처리
        if (auto* projectile = info.gameObject->GetComponent<BossProjectile>())
        {
            // 결정화 가능한 상태면 결정화 처리
            if (!projectile->IsCrystallized())
            {
                projectile->OnCrystallized();
            }

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
        
    }
}
