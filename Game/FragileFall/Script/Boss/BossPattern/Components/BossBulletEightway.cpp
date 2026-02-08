#include "GamePCH.h"
#include "Script/Boss/BossPattern/Components/BossBulletEightway.h"
#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/CharacterScript/Common/BulletMovement.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Physics/PhysicsLayer.h>
#include <Framework/Asset/Prefab.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 초기화 (Factory에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void BossBulletEightway::Setup(std::unique_ptr<IBulletMovement> movement, const BulletParams& params, BulletFactory* factory)
    {
        m_movement = std::move(movement);
        m_params = params;
        m_cachedFactory = factory;
        m_lifetime = params.lifetime;
    }

    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void BossBulletEightway::Start()
    {
        m_elapsedTime = 0.0f;

        // ─────────────────────────────────────────────
        // 총알 스케일 적용 (균등 스케일)
        // ─────────────────────────────────────────────
        engine::Vector3 scaleVec(m_params.scale, m_params.scale, m_params.scale);
        GetTransform()->SetLocalScale(scaleVec);

        // 콜라이더가 Transform 스케일과 동기화되도록 설정
        auto* collider = GetGameObject()->GetComponent<engine::Collider>();
        if (collider)
        {
            collider->SetSyncWithTransform(true);
            collider->CheckAndSyncTransformScale();
        }

        auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>();
        if (!rb) return;

        // Rigidbody 깨우기
        rb->WakeUp();

        // Linear 타입: 글로벌 중력 OFF
        rb->SetUseGravity(false);
        rb->SetLinearDamping(0.0f);
        rb->SetAngularDamping(0.0f);

        // Rigidbody에 초기 속도 설정
        if (!m_movement) return;

        rb->SetLinearVelocity(m_movement->GetVelocity());
        rb->WakeUp();
    }

    void BossBulletEightway::Update()
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

        // Movement 업데이트
        if (m_movement)
        {
            m_movement->Update(GetTransform(), dt);
        }

        // 수명 체크
        if (m_elapsedTime >= m_lifetime)
        {
            GetGameObject()->Destroy();
            return;
        }

        // 화면 밖 체크 (XZ 범위)
        engine::Vector3 pos = GetTransform()->GetWorldPosition();
        float boundary = 50.0f;
        if (std::abs(pos.x) > boundary || std::abs(pos.z) > boundary)
        {
            GetGameObject()->Destroy();
            return;
        }
    }

    void BossBulletEightway::FixedUpdate()
    {
        // ─────────────────────────────────────────────
        // Movement의 FixedUpdate 호출 (물리 연산)
        // ─────────────────────────────────────────────
        if (m_movement)
        {
            m_movement->FixedUpdate();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백
    // ═══════════════════════════════════════════════════════════════
    void BossBulletEightway::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        if (m_isDying) return;
        if (!info.gameObject) return;

        auto* collider = info.gameObject->GetComponent<engine::Collider>();
        if (!collider) return;
        
        auto layer = collider->GetLayer();

        // Player와만 충돌
        if (layer == engine::PhysicsLayer::Player)
        {
            // 플레이어에게 데미지 적용
            auto* player = info.gameObject->GetComponent<PlayerControllerScript>();
            if (player)
            {
                player->TakeDamage(m_params.damage);
            }

            // 총알 파괴
            m_isDying = true;
            m_deathTimer = 0.0f;

            if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
            {
                rb->SetLinearVelocity(engine::Vector3::Zero);
            }

            // 파괴 이펙트
            auto effect = engine::Prefab::Instantiate("Effect_Bullet_Destory_V1.00");
            if (effect && effect->GetTransform())
            {
                effect->GetTransform()->SetWorldMatrix(GetTransform()->GetWorld());
                effect->GetTransform()->SetLocalScale(engine::Vector3(1.0f, 1.0f, 1.0f));
            }
        }
    }
}
