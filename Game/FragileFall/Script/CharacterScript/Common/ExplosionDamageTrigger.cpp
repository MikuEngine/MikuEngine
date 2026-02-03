#include "GamePCH.h"
#include "Script/CharacterScript/Common/ExplosionDamageTrigger.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Physics/PhysicsLayer.h>

namespace game
{
    void ExplosionDamageTrigger::Setup(int damage, float lifetime)
    {
        m_damage = damage;
        m_lifetime = lifetime;
    }

    void ExplosionDamageTrigger::Start()
    {
        m_elapsedTime = 0.0f;
        m_hasDamaged = false;
        
        // Collider 레이어 설정
        if (auto* collider = GetGameObject()->GetComponent<engine::Collider>())
        {
            collider->SetLayer(engine::PhysicsLayer::ExplosionTrigger);
            collider->SetIsTrigger(true);
        }
    }

    void ExplosionDamageTrigger::Update()
    {
        m_elapsedTime += engine::Time::DeltaTime();
        
        // 생존 시간 초과 시 파괴
        if (m_elapsedTime >= m_lifetime)
        {
            GetGameObject()->Destroy();
        }
    }

    void ExplosionDamageTrigger::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        if (m_hasDamaged) return;
        if (!info.gameObject) return;
        
        // 플레이어와 충돌 시 데미지 적용
        if (auto* playerScript = info.gameObject->GetComponent<PlayerControllerScript>())
        {
            // TODO: 플레이어 OnHit() 구현 시 활성화
            // playerScript->OnHit(m_damage);
            
            m_hasDamaged = true;
        }
    }
}
