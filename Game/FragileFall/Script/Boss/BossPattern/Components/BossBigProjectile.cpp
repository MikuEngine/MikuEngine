#include "GamePCH.h"
#include "BossBigProjectile.h"

#include <Framework/Object/Component/Collider.h>
#include <Framework/Asset/Prefab.h>

#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/Boss/BossScript.h"

namespace game
{
    void BossBigProjectile::Awake()
    {
    }

    void BossBigProjectile::Start()
    {
    }

    void BossBigProjectile::Update()
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

        // 결정화 상태가 아니면 이동 OR 반사 중이면 이동
        if (!m_isCrystallized || m_isReflecting)
        {
            auto* transform = GetGameObject()->GetTransform();
            if (transform)
            {
                engine::Vector3 currentPos = transform->GetWorldPosition();
                engine::Vector3 newPos = currentPos + m_direction * m_speed * deltaTime;
                transform->SetLocalPosition(newPos);

                // 반사 중이면 결정화 이펙트도 같이 이동
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

    void BossBigProjectile::Setup(const engine::Vector3& direction, BossScript* boss)
    {
        if (!boss) return;

        // 보스 참조 저장
        m_boss = engine::Ptr<BossScript>(boss);

        // ─────────────────────────────────────────────
        // BossScript에서 모든 설정 가져오기
        // ─────────────────────────────────────────────
        m_Hp = boss->GetBigProjectileHP();
        m_speed = boss->GetBigProjectileSpeed();
        m_damage = boss->GetBigProjectileDamage();
        m_lifetime = boss->GetBigProjectileLifetime();
        m_reflectDamage = boss->GetBigProjectileReflectDamage();
        m_reflectLifetimeExtension = boss->GetBigProjectileReflectLifetimeExtension();

        // ─────────────────────────────────────────────
        // 방향 정규화
        // ─────────────────────────────────────────────
        float length = direction.Length();
        if (length > 0.001f)
        {
            m_direction = direction / length;
        }
        else
        {
            m_direction = engine::Vector3(0.0f, 0.0f, 1.0f);  // 기본 방향
        }

        // ─────────────────────────────────────────────
        // 스케일 적용
        // ─────────────────────────────────────────────
        float scale = boss->GetBigProjectileScale();
        GetTransform()->SetLocalScale(engine::Vector3(scale, scale, scale));

        // 콜라이더가 Transform 스케일과 동기화되도록 설정
        auto* collider = GetGameObject()->GetComponent<engine::Collider>();
        if (collider)
        {
            collider->SetSyncWithTransform(true);
            collider->CheckAndSyncTransformScale();
        }

        // ─────────────────────────────────────────────
        // 상태 초기화
        // ─────────────────────────────────────────────
        m_elapsedTime = 0.0f;
        m_isDestroyed = false;
        m_isCrystallized = false;
        m_canBeCrystallized = true;
        m_isReflecting = false;
    }

    void BossBigProjectile::OnCrystallized()
    {
        // 결정화 시각 효과 생성 (PillarCrystalizedPiece 10개)
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

    void BossBigProjectile::OnExecutionReflected(const engine::Vector3& direction)
    {
        if (!m_isCrystallized) return;  // 결정화된 상태에서만 반사 가능

        float length = direction.Length();
        if (length > 0.001f)
        {
            m_direction = direction / length;
        }

        // 이동 재개
        m_canBeCrystallized = false;  // 한 번 반사되면 다시 결정화 불가
        m_isReflecting = true;

        // 수명 연장 (Setup에서 설정된 값 사용)
        m_lifetime = m_elapsedTime + m_reflectLifetimeExtension;

        // ─────────────────────────────────────────────
        // 콜라이더 레이어 변경 (BossBigProjectile → BBP_Reflected)
        // ─────────────────────────────────────────────
        auto* collider = GetGameObject()->GetComponent<engine::Collider>();
        if (collider)
        {
            collider->SetLayer(engine::PhysicsLayer::BBP_Reflected);
        }

        // TODO: 반사 이펙트 표시
    }

    void BossBigProjectile::Execute()
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
        
        // Y 성분 제거 (XZ 평면 방향만 사용)
        direction.y = 0.0f;
        
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
                
                // Y 성분 제거
                direction.y = 0.0f;
                
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

    void BossBigProjectile::TakeDamage(float damage, bool isShieldPierce)
    {
        // 이미 결정화되었으면 추가 데미지 무시
        if (m_isCrystallized) return;

        m_Hp -= damage;

        // 체력 0 이하 → 결정화 (false→true 전환 시에만 OnCrystallized 호출)
        if (m_Hp <= 0.0f)
        {
            m_Hp = 0.0f;
            m_isCrystallized = true;
            OnCrystallized();
        }
    }

    void BossBigProjectile::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        if (m_isDestroyed) return;
        if (!info.gameObject) return;

        // 결정화되지 않았을 때
        if (!m_isCrystallized)
        {
            // 플레이어와 충돌
            auto* player = info.gameObject->GetComponent<PlayerControllerScript>();
            if (player)
            {
                // TODO: 플레이어에게 데미지 주기
                // player->TakeDamage(m_damage);

                GetGameObject()->Destroy();

                m_isDestroyed = true;
                return;
            }
        }

        // 반사 중일 때 보스와 충돌
        if (m_isReflecting)
        {
            if (auto* boss = info.gameObject->GetComponent<BossScript>())
            {
                // 보스에게 반사 데미지 (실드 감쇄 적용)
                boss->OnReflectedProjectileHit(m_direction, m_reflectDamage);

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
