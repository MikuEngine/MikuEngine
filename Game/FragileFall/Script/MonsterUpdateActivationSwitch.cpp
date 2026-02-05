#include "GamePCH.h"
#include "MonsterUpdateActivationSwitch.h"

#include <Engine/Core/System/MyTime.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void MonsterUpdateActivationSwitch::Awake()
    {
        // 초기 상태: 업데이트 비활성화
        m_isUpdateAllowed = false;
        m_elapsedTime = 0.0f;
        m_hasActivated = false;
    }

    void MonsterUpdateActivationSwitch::Update()
    {
        // 이미 활성화되었으면 더 이상 체크 안 함
        if (m_hasActivated) return;

        // 시간 누적
        m_elapsedTime += engine::Time::DeltaTime();

        // 설정된 시간이 지나면 업데이트 허용
        if (m_elapsedTime >= m_activationDelay)
        {
            m_isUpdateAllowed = true;
            m_hasActivated = true;
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void MonsterUpdateActivationSwitch::OnGui()
    {
        ImGui::Text("=== Monster Update Switch ===");
        ImGui::Separator();

        // 설정
        ImGui::Text("== Settings ==");
        ImGui::DragFloat("Activation Delay", &m_activationDelay, 0.1f, 0.0f, 10.0f, "%.2f sec");
        ImGui::Checkbox("Update Allowed (Manual)", &m_isUpdateAllowed);
        
        ImGui::Separator();

        // 런타임 정보
        ImGui::Text("== Runtime Status ==");
        ImGui::Text("Elapsed Time: %.2f / %.2f sec", m_elapsedTime, m_activationDelay);
        
        ImGui::TextColored(
            m_isUpdateAllowed ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
            "Status: %s", m_isUpdateAllowed ? "ACTIVE (Monsters Running)" : "FROZEN (Monsters Stopped)"
        );
        
        if (m_hasActivated)
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(Auto-activated)");
        }
    }

    void MonsterUpdateActivationSwitch::Save(engine::json& j) const
    {
        j["ActivationDelay"] = m_activationDelay;
        j["IsUpdateAllowed"] = m_isUpdateAllowed;  // 수동 설정 저장
    }

    void MonsterUpdateActivationSwitch::Load(const engine::json& j)
    {
        m_activationDelay = j.value("ActivationDelay", 1.0f);
        m_isUpdateAllowed = j.value("IsUpdateAllowed", false);
    }
}
