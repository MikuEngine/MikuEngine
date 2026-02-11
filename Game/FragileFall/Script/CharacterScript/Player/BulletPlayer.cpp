#include "GamePCH.h"
#include "BulletPlayer.h"

#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Asset/Prefab.h>
#include "Script/CharacterScript/Monster/MonsterScript.h"
#include "Script/Boss/BossPattern/Components/BossPillar.h"
#include "Script/Boss/BossPattern/Components/BossBigProjectile.h"
#include "Script/Boss/BossScript.h"
#include "Script/Interface/IDamageable.h"

namespace game
{
    namespace
    {
        // BulletPlayer 메쉬는 +Y 축이 정면이므로 X축 +90도 보정이 필요하다.
        // 방향은 사용자가 요청한 대로 "현재의 반대 방향"을 바라보게 한다.
        engine::Quaternion BuildBulletRotationFromDirection(const engine::Vector3& position, const engine::Vector3& direction)
        {
            (void)position;

            engine::Vector3 normalizedDir = direction;
            normalizedDir.Normalize();

            // yaw는 XZ 평면 기준으로 계산.
            const float yawRad = std::atan2(normalizedDir.x, normalizedDir.z);
            const float yawDeg = engine::ToDegree(yawRad);

            // +Y 정면 메쉬 보정: pitch +90
            const engine::Vector3 euler(90.0f, yawDeg, 0.0f);
            return engine::Quaternion::CreateFromYawPitchRoll(
                engine::ToRadian(euler.y),
                engine::ToRadian(euler.x),
                engine::ToRadian(euler.z));
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 초기화 (Factory에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void BulletPlayer::Setup(std::unique_ptr<IBulletMovement> movement, float lifetime, float dmg, float range, float scale)
    {
        m_movement = std::move(movement);
        m_lifetime = lifetime;  // 하위 호환성용 (사용 안 함)
        m_range = range;
        m_damage = dmg;
        m_scale = scale;
    }

    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void BulletPlayer::Start()
    {
        m_elapsedTime = 0.0f;

        // ─────────────────────────────────────────────
        // 총알 스케일 적용 (균등 스케일)
        // - syncWithTransform=true이므로 Collider도 자동 스케일됨
        // ─────────────────────────────────────────────
        GetTransform()->SetLocalScale(engine::Vector3(m_scale, m_scale, m_scale));

        // 발사 위치와 방향 저장 (사거리 계산용)
        m_startPosition = GetTransform()->GetWorldPosition();
        if (m_movement)
        {
            engine::Vector3 velocity = m_movement->GetVelocity();
            bool hasValidRotation = false;
            engine::Quaternion bulletRot = engine::Quaternion::Identity;

            if (velocity.LengthSquared() > 0.0001f)
            {
                m_direction = velocity;
                m_direction.Normalize();

                bulletRot = BuildBulletRotationFromDirection(m_startPosition, m_direction);
                hasValidRotation = true;
                GetTransform()->SetLocalRotation(bulletRot);
            }
            
            if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
            {
                // Dynamic Rigidbody는 이후 PhysX -> Transform 동기화가 이루어지므로
                // 초기 회전을 Actor에도 즉시 반영해 두지 않으면 Transform 회전이 덮어써질 수 있다.
                if (hasValidRotation)
                {
                    rb->ForceSetRotation(bulletRot, true);
                }
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

	// Start에서 한 번 회전 세팅하면 충분함
    void BulletPlayer::LateUpdate()
    {
        // no-op
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백 (Push 방식)
    // ═══════════════════════════════════════════════════════════════
    void BulletPlayer::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        if (m_isDying) return;
        if (!info.gameObject) return;

		// IDamageable 상속
        // MonsterScript
        // BossPillar
        // BossBigProjectile
        // BossScript
        if (auto* target = info.gameObject->GetInterface<IDamageable>())
        {
            target->TakeDamage(m_damage);

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

        // 환경, 벽과 충돌 시
        auto collider = info.gameObject->GetComponent<engine::Collider>();
        if (!collider) return;
        auto layer = collider->GetLayer();

        bool isEnvironment = (layer == engine::PhysicsLayer::Environment);
        bool isWall = (layer == engine::PhysicsLayer::Wall);
        
        if (isEnvironment || isWall)
        {
            // dying 상태로 전환
            m_isDying = true;
            m_deathTimer = 0.0f;

            // 속도 정지
            if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
            {
                rb->SetLinearVelocity(engine::Vector3::Zero);
            }

            auto effect = engine::Prefab::Instantiate("Effect_Bullet_Destory_V1.00");
            if (effect && effect->GetTransform())
            {
                effect->GetTransform()->SetWorldMatrix(GetTransform()->GetWorld());
                effect->GetTransform()->SetLocalScale(engine::Vector3(1.0f, 1.0f, 1.0f));
            }
            return;
        }
    }
}
