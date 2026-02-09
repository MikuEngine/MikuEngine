#include "GamePCH.h"
#include "CrystalIceFillControllerScript.h"

#include <Framework/Object/Component/Renderer/StaticMeshRenderer.h>
#include <Core/Graphics/Data/ShaderSlotTypes.h>
#include <Core/System/MyTime.h>
#include <imgui.h>

namespace game
{
    void CrystalIceFillControllerScript::Start()
    {
        m_renderer = GetGameObject()->GetComponent<engine::StaticMeshRenderer>();
        if (!m_renderer)
        {
            return;
        }

        m_renderer->SetTransparentShader(
            "Resource/Shader/Vertex/CrystalIceFill_VS.hlsl",
            "Resource/Shader/Pixel/CrystalIceFill_PS.hlsl");

        m_params.enable = 1;
        m_params.amount = 0.0f;
        m_elapsed = 0.0f;

        m_renderer->SetCustomBuffer(
            static_cast<int>(engine::ConstantBufferSlot::IceFill),
            &m_params, sizeof(m_params));
    }

    void CrystalIceFillControllerScript::SetFillAmount(float amount)
    {
        // 외부에서 직접 amount 설정 (0~1 클램핑)
        m_params.amount = (amount < 0.0f) ? 0.0f : ((amount > 1.0f) ? 1.0f : amount);
        
        // step 적용 (선택적, 더 부드러운 연출을 위해)
        if (m_stepCount > 0)
        {
            float step = 1.0f / static_cast<float>(m_stepCount);
            int stepIndex = static_cast<int>(m_params.amount / step);
            if (stepIndex > m_stepCount) stepIndex = m_stepCount;
            m_params.amount = static_cast<float>(stepIndex) * step;
        }
        
        // 셰이더에 업데이트
        if (m_renderer)
        {
            m_renderer->SetCustomBuffer(
                static_cast<int>(engine::ConstantBufferSlot::IceFill),
                &m_params, sizeof(m_params));
        }
    }

    void CrystalIceFillControllerScript::StartDrain()
    {
        m_isDraining = true;
        m_drainElapsed = 0.0f;
        m_params.amount = 1.0f;
    }

    void CrystalIceFillControllerScript::Update()
    {
        if (!m_renderer)
            return;

        // 드레인 중: 1 → 0 역방향 (Fragile 회복 시)
        if (m_isDraining)
        {
            if (m_drainDuration > 0.0f && m_stepCount > 0)
            {
                m_drainElapsed += engine::Time::DeltaTime();
                float t = m_drainElapsed / m_drainDuration;
                if (t >= 1.0f)
                {
                    t = 1.0f;
                    m_isDraining = false;
                }
                float step = 1.0f / static_cast<float>(m_stepCount);
                int stepIndex = static_cast<int>(t * m_stepCount);
                if (stepIndex > m_stepCount) stepIndex = m_stepCount;
                m_params.amount = 1.0f - static_cast<float>(stepIndex) * step;
                if (m_params.amount < 0.0f) m_params.amount = 0.0f;
            }
            else
            {
                m_params.amount = 0.0f;
                m_isDraining = false;
            }
            m_renderer->SetCustomBuffer(
                static_cast<int>(engine::ConstantBufferSlot::IceFill),
                &m_params, sizeof(m_params));
            return;
        }

        // 수동 제어 모드면 자동 채우기 건너뛰기
        if (m_manualControl)
            return;

        // 채우기: 0 → 1 (Fragile 진입 시, 수동 제어가 아닐 때만)
        if (m_elapsed >= m_duration)
            return;

        m_elapsed += engine::Time::DeltaTime();
        if (m_duration > 0.0f && m_stepCount > 0)
        {
            float t = m_elapsed / m_duration;
            if (t > 1.0f) t = 1.0f;
            float step = 1.0f / static_cast<float>(m_stepCount);
            m_params.amount = static_cast<float>(static_cast<int>(t * m_stepCount)) * step;
            if (m_params.amount > 1.0f) m_params.amount = 1.0f;
        }
        else
        {
            m_params.amount = 1.0f;
        }

        m_renderer->SetCustomBuffer(
            static_cast<int>(engine::ConstantBufferSlot::IceFill),
            &m_params, sizeof(m_params));
    }

    void CrystalIceFillControllerScript::OnGui()
    {
        ImGui::DragFloat("Fill Duration (sec)", &m_duration, 0.1f, 0.1f, 60.0f, "%.1f");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("채워지는 데 걸리는 시간(초)");

        ImGui::DragFloat("Drain Duration (sec)", &m_drainDuration, 0.1f, 0.0f, 60.0f, "%.1f");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("회복 시 줄어드는 데 걸리는 시간(초)");

        ImGui::DragInt("Step Count", &m_stepCount, 1, 1, 32);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("몇 단계로 올릴지 (면이 딱딱 올라가는 횟수)");
    }

    void CrystalIceFillControllerScript::Save(engine::json& j) const
    {
        engine::Object::Save(j);
        j["Duration"] = m_duration;
        j["StepCount"] = m_stepCount;
        j["DrainDuration"] = m_drainDuration;
    }

    void CrystalIceFillControllerScript::Load(const engine::json& j)
    {
        engine::Object::Load(j);
        engine::JsonGet(j, "Duration", m_duration);
        engine::JsonGet(j, "StepCount", m_stepCount);
        engine::JsonGet(j, "DrainDuration", m_drainDuration);
        if (m_duration < 0.1f) m_duration = 0.1f;
        if (m_stepCount < 1) m_stepCount = 1;
        if (m_drainDuration < 0.0f) m_drainDuration = m_duration;
    }
}
