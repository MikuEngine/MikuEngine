#include "GamePCH.h"
#include "BossSubPartsController.h"

#include <Framework/Asset/Prefab.h>
#include <algorithm>

namespace game
{
    namespace
    {
        float EaseOutCubic(float t)
        {
            t = std::clamp(t, 0.0f, 1.0f);
            float k = 1.0f - t;
            return 1.0f - (k * k * k);
        }
    }

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
        m_floatingInitialLocalPositions.clear();
        m_floatingInitialLocalPositions.reserve(m_floatingSubParts.size());
        for (auto* part : m_floatingSubParts)
        {
            m_floatingInitialLocalPositions.push_back(part->GetLocalPosition());
        }

        auto nestParts = GetTransform()->FindChildByNameRecursive("NestParts");
        m_nestSubParts = nestParts->GetTransform()->GetChildren();
        m_nestInitialLocalPositions.clear();
        m_nestInitialLocalPositions.reserve(m_nestSubParts.size());
        for (auto* part : m_nestSubParts)
        {
            m_nestInitialLocalPositions.push_back(part->GetLocalPosition());
        }
    }

    void BossSubPartsController::PrepareIntroAssembly()
    {
        m_introParts.clear();
        m_introStartLocalPositions.clear();
        m_introStartLocalRotations.clear();
        m_introTargetLocalPositions.clear();
        m_introTargetRotations.clear();
        m_introPartStartTimes.clear();

        const size_t rotatingCount = m_rotatingSubParts.size();
        for (size_t i = 0; i < rotatingCount; ++i)
        {
            m_introParts.push_back(m_rotatingSubParts[i]);
            m_introTargetLocalPositions.push_back(ComputeRotatingTargetLocalPosition(i, rotatingCount));
            m_introTargetRotations.push_back(ComputeRotatingTargetLocalRotation(i, rotatingCount));
        }

        for (auto* part : m_floatingSubParts)
        {
            m_introParts.push_back(part);
            m_introTargetLocalPositions.push_back(part->GetLocalPosition());
            m_introTargetRotations.push_back(part->GetLocalEulerAngles());
        }

        for (auto* part : m_nestSubParts)
        {
            m_introParts.push_back(part);
            m_introTargetLocalPositions.push_back(part->GetLocalPosition());
            m_introTargetRotations.push_back(part->GetLocalEulerAngles());
        }

        const size_t totalCount = m_introParts.size();
        if (totalCount == 0)
        {
            m_introActive = false;
            m_introAssembled = true;
            return;
        }

        m_introStartLocalPositions.reserve(totalCount);
        m_introStartLocalRotations.reserve(totalCount);
        for (size_t i = 0; i < totalCount; ++i)
        {
            const engine::Vector3 startPos = GenerateIntroSpawnOffset(i, totalCount);
            m_introStartLocalPositions.push_back(startPos);
            m_introStartLocalRotations.push_back(m_introParts[i]->GetLocalEulerAngles());
            m_introParts[i]->SetLocalPosition(startPos);
        }

        m_introPartStartTimes.assign(totalCount, 0.0f);
        if (totalCount > 1)
        {
            const float perPartDelay = std::max(0.0f, m_introStaggerPerPart);
            const float jitter = std::max(0.0f, m_introStaggerJitter);

            if (m_introStaggerRandomOrder)
            {
                std::vector<std::pair<float, size_t>> randomOrder;
                randomOrder.reserve(totalCount);
                for (size_t i = 0; i < totalCount; ++i)
                {
                    randomOrder.emplace_back(engine::Random::Float(0.0f, 1.0f), i);
                }

                std::sort(randomOrder.begin(), randomOrder.end(),
                    [](const auto& a, const auto& b)
                    {
                        return a.first < b.first;
                    });

                for (size_t rank = 0; rank < totalCount; ++rank)
                {
                    const size_t partIndex = randomOrder[rank].second;
                    m_introPartStartTimes[partIndex] =
                        (static_cast<float>(rank) * perPartDelay) + engine::Random::Float(0.0f, jitter);
                }
            }
            else
            {
                for (size_t i = 0; i < totalCount; ++i)
                {
                    m_introPartStartTimes[i] =
                        (static_cast<float>(i) * perPartDelay) + engine::Random::Float(0.0f, jitter);
                }
            }
        }

        m_introElapsed = 0.0f;
        m_introAssembled = false;
        m_introActive = true;
    }

    void BossSubPartsController::Update()
    {
        const float deltaTime = engine::Time::DeltaTime();

        if (engine::Input::IsKeyPressed(engine::Keys::D1))
        {
            TriggerHitShake(1.0f);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D2))
        {
            TriggerHitShake(3.0f);
        }
        
        // 피격 떨림 감쇠 처리
        if (m_currentHitShake > 0.0f)
        {
            m_currentHitShake -= m_hitShakeDecay * deltaTime;
            if (m_currentHitShake < 0.0f)
                m_currentHitShake = 0.0f;
        }

        m_accTime += deltaTime;

        if (m_introActive)
        {
            // During intro, chase runtime-updated target positions.
            UpdateIntroAssembly(deltaTime);
            return;
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
            engine::Vector3 basePosition = ComputeFloatingTargetLocalPosition(i);
            engine::Vector3 finalPosition = ApplyShakeEffect(basePosition, static_cast<int>(i) + 1000); // 다른 시드 사용
            m_floatingSubParts[i]->SetLocalPosition(finalPosition);
        }

        // Nest Parts 업데이트
        size_t nestCount = m_nestSubParts.size();
        for (size_t i = 0; i < nestCount; ++i)
        {
            engine::Vector3 basePosition = ComputeNestTargetLocalPosition(i);
            engine::Vector3 finalPosition = ApplyShakeEffect(basePosition, static_cast<int>(i) + 2000); // 다른 시드 사용
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

    engine::Vector3 BossSubPartsController::ComputeRotatingTargetLocalPosition(size_t partIndex, size_t totalCount) const
    {
        if (totalCount == 0)
        {
            return engine::Vector3::Zero;
        }

        float coneSlant = 0.5f;
        float baseRadius = m_orbitRadius;
        float ratio = static_cast<float>(partIndex) / static_cast<float>(totalCount);
        float circleOffset = ratio * DirectX::XM_2PI;
        float individualSway = std::sin(m_accTime * (m_bobbingSpeed * 1.5f) + (static_cast<float>(partIndex) * 1.33f)) * (m_bobbingAmount * 0.2f);
        float individualSpeedVar = std::sin(m_accTime * 0.5f + (static_cast<float>(partIndex) * 0.77f)) * 0.1f;
        float y = (std::sin(m_accTime * m_bobbingSpeed + circleOffset) * m_bobbingAmount) + individualSway;
        float currentAngle = (m_accTime * engine::ToRadian(m_orbitSpeed + individualSpeedVar)) + circleOffset;
        float ellipseX = std::cos(currentAngle) * m_ellipseRatioX;
        float ellipseZ = std::sin(currentAngle) * m_ellipseRatioZ;
        float distortionTime = m_accTime * m_orbitDistortionSpeed;
        float partDistortionPhase = static_cast<float>(partIndex) * 0.5f;
        float waveDistortion = 0.0f;
        for (int wave = 1; wave <= static_cast<int>(m_orbitWaveCount); ++wave)
        {
            float wavePhase = currentAngle * wave + distortionTime + partDistortionPhase;
            waveDistortion += std::sin(wavePhase) / static_cast<float>(wave);
        }
        float noisePhase1 = (currentAngle * m_orbitNoiseScale) + distortionTime;
        float noisePhase2 = (currentAngle * m_orbitNoiseScale * 1.7f) + (distortionTime * 0.6f) + partDistortionPhase;
        float noiseDistortion = (std::sin(noisePhase1) + std::sin(noisePhase2) * 0.5f);
        float totalDistortion = (waveDistortion + noiseDistortion) * m_orbitDistortionAmount;
        float distortedRadius = baseRadius * (1.0f + totalDistortion);
        distortedRadius += (y * coneSlant);

        return engine::Vector3(ellipseX * distortedRadius, y, ellipseZ * distortedRadius);
    }

    engine::Vector3 BossSubPartsController::ComputeRotatingTargetLocalRotation(size_t partIndex, size_t totalCount) const
    {
        if (totalCount == 0)
        {
            return engine::Vector3::Zero;
        }

        const float ratio = static_cast<float>(partIndex) / static_cast<float>(totalCount);
        const float circleOffset = ratio * DirectX::XM_2PI;
        const float individualSpeedVar = std::sin(m_accTime * 0.5f + (static_cast<float>(partIndex) * 0.77f)) * 0.1f;
        const float currentAngle = (m_accTime * engine::ToRadian(m_orbitSpeed + individualSpeedVar)) + circleOffset;
        const float lookAngle = std::atan2(-std::sin(currentAngle), std::cos(currentAngle));
        const float tiltAngle = -30.0f;
        return engine::Vector3(0.0f, engine::ToDegree(lookAngle), tiltAngle);
    }

    engine::Vector3 BossSubPartsController::ComputeFloatingTargetLocalPosition(size_t partIndex) const
    {
        if (partIndex >= m_floatingInitialLocalPositions.size())
        {
            return engine::Vector3::Zero;
        }

        const float individualSpeedMultiplier = 1.0f + (static_cast<float>(partIndex) * m_floatingSpeedVariation * 0.1f);
        const float individualPhase = static_cast<float>(partIndex) * m_floatingPhaseVariation;
        const float currentTime = m_accTime * m_floatingSpeed * individualSpeedMultiplier;
        const float yOffset = std::sin(currentTime + individualPhase) * m_floatingAmplitude;

        engine::Vector3 pos = m_floatingInitialLocalPositions[partIndex];
        pos.y += yOffset;
        return pos;
    }

    engine::Vector3 BossSubPartsController::ComputeNestTargetLocalPosition(size_t partIndex) const
    {
        if (partIndex >= m_nestInitialLocalPositions.size())
        {
            return engine::Vector3::Zero;
        }

        const float individualPunchPhase = static_cast<float>(partIndex) * m_nestPunchVariation;
        const float punchTime = m_accTime * m_nestPunchSpeed + individualPunchPhase;
        float punchValue = std::sin(punchTime);
        punchValue = (punchValue > 0.0f) ? (punchValue * punchValue) : 0.0f;
        const float yPunchOffset = punchValue * m_nestPunchAmount;

        const float groupTime = m_accTime * m_nestGroupSpeed;
        const float groupCycle = std::max(0.01f, m_nestGroupCycle);
        const float groupCycleTime = std::fmod(groupTime, groupCycle);

        float groupEffect = 0.0f;
        if (groupCycleTime < groupCycle * 0.3f)
        {
            const float t = groupCycleTime / (groupCycle * 0.3f);
            groupEffect = std::sin(t * DirectX::XM_PI * 0.5f);
        }
        else if (groupCycleTime < groupCycle * 0.6f)
        {
            groupEffect = 1.0f;
        }
        else
        {
            const float t = (groupCycleTime - groupCycle * 0.6f) / (groupCycle * 0.4f);
            groupEffect = std::cos(t * DirectX::XM_PI * 0.5f);
        }

        const engine::Vector3 initialPos = m_nestInitialLocalPositions[partIndex];
        engine::Vector3 toCenter = engine::Vector3::Zero - initialPos;
        toCenter.y = 0.0f;

        engine::Vector3 pos = initialPos;
        pos += toCenter * (groupEffect * m_nestGroupAmount);
        pos.y += yPunchOffset;
        return pos;
    }

    engine::Vector3 BossSubPartsController::ComputeIntroDynamicTargetLocalPosition(size_t introPartIndex) const
    {
        const size_t rotatingCount = m_rotatingSubParts.size();
        const size_t floatingCount = m_floatingSubParts.size();

        if (introPartIndex < rotatingCount)
        {
            return ComputeRotatingTargetLocalPosition(introPartIndex, rotatingCount);
        }

        if (introPartIndex < (rotatingCount + floatingCount))
        {
            return ComputeFloatingTargetLocalPosition(introPartIndex - rotatingCount);
        }

        return ComputeNestTargetLocalPosition(introPartIndex - rotatingCount - floatingCount);
    }

    engine::Vector3 BossSubPartsController::ComputeIntroDynamicTargetLocalRotation(size_t introPartIndex) const
    {
        const size_t rotatingCount = m_rotatingSubParts.size();
        const size_t floatingCount = m_floatingSubParts.size();

        if (introPartIndex < rotatingCount)
        {
            return ComputeRotatingTargetLocalRotation(introPartIndex, rotatingCount);
        }

        if (introPartIndex < (rotatingCount + floatingCount))
        {
            return m_floatingSubParts[introPartIndex - rotatingCount]->GetLocalEulerAngles();
        }

        const size_t nestIndex = introPartIndex - rotatingCount - floatingCount;
        if (nestIndex < m_nestSubParts.size())
        {
            return m_nestSubParts[nestIndex]->GetLocalEulerAngles();
        }

        return engine::Vector3::Zero;
    }

    engine::Vector3 BossSubPartsController::GenerateIntroSpawnOffset(size_t partIndex, size_t totalCount) const
    {
        if (totalCount == 0)
        {
            return engine::Vector3::Zero;
        }

        if (m_introDirectionMode == 0)
        {
            // Side-aware spawn: left-target parts come from left-upper side,
            // right-target parts come from right side. This reduces core crossing.
            const engine::Vector3 targetPos =
                (partIndex < m_introTargetLocalPositions.size()) ? m_introTargetLocalPositions[partIndex] : engine::Vector3::Zero;
            const float side = (targetPos.x < 0.0f) ? -1.0f : 1.0f;

            const float sideMin = m_introSpawnRadius * 0.70f;
            const float sideMax = m_introSpawnRadius * 1.05f;
            const float x = (side * engine::Random::Float(sideMin, sideMax)) + engine::Random::Float(-1.0f, 1.0f);

            // Keep depth loosely correlated with the target to avoid heavy cross-through.
            const float targetZBias = std::clamp(targetPos.z * 0.5f, -m_introSpawnRadius * 0.6f, m_introSpawnRadius * 0.6f);
            const float z = targetZBias + engine::Random::Float(-m_introSpawnRadius * 0.22f, m_introSpawnRadius * 0.22f);

            float y = m_introSpawnBaseHeight + engine::Random::Float(0.0f, m_introSpawnHeightJitter);
            if (side < 0.0f)
            {
                y += engine::Random::Float(1.0f, 2.5f); // left side starts higher
            }

            return engine::Vector3(x, y, z);
        }

        if (m_introDirectionMode == 2)
        {
            static const engine::Vector3 directions[8] =
            {
                engine::Vector3(1.0f, 0.0f, 0.0f), engine::Vector3(-1.0f, 0.0f, 0.0f),
                engine::Vector3(0.0f, 0.0f, 1.0f), engine::Vector3(0.0f, 0.0f, -1.0f),
                engine::Vector3(0.7071f, 0.0f, 0.7071f), engine::Vector3(-0.7071f, 0.0f, 0.7071f),
                engine::Vector3(0.7071f, 0.0f, -0.7071f), engine::Vector3(-0.7071f, 0.0f, -0.7071f)
            };
            const engine::Vector3 dir = directions[partIndex % 8];
            return engine::Vector3(
                dir.x * m_introSpawnRadius + engine::Random::Float(-1.5f, 1.5f),
                m_introSpawnBaseHeight + engine::Random::Float(0.0f, m_introSpawnHeightJitter),
                dir.z * m_introSpawnRadius + engine::Random::Float(-1.5f, 1.5f)
            );
        }

        // Fibonacci sphere distribution for omnidirectional gather.
        const float goldenAngle = DirectX::XM_2PI * 0.61803398875f;
        const float t = (static_cast<float>(partIndex) + 0.5f) / static_cast<float>(totalCount);
        const float y = 1.0f - (2.0f * t);
        const float radius = std::sqrt(std::max(0.0f, 1.0f - (y * y)));
        const float theta = goldenAngle * static_cast<float>(partIndex);
        const float x = std::cos(theta) * radius;
        const float z = std::sin(theta) * radius;

        return engine::Vector3(
            x * m_introSpawnRadius,
            y * m_introSpawnRadius * 0.35f + m_introSpawnBaseHeight + engine::Random::Float(-m_introSpawnHeightJitter * 0.5f, m_introSpawnHeightJitter),
            z * m_introSpawnRadius
        );
    }

    void BossSubPartsController::UpdateIntroAssembly(float deltaTime)
    {
        m_introElapsed += deltaTime;

        const size_t count = m_introParts.size();
        if (count == 0)
        {
            m_introActive = false;
            m_introAssembled = true;
            return;
        }

        if (m_introPartStartTimes.size() != count)
        {
            m_introPartStartTimes.assign(count, 0.0f);
        }

        const float totalDuration = std::max(0.01f, m_introAssembleDuration);
        float maxStartTime = 0.0f;
        for (float startTime : m_introPartStartTimes)
        {
            maxStartTime = std::max(maxStartTime, startTime);
        }
        const float moveDuration = std::max(0.01f, totalDuration - maxStartTime);

        bool allArrived = true;
        for (size_t i = 0; i < count; ++i)
        {
            m_introTargetLocalPositions[i] = ComputeIntroDynamicTargetLocalPosition(i);
            m_introTargetRotations[i] = ComputeIntroDynamicTargetLocalRotation(i);
            const float startTime = m_introPartStartTimes[i];
            const float localT = std::clamp((m_introElapsed - startTime) / moveDuration, 0.0f, 1.0f);
            const float eased = EaseOutCubic(localT);
            const engine::Vector3& startPos = m_introStartLocalPositions[i];
            const engine::Vector3& targetPos = m_introTargetLocalPositions[i];
            const engine::Vector3 lerped = startPos + ((targetPos - startPos) * eased);
            const engine::Vector3& startRot = m_introStartLocalRotations[i];
            const engine::Vector3& targetRot = m_introTargetRotations[i];
            const engine::Vector3 lerpedRot = startRot + ((targetRot - startRot) * eased);
            m_introParts[i]->SetLocalPosition(lerped);
            m_introParts[i]->SetLocalRotation(lerpedRot);

            if (localT < 1.0f)
            {
                allArrived = false;
            }
        }

        if (allArrived)
        {
            for (size_t i = 0; i < count; ++i)
            {
                m_introParts[i]->SetLocalPosition(m_introTargetLocalPositions[i]);
                m_introParts[i]->SetLocalRotation(m_introTargetRotations[i]);
            }

            m_introActive = false;
            m_introAssembled = true;
        }
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

        if (ImGui::CollapsingHeader("Boss Intro Assemble"))
        {
            ImGui::DragFloat("Intro Assemble Duration", &m_introAssembleDuration, 0.05f, 0.1f, 10.0f);
            ImGui::DragFloat("Intro Stagger Per Part", &m_introStaggerPerPart, 0.001f, 0.0f, 0.2f);
            ImGui::DragFloat("Intro Stagger Jitter", &m_introStaggerJitter, 0.001f, 0.0f, 0.3f);
            ImGui::Checkbox("Intro Stagger Random Order", &m_introStaggerRandomOrder);
            ImGui::DragFloat("Intro Spawn Radius", &m_introSpawnRadius, 0.5f, 2.0f, 60.0f);
            ImGui::DragFloat("Intro Spawn Base Height", &m_introSpawnBaseHeight, 0.1f, -5.0f, 30.0f);
            ImGui::DragFloat("Intro Height Jitter", &m_introSpawnHeightJitter, 0.1f, 0.0f, 20.0f);
            ImGui::DragFloat("Intro Front Spread Deg", &m_introFrontSpreadDegree, 1.0f, 0.0f, 85.0f);
            ImGui::DragInt("Intro Direction Mode (0 Front / 1 Sphere / 2 EightDir)", &m_introDirectionMode, 1, 0, 2);
            ImGui::Text("Intro Active: %s", m_introActive ? "true" : "false");
            ImGui::Text("Intro Assembled: %s", m_introAssembled ? "true" : "false");

            if (ImGui::Button("Trigger Intro Assemble"))
            {
                PrepareIntroAssembly();
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

        j["IntroAssembleDuration"] = m_introAssembleDuration;
        j["IntroStaggerPerPart"] = m_introStaggerPerPart;
        j["IntroStaggerJitter"] = m_introStaggerJitter;
        j["IntroStaggerRandomOrder"] = m_introStaggerRandomOrder;
        j["IntroSpawnRadius"] = m_introSpawnRadius;
        j["IntroSpawnBaseHeight"] = m_introSpawnBaseHeight;
        j["IntroSpawnHeightJitter"] = m_introSpawnHeightJitter;
        j["IntroFrontSpreadDegree"] = m_introFrontSpreadDegree;
        j["IntroDirectionMode"] = m_introDirectionMode;
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

        engine::JsonGet(j, "IntroAssembleDuration", m_introAssembleDuration);
        engine::JsonGet(j, "IntroStaggerPerPart", m_introStaggerPerPart);
        engine::JsonGet(j, "IntroStaggerJitter", m_introStaggerJitter);
        engine::JsonGet(j, "IntroStaggerRandomOrder", m_introStaggerRandomOrder);
        engine::JsonGet(j, "IntroSpawnRadius", m_introSpawnRadius);
        engine::JsonGet(j, "IntroSpawnBaseHeight", m_introSpawnBaseHeight);
        engine::JsonGet(j, "IntroSpawnHeightJitter", m_introSpawnHeightJitter);
        engine::JsonGet(j, "IntroFrontSpreadDegree", m_introFrontSpreadDegree);
        engine::JsonGet(j, "IntroDirectionMode", m_introDirectionMode);
    }
}