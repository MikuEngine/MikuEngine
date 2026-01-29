#include "GamePCH.h"
#include "BossProjectile.h"

#include <Framework/Object/Component/Collider.h>
#include <Framework/Asset/Prefab.h>

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
            GetGameObject()->Destroy();

            if (!m_pillarCrystalizedPieces.empty())
            {
                for (auto& e : m_pillarCrystalizedPieces)
                {
                    if (e)
                    {
                        e->Destroy();
                    }
                }
            }

            m_isDestroyed = true;
            return;
        }

        // 결정화 상태가 아니면 이동
        if (!m_isCrystallized || m_isReflecting)
        {
            auto* transform = GetGameObject()->GetTransform();
            if (transform)
            {
                engine::Vector3 currentPos = transform->GetWorldPosition();
                engine::Vector3 newPos = currentPos + m_direction * m_speed * deltaTime;
                transform->SetLocalPosition(newPos);

                if (m_isReflecting && !m_pillarCrystalizedPieces.empty())
                {
                    for (auto& e : m_pillarCrystalizedPieces)
                    {
                        if (e)
                        {
                            e->GetTransform()->SetLocalPosition(newPos);
                        }
                        
                    }
                }

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

        for (int i = 0; i < 10; ++i)
        {
            auto go = engine::Prefab::Instantiate("PillarCrystalizedPiece");

            auto pos = GetTransform()->GetLocalPosition();

            float rotY = engine::Random::Float(0.0f, 360.f);
            float rotX = engine::Random::Float(30.0f, 60.0f);

            go->GetTransform()->SetLocalPosition(pos);
            go->GetTransform()->SetLocalRotation(engine::Vector3(rotX, rotY, 0.0f));

            m_pillarCrystalizedPieces.push_back(go);
        }
    }

    void BossProjectile::OnExecutionReflected(const engine::Vector3& direction)
    {
        if (!m_isCrystallized) return;  // 결정화된 상태에서만 반사 가능

        float length = direction.Length();
        if (length > 0.001f)
        {
            m_direction = direction / length;
        }

        //  이동 재개
        m_canBeCrystallized = false;  // 한 번 반사되면 다시 결정화 불가
        m_isReflecting = true;

        // 데미지 증가 (반사된 투사체는 큰 데미지)
        m_damage = 200.0f;

        // 수명 연장 (보스까지 도달할 시간 확보)
        m_lifetime = m_elapsedTime + 10.0f;

        // TODO: 반사 이펙트 표시
    }

    void BossProjectile::Execute()
    {
        if (!m_isCrystallized) return;  // 결정화된 상태에서만 처형 가능

        // 플레이어 위치 가져오기
        engine::Vector3 playerPos;
        bool hasPlayer = false;

        if (m_boss)
        {
            auto player = m_boss->GetTargetPlayer();
            if (player)
            {
                auto playerTr = player->GetTransform();
                if (playerTr)
                {
                    playerPos = playerTr->GetWorldPosition();
                    hasPlayer = true;
                }
            }
        }

        // 구체 위치
        engine::Vector3 projectilePos = GetTransform()->GetWorldPosition();

        // 플레이어->구체 방향 계산
        engine::Vector3 direction = projectilePos - playerPos;
        float length = direction.Length();
        if (length > 0.001f && hasPlayer)
        {
            direction = direction / length;
        }
        else
        {
            // 플레이어를 찾을 수 없으면 보스 방향으로 (기존 로직)
            if (m_boss && m_boss->GetGameObject())
            {
                engine::Vector3 bossPos = m_boss->GetGameObject()->GetTransform()->GetWorldPosition();
                direction = bossPos - projectilePos;
                length = direction.Length();
                if (length > 0.001f)
                {
                    direction = direction / length;
                }
                else
                {
                    direction = engine::Vector3(0.0f, 0.0f, 1.0f);  // 기본 방향
                }
            }
            else
            {
                direction = engine::Vector3(0.0f, 0.0f, 1.0f);  // 기본 방향
            }
        }

        // 반사 처리
        OnExecutionReflected(direction);
    }

    void BossProjectile::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        if (m_isDestroyed) return;

        if (!info.gameObject) return;

        if (!m_isCrystallized)
        {// 플레이어와 충돌
            auto* player = info.gameObject->GetComponent<PlayerControllerScript>();
            if (player)
            {
                // TODO: 플레이어에게 데미지 주기
                // player->TakeDamage(static_cast<int>(m_damage));

                GetGameObject()->Destroy();

                m_isDestroyed = true;
                return;
            }
        }

        // 보스와 충돌 (반사된 투사체만)
        if (m_isReflecting)
        {
            if (auto boss = info.gameObject->GetComponent<BossScript>())
            {
                // 보스에게 큰 데미지
                boss->OnExecutionReflected(m_direction);

                GetGameObject()->Destroy();

                if (!m_pillarCrystalizedPieces.empty())
                {
                    for (auto& e : m_pillarCrystalizedPieces)
                    {
                        if (e)
                        {
                            e->Destroy();
                        }
                    }
                }

                m_isDestroyed = true;
                return;
            }
        }
    }
}
