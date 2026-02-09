#include "GamePCH.h"
#include "BossSubPartsController.h"

#include <Framework/Asset/Prefab.h>

namespace game
{
    void BossSubPartsController::Start()
    {
        auto rotatingParts = GetTransform()->FindChildByNameRecursive("RotatingParts");

        for (int i = 0; i < m_orbitPartsCount; ++i)
        {
            auto go = engine::Prefab::Instantiate("RotatingPart");
            m_rotatingSubParts.push_back(go->GetTransform());
            go->GetTransform()->SetParent(rotatingParts->GetTransform(), false);
        }

        auto floatingParts = GetTransform()->FindChildByNameRecursive("FloatingParts");

        
    }

    void BossSubPartsController::Update()
    {
        // 1. DeltaTime 누적
        m_accTime += engine::Time::DeltaTime();

        size_t count = m_rotatingSubParts.size();
        if (count != 0)
        {
            float coneSlant = 0.5f;
            float baseRadius = m_orbitRadius; // 기본 반지름

            for (size_t i = 0; i < count; ++i)
            {
                // 1. 기본 위상 설정
                float ratio = static_cast<float>(i) / static_cast<float>(count);
                float circleOffset = ratio * DirectX::XM_2PI;

                // 2. 개별 파츠를 위한 미세 변동값 (Noise 효과)
                // 각 파츠마다 고유한 속도와 진폭을 가지도록 i를 이용해 시차를 줍니다.
                float individualSway = sin(m_accTime * (m_bobbingSpeed * 1.5f) + (i * 1.33f)) * (m_bobbingAmount * 0.2f);
                float individualSpeedVar = sin(m_accTime * 0.5f + (i * 0.77f)) * 0.1f; // 미세한 앞뒤 간격 변동

                // 3. Y축 계산 (전체 Bobbing + 개별 Sway)
                float y = (sin(m_accTime * m_bobbingSpeed + circleOffset) * m_bobbingAmount) + individualSway;

                // 4. 반지름 및 각도 계산 (미세 속도 변동 적용)
                float currentRadius = baseRadius + (y * coneSlant);
                float currentAngle = (m_accTime * engine::ToRadian(m_orbitSpeed + individualSpeedVar)) + circleOffset;

                float x = cos(currentAngle) * currentRadius;
                float z = sin(currentAngle) * currentRadius;

                // 5. 위치 적용
                m_rotatingSubParts[i]->SetLocalPosition(engine::Vector3(x, y, z));

                // 6. 회전 적용 (기존 틸트 유지)
                float lookAngle = atan2(-sin(currentAngle), cos(currentAngle));
                float tiltAngle = -30.0f;

                // 회전에도 미세한 떨림을 추가하고 싶다면 여기에 individualSway를 활용할 수 있습니다.
                m_rotatingSubParts[i]->SetLocalRotation(engine::Vector3(0.0f, engine::ToDegree(lookAngle), tiltAngle));
            }
        }
    }

    void BossSubPartsController::OnGui()
    {
        ImGui::DragInt("Orbit Parts Count", &m_orbitPartsCount, 1.0f, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
        ImGui::DragFloat("Orbit Speed", &m_orbitSpeed, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Orbit Radius", &m_orbitRadius, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Bobbing Speed", &m_bobbingSpeed, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Bobbing Amount", &m_bobbingAmount, 0.1f, 0.0f, 100.0f);
    }

    void BossSubPartsController::Save(engine::json& j) const
    {
        Object::Save(j);

        j["OrbitPartsCount"] = m_orbitPartsCount;
        j["OrbitSpeed"] = m_orbitSpeed;
        j["OrbitRadius"] = m_orbitRadius;
        j["BobbingSpeed"] = m_bobbingSpeed;
        j["BobbingAmount"] = m_bobbingAmount;
    }

    void BossSubPartsController::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "OrbitPartsCount", m_orbitPartsCount);
        engine::JsonGet(j, "OrbitSpeed", m_orbitSpeed);
        engine::JsonGet(j, "OrbitRadius", m_orbitRadius);
        engine::JsonGet(j, "BobbingSpeed", m_bobbingSpeed);
        engine::JsonGet(j, "BobbingAmount", m_bobbingAmount);
    }
}