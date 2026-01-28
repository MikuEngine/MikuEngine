#include "GamePCH.h"
#include "BossProjectile.h"

#include <Framework/Object/Component/Transform.h>
#include <Framework/Object/Component/Collider.h>

#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/Boss/BossScript.h"

namespace game
{
    void BossProjectile::Awake()
    {
    }

    void BossProjectile::Start()
    {
    }

    void BossProjectile::Update()
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

        // 결정화 상태가 아니면 이동
        if (!m_isCrystallized)
        {
            auto* transform = GetGameObject()->GetTransform();
            if (transform)
            {
                engine::Vector3 currentPos = transform->GetWorldPosition();
                engine::Vector3 newPos = currentPos + m_direction * m_speed * deltaTime;
                transform->SetLocalPosition(newPos);
            }
        }
    }

    void BossProjectile::Setup(const engine::Vector3& direction, float speed, float damage, float lifetime, BossScript* boss)
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
        m_isCrystallized = false;
        m_canBeCrystallized = true;

        if (boss)
        {
            m_boss = engine::Ptr<BossScript>(boss);
        }
    }

    void BossProjectile::OnCrystallized()
    {
        if (!m_canBeCrystallized || m_isCrystallized) return;

        m_isCrystallized = true;
        // 이동 정지 (Update에서 처리됨)

        // TODO: 결정화 이펙트 표시
        // 예: 색상 변경, 파티클 이펙트 등
    }

    void BossProjectile::OnExecutionReflected(const engine::Vector3& direction)
    {
        if (!m_isCrystallized) return;  // 결정화된 상태에서만 반사 가능

        // 방향 설정 (보스 방향으로)
        float length = direction.Length();
        if (length > 0.001f)
        {
            m_direction = direction / length;
        }

        // 결정화 해제 및 이동 재개
        m_isCrystallized = false;
        m_canBeCrystallized = false;  // 한 번 반사되면 다시 결정화 불가

        // 데미지 증가 (반사된 투사체는 큰 데미지)
        m_damage = 200.0f;

        // 수명 연장 (보스까지 도달할 시간 확보)
        m_lifetime = m_elapsedTime + 10.0f;

        // TODO: 반사 이펙트 표시
    }

    void BossProjectile::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        if (m_isDestroyed) return;

        if (!info.gameObject) return;

        // 플레이어와 충돌
        auto* player = info.gameObject->GetComponent<PlayerControllerScript>();
        if (player)
        {
            // TODO: 플레이어에게 데미지 주기
            // player->TakeDamage(static_cast<int>(m_damage));

            // 투사체 파괴
            if (GetGameObject())
            {
                GetGameObject()->Destroy();
            }
            m_isDestroyed = true;
            return;
        }

        // 보스와 충돌 (반사된 투사체만)
        if (m_boss && info.gameObject == m_boss->GetGameObject())
        {
            // 보스에게 큰 데미지
            m_boss->OnExecutionReflected(m_direction);

            // 투사체 파괴
            if (GetGameObject())
            {
                GetGameObject()->Destroy();
            }
            m_isDestroyed = true;
            return;
        }
    }
}
