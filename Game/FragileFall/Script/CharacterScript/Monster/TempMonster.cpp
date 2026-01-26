#include "GamePCH.h"
#include "TempMonster.h"

#include <Framework/Object/Component/Renderer/SpriteRenderer.h>
#include <Framework/Asset/Prefab.h>

namespace game
{
    void TempMonster::Start()
    {
        LOG_PRINT("[TempMonster] Started");
        
        // 자식 오브젝트에서 SpriteRenderer 찾기
        for (auto* childTransform : GetTransform()->GetChildren())
        {
            if (childTransform && childTransform->GetGameObject())
            {
                m_spriteRenderer = childTransform->GetGameObject()->GetComponent<engine::SpriteRenderer>();
                if (m_spriteRenderer)
                {
                    LOG_PRINT("[TempMonster] Found SpriteRenderer in child object");
                    break;
                }
            }
        }

        // 자식에 없으면 자신에서 찾기
        if (!m_spriteRenderer)
        {
            m_spriteRenderer = GetGameObject()->GetComponent<engine::SpriteRenderer>();
        }

        if (!m_spriteRenderer)
        {
            LOG_INFO("[TempMonster] Warning: SpriteRenderer not found. Color change will not work.");
        }

        m_hitCount = 0;
    }

    void TempMonster::OnHit()
    {
        m_hitCount++;
        
        LOG_PRINT("[TempMonster] '{}' hit! Count: {}", 
            GetGameObject()->GetName(),
            m_hitCount);
        
        ToggleHitIndicator();

        if (m_hitCount > 5)
        {
            if (!m_isCrystalized)
            {
                m_isCrystalized = true;

                auto go = engine::Prefab::Instantiate("Crystalized");
                go->GetTransform()->SetParent(GetTransform(), false);
                go->GetTransform()->SetLocalPosition(engine::Vector3(0.0f, 2.0f, 0.0f));
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // Push 방식 충돌 콜백
    // CollisionSystem에서 자동으로 호출됨
    // ═══════════════════════════════════════════════════════════════
    void TempMonster::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        // 상대 오브젝트 확인
        if (!info.gameObject) return;
        
        // 총알과 충돌했는지 확인
        // 참고: TempBullet::OnTriggerEnter에서 이미 OnHit()을 호출하므로
        //       여기서는 추가 처리가 필요한 경우에만 사용
        //       중복 호출 방지를 위해 아래 코드는 주석 처리
        /*
        if (auto* bullet = info.gameObject->GetComponent<TempBullet>())
        {
            // OnHit();  // TempBullet에서 이미 호출함
            LOG_PRINT("[TempMonster] Detected collision with bullet (via OnTriggerEnter)");
        }
        */
        
        // 향후 다른 종류의 충돌 처리 확장 가능
        // 예: 다른 종류의 공격, 아이템 등
    }

    void TempMonster::ToggleHitIndicator()
    {
        if (m_spriteRenderer)
        {
            // 활성화/비활성화 토글로 피격 표시
            bool currentActive = m_spriteRenderer->IsActive();
            m_spriteRenderer->SetActive(!currentActive);
            
            LOG_PRINT("[TempMonster] SpriteRenderer toggled: {}", !currentActive ? "ON" : "OFF");
        }
    }

    void TempMonster::OnGui()
    {
        ImGui::Text("Hit Count: %d", m_hitCount);
        ImGui::Text("SpriteRenderer: %s", m_spriteRenderer ? "Found" : "NOT FOUND");
        if (m_spriteRenderer)
        {
            ImGui::Text("Indicator: %s", m_spriteRenderer->IsActive() ? "ON" : "OFF");
        }
    }

    void TempMonster::Save(engine::json& j) const
    {
        Object::Save(j);
        j["HitCount"] = m_hitCount;
    }

    void TempMonster::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "HitCount", m_hitCount);
    }
}
