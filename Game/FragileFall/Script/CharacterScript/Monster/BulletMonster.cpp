#include "GamePCH.h"
#include "Script/CharacterScript/Monster/BulletMonster.h"
#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/CharacterScript/Common/BulletMovement.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Collider.h>

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>


namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 초기화 (Factory에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void BulletMonster::Setup(std::unique_ptr<IBulletMovement> movement, const BulletParams& params, BulletFactory* factory)
    {
        m_movement = std::move(movement);
        m_lifetime = params.lifetime;
		m_params = params;
        m_cachedFactory = factory;
    }

    void BulletMonster::SetupField(float radius, const BulletParams& params)
    {
        m_isFieldType = true;
        m_radius = radius;
        m_lifetime = params.lifetime;
        m_params = params;

        auto* collider = GetGameObject()->GetComponent<engine::Collider>();
        if (collider)
        {
            collider->SetLayer(engine::PhysicsLayer::Field);
            collider->SetIsTrigger(true);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void BulletMonster::Start()
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

    void BulletMonster::Update()
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

        if (m_movement && !m_isFieldType)
        {
            m_movement->Update(GetTransform(), dt);
        }

        // 수명 체크
        if (m_elapsedTime >= m_lifetime)
        {
            GetGameObject()->Destroy();
            return;
        }

		// 장판형 총알일 경우 주기적으로 데미지 적용
        if (m_isFieldType)
        {
            m_tickTimer += engine::Time::DeltaTime();

            if (m_tickTimer >= m_tickInterval)
            {
                m_tickTimer = 0.0f;

                if (auto* scene = engine::SceneManager::Get().GetScene())
                {
                    auto* playerGO = scene->FindGameObject("Player");
                    if (playerGO)
                    {
                        float distance = engine::Vector3::Distance(
                            GetTransform()->GetWorldPosition(),
                            playerGO->GetTransform()->GetWorldPosition()
                        );

                        if (distance <= m_params.radius)
                        {
                            if (auto* playerScript = playerGO->GetComponent<PlayerControllerScript>())
                            {
                                // playerScript->OnHit(m_params.damage); 
                                // LOG_PRINT("Field Damage Dealt to Player!");
                            }
                        }
                    }
                }
            }
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
    // 충돌 콜백
    // ═══════════════════════════════════════════════════════════════
    void BulletMonster::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        if (m_isDying || m_isFieldType) return;
        if (!info.gameObject) return;

        bool isPlayer = (info.gameObject->GetComponent<PlayerControllerScript>() != nullptr);
        bool isEnvironment = (info.gameObject->GetComponent<engine::Collider>()->GetLayer() == engine::PhysicsLayer::Environment);

        // 플레이어와 충돌했는지 확인
        if (isPlayer || isEnvironment)
        {
            // TODO: 플레이어 OnHit() 구현 시 호출
            // player->OnHit(m_damage);

            if (m_params.type == BulletType::Field && m_cachedFactory)
            {
                m_cachedFactory->FieldFireMonster(GetTransform()->GetWorldPosition(), m_params);
            }

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
