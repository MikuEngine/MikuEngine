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

#include <Framework/Object/Component/Renderer/DebugRenderer.h>

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
     
        auto* scene = engine::SceneManager::Get().GetScene();
        if (scene)
        {
            m_targetPlayer = scene->FindGameObject("Player");
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void BulletMonster::Start()
    {
        m_elapsedTime = 0.0f;

        if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
        {
            // ─────────────────────────────────────────────
            // Parabolic/Field 타입: 자체 중력 사용 (PhysX 글로벌 중력 OFF)
            // ─────────────────────────────────────────────
            if (m_params.type == BulletType::Parabolic || m_params.type == BulletType::Field)
            {
                rb->SetUseGravity(false);
            }

            // Rigidbody에 초기 속도 설정
            if (m_movement)
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

        // ─────────────────────────────────────────────
        // Movement 업데이트
        // - UsesPhysics() == true: AddForce 적용 (ParabolicMovement 등)
        // - UsesPhysics() == false: Transform 직접 조작
        // ─────────────────────────────────────────────
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

        // Y축 높이가 바닥(0) 근처로 떨어졌는지 체크
        if (!m_isDying && !m_isFieldType)
        {
            if (GetTransform()->GetWorldPosition().y <= 0.1f)
            {
                if (m_params.type == BulletType::Field && m_cachedFactory)
                {
                    engine::Vector3 spawnPos = GetTransform()->GetWorldPosition();
                    spawnPos.y = 0.0f;

                    m_cachedFactory->FieldFireMonster(spawnPos, m_params);
                }

                m_isDying = true;
                m_deathTimer = 0.0f;

                if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
                {
                    rb->SetLinearVelocity(engine::Vector3::Zero);
                }

                return; 
            }
        }
        
        // 장판형 총알일 경우 주기적으로 데미지 적용
        if (m_isFieldType)
        {

#ifdef _DEBUG  // 장판 범위 디버그 렌더링
            engine::DebugRenderer::Get().AddDebugCircle(
                GetTransform()->GetWorldPosition() + engine::Vector3(0, 0.05f, 0),
                m_radius,
                engine::Vector3::UnitY,
                DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.2f),
                32
            );
#endif

            m_tickTimer += engine::Time::DeltaTime();

            if (m_tickTimer >= m_tickInterval)
            {
                m_tickTimer = 0.0f;

                if (m_targetPlayer)
                {
                    float distance = engine::Vector3::Distance(GetTransform()->GetWorldPosition(), m_targetPlayer->GetTransform()->GetWorldPosition());

                    if (distance <= m_params.radius)
                    {
                        if (auto* playerScript = m_targetPlayer->GetComponent<PlayerControllerScript>())
                        {
                            // playerScript->OnHit(m_params.damage);
                        }
                    }
                }  
            }
            return;
        }

        // ─────────────────────────────────────────────
        // 소멸 체크
        // ─────────────────────────────────────────────
        engine::Vector3 pos = GetTransform()->GetWorldPosition();

        // Y좌표 기반 소멸 (바닥 아래로 떨어지면 제거)
        constexpr float kGroundY = -1.0f;
        if (pos.y < kGroundY)
        {
            GetGameObject()->Destroy();
            return;
        }

        // 화면 밖 체크 (XZ 범위)
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

        // 플레이어와 충돌했는지 확인
        if (info.gameObject->GetComponent<PlayerControllerScript>())
        {
            // TODO: 플레이어 OnHit() 구현 시 호출
            // player->OnHit(m_damage);

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
