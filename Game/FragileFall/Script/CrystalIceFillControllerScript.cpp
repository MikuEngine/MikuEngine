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

    void CrystalIceFillControllerScript::Update()
    {
        if (!m_renderer || m_elapsed >= m_duration)
        {
            return;
        }

        m_elapsed += engine::Time::DeltaTime();
        // step 식: 0 → 1/stepCount → 2/stepCount → … → 1.0 으로 딱딱 올라감
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

        ImGui::DragInt("Step Count", &m_stepCount, 1, 1, 32);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("몇 단계로 올릴지 (면이 딱딱 올라가는 횟수)");
    }

    void CrystalIceFillControllerScript::Save(engine::json& j) const
    {
        engine::Object::Save(j);
        j["Duration"] = m_duration;
        j["StepCount"] = m_stepCount;
    }

    void CrystalIceFillControllerScript::Load(const engine::json& j)
    {
        engine::Object::Load(j);
        engine::JsonGet(j, "Duration", m_duration);
        engine::JsonGet(j, "StepCount", m_stepCount);
        if (m_duration < 0.1f) m_duration = 0.1f;
        if (m_stepCount < 1) m_stepCount = 1;
    }
}
