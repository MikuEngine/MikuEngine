#include "GamePCH.h"
#include "TempMonster.h"

#include <Framework/Object/Component/SpriteRenderer.h>

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

    std::string TempMonster::GetType() const
    {
        return "TempMonster";
    }
}
