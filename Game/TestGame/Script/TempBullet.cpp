#include "GamePCH.h"
#include "TempBullet.h"
#include "TempMonster.h"

#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Physics/CollisionSystem.h>

namespace game
{
    void TempBullet::Initialize(const engine::Vector3& direction, float speed, float lifetime)
    {
        m_direction = direction;
        m_direction.Normalize();
        m_speed = speed;
        m_lifetime = lifetime;
    }

    void TempBullet::Start()
    {
        m_elapsedTime = 0.0f;
        
        // Rigidbody에 초기 속도 설정
        if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
        {
            rb->SetLinearVelocity(m_direction * m_speed);
        }
    }

    void TempBullet::Update()
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
        m_elapsedTime += engine::Time::DeltaTime();
        
        // 수명 체크
        if (m_elapsedTime >= m_lifetime)
        {
            GetGameObject()->Destroy();
            return;
        }

        // ═══════════════════════════════════════════════════════════════
        // [PULL 방식 - 참고용 주석]
        // 특수한 경우 직접 조회가 필요하면 아래 방식 사용 가능
        // ═══════════════════════════════════════════════════════════════
        /*
        auto* myCollider = GetGameObject()->GetComponent<engine::Collider>();
        if (myCollider)
        {
            // Trigger 오버랩 확인
            const auto& overlaps = engine::CollisionSystem::Get().GetTriggerOverlaps(myCollider);
            for (auto* otherCollider : overlaps)
            {
                if (!otherCollider) continue;
                
                auto* otherGO = otherCollider->GetGameObject();
                if (!otherGO) continue;
                
                // 몬스터와 충돌했는지 확인
                if (auto* monster = otherGO->GetComponent<TempMonster>())
                {
                    monster->OnHit();
                    
                    // 즉시 삭제 대신, dying 상태로 전환하고 정지
                    m_isDying = true;
                    m_deathTimer = 0.0f;
                    
                    // 속도 정지
                    if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
                    {
                        rb->SetLinearVelocity(engine::Vector3::Zero);
                    }
                    return;
                }
            }
        }
        */

        // 화면 밖 체크 (간단한 범위 체크)
        engine::Vector3 pos = GetTransform()->GetWorldPosition();
        float boundary = 50.0f;  // 임시 경계값
        if (std::abs(pos.x) > boundary || std::abs(pos.y) > boundary)
        {
            GetGameObject()->Destroy();
            return;
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // Push 방식 충돌 콜백
    // CollisionSystem에서 자동으로 호출됨
    // ═══════════════════════════════════════════════════════════════
    void TempBullet::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        // 이미 죽는 중이면 무시
        if (m_isDying) return;
        
        // 상대 오브젝트 확인
        if (!info.gameObject) return;
        
        // 몬스터와 충돌했는지 확인
        if (auto* monster = info.gameObject->GetComponent<TempMonster>())
        {
            monster->OnHit();
            
            // 즉시 삭제 대신, dying 상태로 전환하고 정지
            m_isDying = true;
            m_deathTimer = 0.0f;
            
            // 속도 정지
            if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
            {
                rb->SetLinearVelocity(engine::Vector3::Zero);
            }
        }
    }

    void TempBullet::OnGui()
    {
        ImGui::Text("Direction: (%.2f, %.2f, %.2f)", m_direction.x, m_direction.y, m_direction.z);
        ImGui::Text("Speed: %.2f", m_speed);
        ImGui::Text("Lifetime: %.2f", m_lifetime);
    }

    void TempBullet::Save(engine::json& j) const
    {
        Object::Save(j);
        j["Direction"] = { m_direction.x, m_direction.y, m_direction.z };
        j["Speed"] = m_speed;
        j["Lifetime"] = m_lifetime;
    }

    void TempBullet::Load(const engine::json& j)
    {
        Object::Load(j);
        if (j.contains("Direction"))
        {
            auto& d = j["Direction"];
            m_direction = engine::Vector3(d[0].get<float>(), d[1].get<float>(), d[2].get<float>());
        }
        engine::JsonGet(j, "Speed", m_speed);
        engine::JsonGet(j, "Lifetime", m_lifetime);
    }

    std::string TempBullet::GetType() const
    {
        return "TempBullet";
    }
}
