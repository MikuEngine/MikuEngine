#include "GamePCH.h"
#include "ExecutionSlowScript.h"

#include "Manager/TimeScaler.h"

namespace game
{
    void ExecutionSlowScript::Start()
    {
        // 초기화
    }

    void ExecutionSlowScript::Update()
    {
        // TimeScaler 완료 시 m_isActive 리셋
        if (m_isActive && !TimeScaler::IsActive())
        {
            m_isActive = false;
        }
    }

    void ExecutionSlowScript::StartSlowMotion()
    {
        // 이미 활성화 중이면 리셋 후 재시작 (연속 처형 지원)
        if (m_isActive)
        {
            TimeScaler::ResetWorldTimeScale();
        }

        m_isActive = true;

        if (m_useCrossfade)
        {
            // 크로스페이드 시간 계산
            float fadeInTime = m_slowDuration * m_fadeInRatio;
            float fadeOutTime = m_slowDuration * m_fadeOutRatio;

            // TimeScaler에 크로스페이드 슬로우 효과 적용
            TimeScaler::ApplyCrossfadeWorldTimeScale(
                m_targetTimeScale,  // 목표 스케일
                m_slowDuration,     // 총 지속 시간
                fadeInTime,         // 진입 시간
                fadeOutTime         // 이탈 시간
            );
        }
        else
        {
            // 고정 타임스케일 적용 (크로스페이드 없음)
            TimeScaler::ApplyWorldTimeScale(
                m_targetTimeScale,  // 목표 스케일
                m_slowDuration      // 총 지속 시간
            );
        }
    }

    void ExecutionSlowScript::StopSlowMotion()
    {
        if (!m_isActive) return;

        m_isActive = false;

        // 타임스케일 즉시 복원
        TimeScaler::ResetWorldTimeScale();
    }

    void ExecutionSlowScript::OnGui()
    {
        ImGui::Text("Execution Slow Motion");
        
        ImGui::Separator();
        ImGui::Text("Slow Effect Settings:");
        
        ImGui::DragFloat("Duration (sec)", &m_slowDuration, 0.1f, 0.1f, 10.0f);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Total duration of slow motion effect (real time)");
        }
        
        ImGui::DragFloat("Target Time Scale", &m_targetTimeScale, 0.01f, 0.01f, 1.0f);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Target time scale (0.2 = 5x slower)");
        }
        
        ImGui::Checkbox("Use Crossfade", &m_useCrossfade);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Enable crossfade (fade in/out). Disable for instant effect.");
        }
        
        if (m_useCrossfade)
        {
            ImGui::Separator();
            ImGui::Text("Crossfade Ratios (should sum to 1.0):");
            
            ImGui::DragFloat("Fade In Ratio", &m_fadeInRatio, 0.05f, 0.0f, 1.0f);
            ImGui::DragFloat("Sustain Ratio", &m_sustainRatio, 0.05f, 0.0f, 1.0f);
            ImGui::DragFloat("Fade Out Ratio", &m_fadeOutRatio, 0.05f, 0.0f, 1.0f);
            
            // 비율 합계 표시
            float totalRatio = m_fadeInRatio + m_sustainRatio + m_fadeOutRatio;
            if (std::abs(totalRatio - 1.0f) > 0.01f)
            {
                ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Warning: Ratios sum to %.2f (should be 1.0)", totalRatio);
            }
            else
            {
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Ratios OK: %.2f", totalRatio);
            }
            
            // 계산된 시간 표시
            ImGui::Text("Fade In: %.2f sec", m_slowDuration * m_fadeInRatio);
            ImGui::Text("Sustain: %.2f sec", m_slowDuration * m_sustainRatio);
            ImGui::Text("Fade Out: %.2f sec", m_slowDuration * m_fadeOutRatio);
        }
        
        ImGui::Separator();
        ImGui::Text("Runtime Info:");
        ImGui::Text("Is Active: %s", m_isActive ? "Yes" : "No");
    }

    void ExecutionSlowScript::Save(engine::json& j) const
    {
        Object::Save(j);
        
        j["SlowDuration"] = m_slowDuration;
        j["TargetTimeScale"] = m_targetTimeScale;
        j["UseCrossfade"] = m_useCrossfade;
        j["FadeInRatio"] = m_fadeInRatio;
        j["SustainRatio"] = m_sustainRatio;
        j["FadeOutRatio"] = m_fadeOutRatio;
    }

    void ExecutionSlowScript::Load(const engine::json& j)
    {
        Object::Load(j);
        
        engine::JsonGet(j, "SlowDuration", m_slowDuration);
        engine::JsonGet(j, "TargetTimeScale", m_targetTimeScale);
        engine::JsonGet(j, "UseCrossfade", m_useCrossfade);
        engine::JsonGet(j, "FadeInRatio", m_fadeInRatio);
        engine::JsonGet(j, "SustainRatio", m_sustainRatio);
        engine::JsonGet(j, "FadeOutRatio", m_fadeOutRatio);
    }
}
