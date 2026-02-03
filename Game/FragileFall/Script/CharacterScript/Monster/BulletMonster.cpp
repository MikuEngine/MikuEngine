#include "GamePCH.h"
#include "Script/CharacterScript/Monster/BulletMonster.h"
#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/CharacterScript/Common/BulletMovement.h"
#include "Script/CharacterScript/Common/ExplosionDamageTrigger.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Physics/PhysicsLayer.h>

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>

#include <Framework/Asset/Prefab.h>

#include <Framework/Object/Component/Renderer/DebugRenderer.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 초기화 (Factory에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void BulletMonster::Setup(std::unique_ptr<IBulletMovement> movement, const BulletParams& params, BulletFactory* factory)
    {
        m_movement = std::move(movement);
        m_params = params;
        m_cachedFactory = factory;
        
        // 포물선 타입은 lifetime 10초 (충돌로 사라지지 않는 경우 대비)
        if (m_params.type == BulletType::Parabolic)
        {
            m_lifetime = 10.0f;
        }
        else
        {
            m_lifetime = params.lifetime;
        }
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

        auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>();
        if (!rb) return;
        
        // Rigidbody 깨우기 (Sleep 상태 해제)
        rb->WakeUp();
        
        // Parabolic/Field 타입: 자체 중력 사용 (PhysX 글로벌 중력 OFF)
        if (m_params.type == BulletType::Parabolic || m_params.type == BulletType::Field)
        {
            rb->SetUseGravity(false);
            rb->SetLinearDamping(0.0f);
            rb->SetAngularDamping(0.0f);
        }

        // Rigidbody에 초기 속도 설정
        if (!m_movement) return;
        
        rb->SetLinearVelocity(m_movement->GetVelocity());
        rb->WakeUp();
    }

    void BulletMonster::Update()
    {
        // 죽는 중이면 타이머만 체크
        if (m_isDying)
        {
            m_deathTimer += engine::Time::DeltaTime();
            if (m_deathTimer >= m_deathDelay)
            {
                // 폭발 트리거 생성 (착탄 시에만)
                if (m_shouldSpawnExplosion)
                {
                    SpawnExplosionTrigger(m_impactPoint);
                }
                GetGameObject()->Destroy();
            }
            return;
        }

        // 생존 시간 누적
        float dt = engine::Time::DeltaTime();
        m_elapsedTime += dt;

        // Movement 업데이트
        if (m_movement && !m_isFieldType)
        {
            m_movement->Update(GetTransform(), dt);
        }

        // 수명 체크 (lifetime 만료 시 폭발 트리거 생성 안 함)
        if (m_elapsedTime >= m_lifetime)
        {
            GetGameObject()->Destroy();
            return;
        }

        engine::Vector3 pos = GetTransform()->GetWorldPosition();

        // ═══════════════════════════════════════════════════════════════
        // 포물선 타입 전용: Y좌표 기반 착탄
        // Y <= -0.1 도달 시 착탄점에서 폭발 트리거 생성
        // ═══════════════════════════════════════════════════════════════
        if (m_params.type == BulletType::Parabolic && !m_isDying)
        {
            constexpr float kGroundY = -0.1f;
            if (pos.y <= kGroundY)
            {
                DieWithExplosion(pos);
                return;
            }
        }

        // Field 타입: Y축 높이가 바닥(0) 근처로 떨어졌는지 체크
        if (!m_isDying && !m_isFieldType && m_params.type == BulletType::Field)
        {
            if (pos.y <= 0.1f)
            {
                if (m_cachedFactory)
                {
                    engine::Vector3 spawnPos = pos;
                    spawnPos.y = 0.0f;
                    m_cachedFactory->FieldFireMonster(spawnPos, m_params);
                }

                m_isDying = true;
                m_deathTimer = 0.0f;
                m_shouldSpawnExplosion = false;

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
#ifdef _DEBUG
            engine::DebugRenderer::Get().AddDebugCircle(
                pos + engine::Vector3(0, 0.05f, 0),
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
                    float distance = engine::Vector3::Distance(pos, m_targetPlayer->GetTransform()->GetWorldPosition());
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

        // 화면 밖 체크 (XZ 범위) - 폭발 트리거 생성 안 함
        float boundary = 50.0f;
        if (std::abs(pos.x) > boundary || std::abs(pos.z) > boundary)
        {
            GetGameObject()->Destroy();
            return;
        }
    }

    void BulletMonster::FixedUpdate()
    {
        // ─────────────────────────────────────────────
        // Movement의 FixedUpdate 호출 (물리 연산)
        // PhysX simulate()와 동기화됨
        // ─────────────────────────────────────────────
        if (m_movement)
        {
            m_movement->FixedUpdate();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백
    // ═══════════════════════════════════════════════════════════════
    void BulletMonster::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        if (m_isDying || m_isFieldType) return;
        if (!info.gameObject) return;

        // 포물선 타입: 플레이어 충돌 시 착탄점에서 폭발
        if (m_params.type == BulletType::Parabolic)
        {
            if (info.gameObject->GetComponent<PlayerControllerScript>())
            {
                // 충돌 지점을 착탄점으로 설정 (없으면 탄환 위치)
                engine::Vector3 impactPoint = GetTransform()->GetWorldPosition();
                if (!info.contacts.empty())
                {
                    impactPoint = info.contacts[0].point;
                }
                DieWithExplosion(impactPoint);
            }
            return;
        }

        // 일반 타입: 플레이어와 충돌
        if (info.gameObject->GetComponent<PlayerControllerScript>())
        {
            m_isDying = true;
            m_deathTimer = 0.0f;
            m_shouldSpawnExplosion = false;

            if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
            {
                rb->SetLinearVelocity(engine::Vector3::Zero);
            }
        }
    }

    void BulletMonster::OnCollisionEnter(const engine::CollisionInfo& info)
    {
        if (m_isDying || m_isFieldType) return;
        if (!info.gameObject) return;

        // 포물선 타입: 벽 또는 플레이어 충돌 시 착탄
        if (m_params.type == BulletType::Parabolic)
        {
            auto* collider = info.gameObject->GetComponent<engine::Collider>();
            if (!collider) return;

            uint32_t layer = collider->GetLayer();
            
            // Wall 또는 Player와 충돌
            if (layer == engine::PhysicsLayer::Wall || layer == engine::PhysicsLayer::Player)
            {
                // 충돌 지점을 착탄점으로 설정 (없으면 탄환 위치)
                engine::Vector3 impactPoint = GetTransform()->GetWorldPosition();
                if (!info.contacts.empty())
                {
                    impactPoint = info.contacts[0].point;
                }
                DieWithExplosion(impactPoint);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 포물선 탄환 전용 헬퍼
    // ═══════════════════════════════════════════════════════════════
    void BulletMonster::SpawnExplosionTrigger(const engine::Vector3& position)
    {
        auto* scene = engine::SceneManager::Get().GetScene();
        if (!scene) return;

        // ExplosionDamageTrigger 프리팹 로드 및 인스턴시에이트
        auto explosionGO = engine::Prefab::Instantiate("ExplosionDamageTrigger");

        if (!explosionGO) return;

        // 위치 설정
        explosionGO->GetTransform()->SetLocalPosition(position);       

        // ExplosionDamageTrigger 스크립트 설정
        if (auto* explosionScript = explosionGO->GetComponent<ExplosionDamageTrigger>())
        {
            explosionScript->Setup(m_params.damage, 3.0f);
        }
    }

    void BulletMonster::DieWithExplosion(const engine::Vector3& impactPoint)
    {
        m_isDying = true;
        m_deathTimer = 0.0f;
        m_shouldSpawnExplosion = true;
        m_impactPoint = impactPoint;

        // 속도 정지
        if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
        {
            rb->SetLinearVelocity(engine::Vector3::Zero);
        }
    }
}
