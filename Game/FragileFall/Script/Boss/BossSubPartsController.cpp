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
        m_floatingSubParts = floatingParts->GetTransform()->GetChildren();

        auto nestParts = GetTransform()->FindChildByNameRecursive("NestParts");
        m_nestSubParts = nestParts->GetTransform()->GetChildren();
    }

    void BossSubPartsController::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::D1))
        {
            TriggerHitShake(1.0f);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D2))
        {
            TriggerHitShake(3.0f);
        }

        // 1. DeltaTime 누적
        m_accTime += engine::Time::DeltaTime();
        
        // 2. 피격 떨림 감쇠 처리
        if (m_currentHitShake > 0.0f)
        {
            m_currentHitShake -= m_hitShakeDecay * engine::Time::DeltaTime();
            if (m_currentHitShake < 0.0f)
                m_currentHitShake = 0.0f;
        }

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
                float individualSway = std::sin(m_accTime * (m_bobbingSpeed * 1.5f) + (i * 1.33f)) * (m_bobbingAmount * 0.2f);
                float individualSpeedVar = std::sin(m_accTime * 0.5f + (i * 0.77f)) * 0.1f; // 미세한 앞뒤 간격 변동

                // 3. Y축 계산 (전체 Bobbing + 개별 Sway)
                float y = (std::sin(m_accTime * m_bobbingSpeed + circleOffset) * m_bobbingAmount) + individualSway;

                // 4. 복잡한 타원 궤도 계산
                float currentAngle = (m_accTime * engine::ToRadian(m_orbitSpeed + individualSpeedVar)) + circleOffset;
                
                // 기본 타원 좌표
                float ellipseX = std::cos(currentAngle) * m_ellipseRatioX;
                float ellipseZ = std::sin(currentAngle) * m_ellipseRatioZ;
                
                // 궤도 왜곡을 위한 동적 변화
                float distortionTime = m_accTime * m_orbitDistortionSpeed;
                
                // 각 파츠별로 다른 왜곡 패턴 적용
                float partDistortionPhase = static_cast<float>(i) * 0.5f;
                
                // 파동 형태의 궤도 왜곡 (사인파를 여러 개 겹침)
                float waveDistortion = 0.0f;
                for (int wave = 1; wave <= static_cast<int>(m_orbitWaveCount); ++wave)
                {
                    float wavePhase = currentAngle * wave + distortionTime + partDistortionPhase;
                    waveDistortion += std::sin(wavePhase) / wave; // 고차 파동은 약하게
                }
                
                // 시간에 따라 변하는 노이즈 기반 왜곡
                float noisePhase1 = (currentAngle * m_orbitNoiseScale) + distortionTime;
                float noisePhase2 = (currentAngle * m_orbitNoiseScale * 1.7f) + (distortionTime * 0.6f) + partDistortionPhase;
                float noiseDistortion = (std::sin(noisePhase1) + std::sin(noisePhase2) * 0.5f);
                
                // 전체 왜곡량 계산
                float totalDistortion = (waveDistortion + noiseDistortion) * m_orbitDistortionAmount;
                
                // 왜곡된 반지름 계산
                float distortedRadius = baseRadius * (1.0f + totalDistortion);
                distortedRadius += (y * coneSlant); // 기존 콘 효과 유지
                
                // 최종 위치 계산
                float x = ellipseX * distortedRadius;
                float z = ellipseZ * distortedRadius;

                // 5. 위치 적용 (떨림 효과 포함)
                engine::Vector3 basePosition(x, y, z);
                engine::Vector3 finalPosition = ApplyShakeEffect(basePosition, static_cast<int>(i));
                m_rotatingSubParts[i]->SetLocalPosition(finalPosition);

                // 6. 회전 적용 (기존 틸트 유지)
                float lookAngle = std::atan2(-std::sin(currentAngle), std::cos(currentAngle));
                float tiltAngle = -30.0f;

                // 회전에도 미세한 떨림을 추가하고 싶다면 여기에 individualSway를 활용할 수 있습니다.
                m_rotatingSubParts[i]->SetLocalRotation(engine::Vector3(0.0f, engine::ToDegree(lookAngle), tiltAngle));
            }
        }

        // Floating Parts 업데이트
        size_t floatingCount = m_floatingSubParts.size();
        for (size_t i = 0; i < floatingCount; ++i)
        {
            // 각 파츠마다 고유한 속도와 위상을 계산
            float individualSpeedMultiplier = 1.0f + (static_cast<float>(i) * m_floatingSpeedVariation * 0.1f);
            float individualPhase = static_cast<float>(i) * m_floatingPhaseVariation;
            
            // 현재 파츠의 초기 위치 저장 (첫 프레임에서만)
            static std::vector<engine::Vector3> initialPositions;
            if (initialPositions.size() != floatingCount)
            {
                initialPositions.clear();
                for (size_t j = 0; j < floatingCount; ++j)
                {
                    initialPositions.push_back(m_floatingSubParts[j]->GetLocalPosition());
                }
            }
            
            // 위아래 움직임 계산
            float currentTime = m_accTime * m_floatingSpeed * individualSpeedMultiplier;
            float yOffset = std::sin(currentTime + individualPhase) * m_floatingAmplitude;
            
            // 초기 위치에서 Y축만 변경
            engine::Vector3 newPosition = initialPositions[i];
            newPosition.y += yOffset;
            
            // 떨림 효과 적용
            engine::Vector3 finalPosition = ApplyShakeEffect(newPosition, static_cast<int>(i) + 1000); // 다른 시드 사용
            m_floatingSubParts[i]->SetLocalPosition(finalPosition);
        }

        // Nest Parts 업데이트
        size_t nestCount = m_nestSubParts.size();
        for (size_t i = 0; i < nestCount; ++i)
        {
            // 각 파츠마다 고유한 펀치 타이밍
            float individualPunchPhase = static_cast<float>(i) * m_nestPunchVariation;
            
            // 현재 파츠의 초기 위치 저장 (첫 프레임에서만)
            static std::vector<engine::Vector3> nestInitialPositions;
            if (nestInitialPositions.size() != nestCount)
            {
                nestInitialPositions.clear();
                for (size_t j = 0; j < nestCount; ++j)
                {
                    nestInitialPositions.push_back(m_nestSubParts[j]->GetLocalPosition());
                }
            }
            
            // 1. 개별 파츠의 기계적인 펀치 움직임 (Y축)
            float punchTime = m_accTime * m_nestPunchSpeed + individualPunchPhase;
            // 기계적인 느낌을 위해 sharp한 움직임 사용 (sin^2 또는 step function)
            float punchValue = std::sin(punchTime);
            punchValue = punchValue > 0 ? punchValue * punchValue : 0; // 양수일 때만 제곱해서 sharp하게
            float yPunchOffset = punchValue * m_nestPunchAmount;
            
            // 2. 전체 그룹의 중심으로 모이는 움직임
            float groupTime = m_accTime * m_nestGroupSpeed;
            float groupCycleTime = std::fmod(groupTime, m_nestGroupCycle);
            
            // 주기적으로 중심으로 모였다가 돌아가는 움직임
            float groupEffect = 0.0f;
            if (groupCycleTime < m_nestGroupCycle * 0.3f) // 30% 시간 동안 중심으로 모이기
            {
                float t = groupCycleTime / (m_nestGroupCycle * 0.3f);
                groupEffect = std::sin(t * DirectX::XM_PI * 0.5f); // 부드럽게 증가
            }
            else if (groupCycleTime < m_nestGroupCycle * 0.6f) // 30% 시간 동안 중심에 머물기
            {
                groupEffect = 1.0f;
            }
            else // 40% 시간 동안 원래 자리로 돌아가기
            {
                float t = (groupCycleTime - m_nestGroupCycle * 0.6f) / (m_nestGroupCycle * 0.4f);
                groupEffect = std::cos(t * DirectX::XM_PI * 0.5f); // 부드럽게 감소
            }
            
            // 중심으로 향하는 벡터 계산
            engine::Vector3 initialPos = nestInitialPositions[i];
            engine::Vector3 toCenter = engine::Vector3(0, 0, 0) - initialPos;
            toCenter.y = 0; // Y축은 펀치 움직임만 사용
            
            // 최종 위치 계산
            engine::Vector3 newPosition = initialPos;
            newPosition += toCenter * (groupEffect * m_nestGroupAmount); // 중심으로 모이는 움직임
            newPosition.y += yPunchOffset; // 개별 펀치 움직임
            
            // 떨림 효과 적용
            engine::Vector3 finalPosition = ApplyShakeEffect(newPosition, static_cast<int>(i) + 2000); // 다른 시드 사용
            m_nestSubParts[i]->SetLocalPosition(finalPosition);
        }
    }
    
    // 떨림 효과를 위치에 적용하는 헬퍼 함수
    engine::Vector3 BossSubPartsController::ApplyShakeEffect(const engine::Vector3& originalPos, int partIndex)
    {
        // 전체 떨림 강도 계산 (기본 떨림 + 피격 떨림)
        float totalShakeIntensity = m_shakeIntensity + m_baseShakeAmount + (m_currentHitShake * m_hitShakeAmount);
        
        if (totalShakeIntensity <= 0.0f)
            return originalPos;
        
        // X, Z축에 대해 각각 다른 노이즈 생성
        float xNoise = engine::Random::Float(-1.0f, 1.0f);
        float zNoise = engine::Random::Float(-1.0f, 1.0f);
        
        // 떨림 벡터 생성
        engine::Vector3 shakeOffset;
        shakeOffset.x = xNoise * totalShakeIntensity;
        shakeOffset.y = 0.0f; // Y축은 떨림 없음
        shakeOffset.z = zNoise * totalShakeIntensity;
        
        return originalPos + shakeOffset;
    }
    
    void BossSubPartsController::TriggerHitShake(float intensity)
    {
        m_currentHitShake = intensity;
    }

    void BossSubPartsController::OnGui()
    {
        if (ImGui::CollapsingHeader("Rotating Parts"))
        {
            ImGui::DragInt("Orbit Parts Count", &m_orbitPartsCount, 1.0f, 0, 100, "%d", ImGuiSliderFlags_AlwaysClamp);
            ImGui::DragFloat("Orbit Speed", &m_orbitSpeed, 0.1f, 0.0f, 100.0f);
            ImGui::DragFloat("Orbit Radius", &m_orbitRadius, 0.1f, 0.0f, 100.0f);
            ImGui::DragFloat("Bobbing Speed", &m_bobbingSpeed, 0.1f, 0.0f, 100.0f);
            ImGui::DragFloat("Bobbing Amount", &m_bobbingAmount, 0.1f, 0.0f, 100.0f);
            
            ImGui::Separator();
            ImGui::Text("Ellipse Orbit Settings");
            ImGui::DragFloat("Ellipse Ratio X", &m_ellipseRatioX, 0.1f, 0.1f, 5.0f);
            ImGui::DragFloat("Ellipse Ratio Z", &m_ellipseRatioZ, 0.1f, 0.1f, 5.0f);
            ImGui::DragFloat("Distortion Amount", &m_orbitDistortionAmount, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Distortion Speed", &m_orbitDistortionSpeed, 0.1f, 0.0f, 5.0f);
            ImGui::DragFloat("Noise Scale", &m_orbitNoiseScale, 0.1f, 0.5f, 10.0f);
            ImGui::DragFloat("Wave Count", &m_orbitWaveCount, 0.5f, 1.0f, 10.0f);
        }
        
        if (ImGui::CollapsingHeader("Floating Parts"))
        {
            ImGui::DragFloat("Floating Speed", &m_floatingSpeed, 0.1f, 0.0f, 10.0f);
            ImGui::DragFloat("Floating Amplitude", &m_floatingAmplitude, 0.1f, 0.0f, 10.0f);
            ImGui::DragFloat("Speed Variation", &m_floatingSpeedVariation, 0.01f, 0.0f, 2.0f);
            ImGui::DragFloat("Phase Variation", &m_floatingPhaseVariation, 0.1f, 0.0f, 5.0f);
            ImGui::Text("Floating Parts Count: %d", static_cast<int>(m_floatingSubParts.size()));
        }
        
        if (ImGui::CollapsingHeader("Nest Parts"))
        {
            ImGui::DragFloat("Punch Speed", &m_nestPunchSpeed, 0.1f, 0.0f, 10.0f);
            ImGui::DragFloat("Punch Amount", &m_nestPunchAmount, 0.1f, 0.0f, 3.0f);
            ImGui::DragFloat("Punch Variation", &m_nestPunchVariation, 0.1f, 0.0f, 5.0f);
            ImGui::Separator();
            ImGui::DragFloat("Group Speed", &m_nestGroupSpeed, 0.01f, 0.0f, 2.0f);
            ImGui::DragFloat("Group Amount", &m_nestGroupAmount, 0.01f, 0.0f, 1.0f);
            ImGui::DragFloat("Group Cycle", &m_nestGroupCycle, 0.5f, 1.0f, 20.0f);
            ImGui::Text("Nest Parts Count: %d", static_cast<int>(m_nestSubParts.size()));
        }
        
        if (ImGui::CollapsingHeader("Shake Effects"))
        {
            ImGui::DragFloat("Shake Intensity", &m_shakeIntensity, 0.01f, 0.0f, 2.0f);
            ImGui::DragFloat("Shake Speed", &m_shakeSpeed, 0.5f, 1.0f, 50.0f);
            ImGui::DragFloat("Base Shake Amount", &m_baseShakeAmount, 0.001f, 0.0f, 0.2f);
            ImGui::Separator();
            ImGui::DragFloat("Hit Shake Amount", &m_hitShakeAmount, 0.01f, 0.0f, 2.0f);
            ImGui::DragFloat("Hit Shake Duration", &m_hitShakeDuration, 0.1f, 0.1f, 5.0f);
            ImGui::DragFloat("Hit Shake Decay", &m_hitShakeDecay, 0.1f, 0.1f, 10.0f);
            ImGui::Text("Current Hit Shake: %.3f", m_currentHitShake);
            
            if (ImGui::Button("Test Hit Shake"))
            {
                TriggerHitShake(1.0f);
            }
        }
    }

    void BossSubPartsController::Save(engine::json& j) const
    {
        Object::Save(j);

        j["OrbitPartsCount"] = m_orbitPartsCount;
        j["OrbitSpeed"] = m_orbitSpeed;
        j["OrbitRadius"] = m_orbitRadius;
        j["BobbingSpeed"] = m_bobbingSpeed;
        j["BobbingAmount"] = m_bobbingAmount;
        
        j["EllipseRatioX"] = m_ellipseRatioX;
        j["EllipseRatioZ"] = m_ellipseRatioZ;
        j["OrbitDistortionAmount"] = m_orbitDistortionAmount;
        j["OrbitDistortionSpeed"] = m_orbitDistortionSpeed;
        j["OrbitNoiseScale"] = m_orbitNoiseScale;
        j["OrbitWaveCount"] = m_orbitWaveCount;
        
        j["FloatingSpeed"] = m_floatingSpeed;
        j["FloatingAmplitude"] = m_floatingAmplitude;
        j["FloatingSpeedVariation"] = m_floatingSpeedVariation;
        j["FloatingPhaseVariation"] = m_floatingPhaseVariation;
        
        j["NestPunchSpeed"] = m_nestPunchSpeed;
        j["NestPunchAmount"] = m_nestPunchAmount;
        j["NestPunchVariation"] = m_nestPunchVariation;
        j["NestGroupSpeed"] = m_nestGroupSpeed;
        j["NestGroupAmount"] = m_nestGroupAmount;
        j["NestGroupCycle"] = m_nestGroupCycle;
        
        j["ShakeIntensity"] = m_shakeIntensity;
        j["ShakeSpeed"] = m_shakeSpeed;
        j["BaseShakeAmount"] = m_baseShakeAmount;
        j["HitShakeAmount"] = m_hitShakeAmount;
        j["HitShakeDuration"] = m_hitShakeDuration;
        j["HitShakeDecay"] = m_hitShakeDecay;
    }

    void BossSubPartsController::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "OrbitPartsCount", m_orbitPartsCount);
        engine::JsonGet(j, "OrbitSpeed", m_orbitSpeed);
        engine::JsonGet(j, "OrbitRadius", m_orbitRadius);
        engine::JsonGet(j, "BobbingSpeed", m_bobbingSpeed);
        engine::JsonGet(j, "BobbingAmount", m_bobbingAmount);
        
        engine::JsonGet(j, "EllipseRatioX", m_ellipseRatioX);
        engine::JsonGet(j, "EllipseRatioZ", m_ellipseRatioZ);
        engine::JsonGet(j, "OrbitDistortionAmount", m_orbitDistortionAmount);
        engine::JsonGet(j, "OrbitDistortionSpeed", m_orbitDistortionSpeed);
        engine::JsonGet(j, "OrbitNoiseScale", m_orbitNoiseScale);
        engine::JsonGet(j, "OrbitWaveCount", m_orbitWaveCount);
        
        engine::JsonGet(j, "FloatingSpeed", m_floatingSpeed);
        engine::JsonGet(j, "FloatingAmplitude", m_floatingAmplitude);
        engine::JsonGet(j, "FloatingSpeedVariation", m_floatingSpeedVariation);
        engine::JsonGet(j, "FloatingPhaseVariation", m_floatingPhaseVariation);
        
        engine::JsonGet(j, "NestPunchSpeed", m_nestPunchSpeed);
        engine::JsonGet(j, "NestPunchAmount", m_nestPunchAmount);
        engine::JsonGet(j, "NestPunchVariation", m_nestPunchVariation);
        engine::JsonGet(j, "NestGroupSpeed", m_nestGroupSpeed);
        engine::JsonGet(j, "NestGroupAmount", m_nestGroupAmount);
        engine::JsonGet(j, "NestGroupCycle", m_nestGroupCycle);
        
        engine::JsonGet(j, "ShakeIntensity", m_shakeIntensity);
        engine::JsonGet(j, "ShakeSpeed", m_shakeSpeed);
        engine::JsonGet(j, "BaseShakeAmount", m_baseShakeAmount);
        engine::JsonGet(j, "HitShakeAmount", m_hitShakeAmount);
        engine::JsonGet(j, "HitShakeDuration", m_hitShakeDuration);
        engine::JsonGet(j, "HitShakeDecay", m_hitShakeDecay);
    }
}