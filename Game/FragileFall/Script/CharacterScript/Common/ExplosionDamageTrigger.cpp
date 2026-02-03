#include "GamePCH.h"
#include "Script/CharacterScript/Common/ExplosionDamageTrigger.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Object/Component/SphereCollider.h>
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
        
        // SphereCollider 캐시 및 레이어 설정
        m_sphereCollider = GetGameObject()->GetComponent<engine::SphereCollider>();
        if (m_sphereCollider)
        {
            m_sphereCollider->SetLayer(engine::PhysicsLayer::ExplosionTrigger);
            m_sphereCollider->SetIsTrigger(true);
            m_sphereCollider->SetRadius(m_startRadius);
        }
        
        // 초기 스케일 설정
        GetTransform()->SetLocalScale(engine::Vector3(m_startScale, m_startScale, m_startScale));
    }

    void ExplosionDamageTrigger::Update()
    {
        m_elapsedTime += engine::Time::DeltaTime();
        
        // 확장 애니메이션
        UpdateExpansion();
        
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
            playerScript->TakeDamage(m_damage);
            
            m_hasDamaged = true;
        }
    }

    void ExplosionDamageTrigger::UpdateExpansion()
    {
        if (m_elapsedTime >= m_expandDuration) return;
        
        // 0 ~ 1 보간 비율
        float t = m_elapsedTime / m_expandDuration;
        
        // 스케일 보간
        float currentScale = m_startScale + (m_endScale - m_startScale) * t;
        GetTransform()->SetLocalScale(engine::Vector3(currentScale, currentScale, currentScale));
              
    }
}
