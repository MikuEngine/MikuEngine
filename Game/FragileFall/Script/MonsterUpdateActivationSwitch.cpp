#include "GamePCH.h"
#include "MonsterUpdateActivationSwitch.h"

#include <Engine/Core/System/MyTime.h>

namespace game
{
    void MonsterUpdateActivationSwitch::ResetRuntimeState()
    {
        // 런타임 상태는 플레이 시작마다 동일하게 재초기화한다.
        m_isUpdateAllowed = true;
        m_hasActivated = false;
        m_elapsedTime = 0.0f;

        // Idle 대기 모드면 외부 신호를 기다리고,
        // 즉시 대기 모드면 Update에서 곧바로 카운트를 시작한다.
        m_waitingForDelay = !m_waitAfterIdle;
    }

    void MonsterUpdateActivationSwitch::BeginDelayAfterIdle()
    {
        if (!m_waitAfterIdle || m_hasActivated || m_waitingForDelay)
        {
            return;
        }

        m_waitingForDelay = true;
        m_elapsedTime = 0.0f;

        if (m_activationDelay <= 0.0f)
        {
            m_isUpdateAllowed = true;
            m_hasActivated = true;
            m_waitingForDelay = false;
            return;
        }

        m_isUpdateAllowed = false;
    }

    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void MonsterUpdateActivationSwitch::Awake()
    {
        ResetRuntimeState();
    }

    void MonsterUpdateActivationSwitch::Start()
    {
        ResetRuntimeState();
    }

    void MonsterUpdateActivationSwitch::Update()
    {
        // 이미 활성화되었으면 더 이상 체크 안 함
        if (m_hasActivated) return;

        // Idle 도달 이후 대기 모드에서는 신호가 오기 전까지 Update 허용 유지
        if (!m_waitingForDelay)
        {
            m_isUpdateAllowed = true;
            return;
        }

        // 시간 누적
        m_elapsedTime += engine::Time::DeltaTime();

       
        // 설정된 시간이 지나면 업데이트 허용
        if (m_elapsedTime >= m_activationDelay)
        {
            m_isUpdateAllowed = true;
            m_hasActivated = true;
            m_waitingForDelay = false;
        }
        else if (!m_waitAfterIdle && m_elapsedTime >= 0.05f)
        {
            m_isUpdateAllowed = false;
        }
        else if (m_waitAfterIdle)
        {
            m_isUpdateAllowed = false;
        }
    }  

    void MonsterUpdateActivationSwitch::SetSwitchActivation(bool setparam)
    {
        m_isUpdateAllowed = setparam;
        m_hasActivated = setparam;
        m_waitingForDelay = false;
        m_elapsedTime = 0.0f;
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
        ImGui::Checkbox("Wait Delay After First Idle", &m_waitAfterIdle);
        ImGui::Checkbox("Update Allowed (Manual)", &m_isUpdateAllowed);
        
        ImGui::Separator();

        // 런타임 정보
        ImGui::Text("== Runtime Status ==");
        ImGui::Text("Elapsed Time: %.2f / %.2f sec", m_elapsedTime, m_activationDelay);
        ImGui::Text("Waiting For Delay: %s", m_waitingForDelay ? "Yes" : "No");
        
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
        // 베이스 클래스 Save 호출 (Type 필드 직렬화 - 필수!)
        Object::Save(j);
        
        j["ActivationDelay"] = m_activationDelay;
        j["WaitAfterIdle"] = m_waitAfterIdle;
    }

    void MonsterUpdateActivationSwitch::Load(const engine::json& j)
    {
        // 베이스 클래스 Load 호출 (Type 필드 역직렬화 - 필수!)
        Object::Load(j);
        
        m_activationDelay = j.value("ActivationDelay", 1.0f);
        m_waitAfterIdle = j.value("WaitAfterIdle", true);
        ResetRuntimeState();
    }
}
