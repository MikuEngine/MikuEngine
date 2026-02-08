#include "GamePCH.h"
#include "Script/Boss/BossPattern/Components/BossMeteor.h"
#include "Script/Boss/BossScript.h"
#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/CharacterScript/Common/BulletParams.h"
#include "Script/CharacterScript/Common/ExplosionDamageTrigger.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Physics/PhysicsLayer.h>
#include <Framework/Asset/Prefab.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 초기화 (패턴에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void BossMeteor::Setup(BossScript* boss, 
                           const engine::Vector3& targetLandingPos,
                           engine::GameObject* warningGO)
    {
        m_boss = engine::Ptr<BossScript>(boss);
        m_targetLandingPos = targetLandingPos;
        m_warningGO = engine::Ptr<engine::GameObject>(warningGO);
        m_hasExploded = false;
    }

    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void BossMeteor::Start()
    {
        m_rigidbody = GetGameObject()->GetComponent<engine::Rigidbody>();
        if (!m_rigidbody) return;

        // ─────────────────────────────────────────────
        // Rigidbody 설정
        // ─────────────────────────────────────────────
        m_rigidbody->WakeUp();
        m_rigidbody->SetUseGravity(false);     // PhysX 글로벌 중력 OFF
        m_rigidbody->SetLinearDamping(0.0f);
        m_rigidbody->SetAngularDamping(999.0f); // 회전 억제
        
        // 회전 고정
        engine::RigidbodyConstraints constraints = engine::RigidbodyConstraints::FreezeRotation;
        m_rigidbody->SetConstraints(constraints);

        // ─────────────────────────────────────────────
        // 초기 하방 속도 설정
        // ─────────────────────────────────────────────
        if (m_boss)
        {
            float initialSpeed = m_boss->GetMeteorInitialSpeed();
            m_rigidbody->SetLinearVelocity(engine::Vector3(0.0f, -initialSpeed, 0.0f));
            m_rigidbody->WakeUp();
        }

        // ─────────────────────────────────────────────
        // 메테오 스케일 적용
        // ─────────────────────────────────────────────
        if (m_boss)
        {
            float scale = m_boss->GetMeteorScale();
            engine::Vector3 scaleVec(scale, scale, scale);
            GetTransform()->SetLocalScale(scaleVec);

            // 콜라이더가 Transform 스케일과 동기화되도록 설정
            auto* collider = GetGameObject()->GetComponent<engine::Collider>();
            if (collider)
            {
                collider->SetSyncWithTransform(true);
                collider->CheckAndSyncTransformScale();
            }
        }
    }

    void BossMeteor::Update()
    {
        if (m_hasExploded) return;
        if (!m_boss) return;

        // ─────────────────────────────────────────────
        // Y 좌표 착지 판정
        // ─────────────────────────────────────────────
        engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
        float landingY = m_boss->GetMeteorLandingY();
        float threshold = m_boss->GetMeteorLandingThreshold();

        // landingY + threshold 이하로 떨어지면 착지
        if (currentPos.y <= landingY + threshold)
        {
            Explode();
        }
    }

    void BossMeteor::FixedUpdate()
    {
        if (m_hasExploded) return;
        if (!m_rigidbody) return;
        if (!m_boss) return;

        // ─────────────────────────────────────────────
        // 자체 중력 적용
        // ─────────────────────────────────────────────
        float ownGravity = m_boss->GetMeteorOwnGravity();
        if (ownGravity > 0.0f)
        {
            engine::Vector3 gravityForce(0.0f, -ownGravity, 0.0f);
            m_rigidbody->AddForce(gravityForce, engine::ForceMode::Acceleration);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백
    // ═══════════════════════════════════════════════════════════════
    void BossMeteor::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        if (m_hasExploded) return;
        if (!info.gameObject) return;

        auto* collider = info.gameObject->GetComponent<engine::Collider>();
        if (!collider) return;

        // Player와 충돌 시 즉시 폭발
        if (collider->GetLayer() == engine::PhysicsLayer::Player)
        {
            Explode();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 폭발 처리
    // ═══════════════════════════════════════════════════════════════
    void BossMeteor::Explode()
    {
        if (m_hasExploded) return;
        m_hasExploded = true;

        if (!m_boss) return;

        // BulletFactory 가져오기
        auto bulletFactory = m_boss->GetBulletFactory();
        if (!bulletFactory) return;

        // 폭발 위치 (현재 메테오 XZ, Y=0)
        engine::Vector3 explosionPos = GetTransform()->GetWorldPosition();
        explosionPos.y = 0.0f;

        // ─────────────────────────────────────────────
        // 1. ExplosionDamageTrigger 생성
        // ─────────────────────────────────────────────
        auto explosionGO = engine::Prefab::Instantiate("ExplosionDamageTrigger");
        if (explosionGO)
        {
            explosionGO->GetTransform()->SetLocalPosition(explosionPos);
            
            auto* explosionScript = explosionGO->GetComponent<ExplosionDamageTrigger>();
            if (explosionScript)
            {
                float damage = m_boss->GetMeteorExplosionDamage();
                float radius = m_boss->GetMeteorExplosionRadius();
                float lifetime = m_boss->GetMeteorExplosionLifetime();
                explosionScript->Setup(damage, radius, lifetime);
            }
        }

        // ─────────────────────────────────────────────
        // 2. 8방향 총알 발사 (Y=1.5)
        // ─────────────────────────────────────────────
        BulletParams bulletParams;
        bulletParams.type = BulletType::Linear;
        bulletParams.speed = m_boss->GetMeteorBulletSpeed();
        bulletParams.damage = m_boss->GetMeteorBulletDamage();
        bulletParams.lifetime = m_boss->GetMeteorBulletLifetime();
        bulletParams.scale = m_boss->GetMeteorBulletScale();

        bulletFactory->EightwayFireBossMeteor(explosionPos, bulletParams);

        // ─────────────────────────────────────────────
        // 3. 메테오 파괴
        // ─────────────────────────────────────────────
        GetGameObject()->Destroy();

        // ─────────────────────────────────────────────
        // 4. 경고 프리팹 파괴
        // ─────────────────────────────────────────────
        if (m_warningGO && m_warningGO.Get())
        {
            m_warningGO->Destroy();
        }
    }
}
