#include "GamePCH.h"
#include "ExecutionEffectScript.h"

#include <Core/System/MyTime.h>
#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Transform.h>

namespace game
{
    void ExecutionEffectScript::Start()
    {
        // 초기 스케일 저장
        if (auto* transform = GetTransform())
        {
            m_initialScale = transform->GetLocalScale();
        }
        m_timer = 0.0f;
        m_initialized = true;
    }

    void ExecutionEffectScript::Update()
    {
        if (!m_initialized) return;

        float deltaTime = engine::Time::DeltaTime();
        m_timer += deltaTime;

        // 진행률 계산 (0.0 ~ 1.0)
        float progress = m_timer / m_duration;

        if (progress >= 1.0f)
        {
            // 애니메이션 완료 → 자기 자신 소멸
            GetGameObject()->Destroy();
            return;
        }

        // 스케일 증가 애니메이션 (1.0 → scaleMultiplier)
        if (auto* transform = GetTransform())
        {
            float currentScale = 1.0f + (m_scaleMultiplier - 1.0f) * progress;
            engine::Vector3 newScale = m_initialScale * currentScale;
            transform->SetLocalScale(newScale);
        }
    }

    void ExecutionEffectScript::OnGui()
    {
        ImGui::Text("Execution Effect (Runtime Only)");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Settings are controlled by ExecutionIndicatorManager");
        
        ImGui::Separator();
        ImGui::Text("Current Settings:");
        ImGui::Text("Duration: %.2f sec", m_duration);
        ImGui::Text("Scale Multiplier: %.2f", m_scaleMultiplier);
        
        ImGui::Separator();
        ImGui::Text("Runtime Info:");
        ImGui::Text("Timer: %.2f / %.2f", m_timer, m_duration);
        if (m_duration > 0.0f)
        {
            ImGui::Text("Progress: %.1f%%", (m_timer / m_duration) * 100.0f);
        }
    }

    void ExecutionEffectScript::Save(engine::json& j) const
    {
        Object::Save(j);
        // 직렬화 없음 - ExecutionIndicatorManager에서 값을 설정함
    }

    void ExecutionEffectScript::Load(const engine::json& j)
    {
        Object::Load(j);
        // 직렬화 없음 - ExecutionIndicatorManager에서 값을 설정함
    }
}
