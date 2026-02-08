#include "GamePCH.h"
#include "BossPattern_Meteor.h"

#include <Framework/Asset/Prefab.h>
#include <Framework/Object/Component/Rigidbody.h>

#include "Script/Boss/BossScript.h"
#include "Script/Boss/BossPattern/Components/BossMeteor.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 패턴 생명주기
    // ═══════════════════════════════════════════════════════════════
    void BossPattern_Meteor::Start(BossScript* boss)
    {
        if (!boss) return;

        m_isActive = true;
        m_intervalTimer = 0.0f;

        // 현재 인터벌 설정
        m_currentInterval = boss->GetMeteorInterval();

        // 시작 시 즉시 메테오 생성
        SpawnMeteor(boss);
    }

    void BossPattern_Meteor::Update(BossScript* boss, float deltaTime)
    {
        if (!boss) return;

        // intervalTimer 업데이트
        m_intervalTimer += deltaTime;

        // interval이 지나면 새 메테오 생성 (안전장치: 기존 메테오가 없을 때만)
        if (m_intervalTimer >= m_currentInterval)
        {
            // 안전장치: 메테오가 아직 활성화되어 있다면 스킵
            if (!m_activeMeteor || !m_activeMeteor.Get())
            {
                SpawnMeteor(boss);
                m_currentInterval = boss->GetMeteorInterval();  // 다음 인터벌 갱신
            }

            m_intervalTimer = 0.0f;
        }
    }

    void BossPattern_Meteor::End(BossScript* boss)
    {
        m_isActive = false;
        m_intervalTimer = 0.0f;

        // 활성 메테오 정리
        if (m_activeMeteor && m_activeMeteor.Get())
        {
            m_activeMeteor->Destroy();
            m_activeMeteor = nullptr;
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 메테오 생성 (플레이어 예측 + 정확도 + 클램핑)
    // ═══════════════════════════════════════════════════════════════
    void BossPattern_Meteor::SpawnMeteor(BossScript* boss)
    {
        if (!boss) return;

        // ─────────────────────────────────────────────
        // 1. 플레이어 예측 위치 계산
        // ─────────────────────────────────────────────
        engine::Vector3 predictedPos = CalculatePredictedPosition(boss);

        // ─────────────────────────────────────────────
        // 2. 예측 정확도 적용 (랜덤 오프셋)
        // ─────────────────────────────────────────────
        float accuracy = boss->GetMeteorPredictionAccuracy();  // 0~10
        float maxOffset = accuracy;                            // 0~10m (직접 사용)

        float offsetX = engine::Random::Float(-maxOffset, maxOffset);
        float offsetZ = engine::Random::Float(-maxOffset, maxOffset);

        engine::Vector3 finalFallPos = predictedPos;
        finalFallPos.x += offsetX;
        finalFallPos.z += offsetZ;

        // ─────────────────────────────────────────────
        // 3. XZ 유효 범위 클램핑 (중점 + 범위/2)
        // ─────────────────────────────────────────────
        float centerX = boss->GetMeteorSpawnCenterX();
        float centerZ = boss->GetMeteorSpawnCenterZ();
        float rangeX = boss->GetMeteorValidRangeX();
        float rangeZ = boss->GetMeteorValidRangeZ();

        float halfRangeX = rangeX * 0.5f;
        float halfRangeZ = rangeZ * 0.5f;

        finalFallPos.x = std::clamp(finalFallPos.x, centerX - halfRangeX, centerX + halfRangeX);
        finalFallPos.z = std::clamp(finalFallPos.z, centerZ - halfRangeZ, centerZ + halfRangeZ);
        finalFallPos.y = 0.0f;  // 착지 목표 Y는 항상 0

        // ─────────────────────────────────────────────
        // 4. 메테오 스폰 위치 (X, Z는 finalFallPos, Y는 BossScript 설정값)
        // ─────────────────────────────────────────────
        engine::Vector3 spawnPos = finalFallPos;
        spawnPos.y = boss->GetMeteorSpawnHeight();

        // ─────────────────────────────────────────────
        // 5. 경고 프리팹 생성
        // ─────────────────────────────────────────────
        auto warningGO = engine::Prefab::Instantiate("BossMeteorWarning");
        if (warningGO)
        {
            engine::Vector3 warningPos = finalFallPos;
            warningPos.y = 0.01f;
            warningGO->GetTransform()->SetLocalPosition(warningPos);
        }

        // ─────────────────────────────────────────────
        // 6. 메테오 프리팹 생성
        // ─────────────────────────────────────────────
        auto meteorGO = engine::Prefab::Instantiate("BossMeteorProjectile");
        if (!meteorGO) return;

        meteorGO->GetTransform()->SetLocalPosition(spawnPos);

        // ─────────────────────────────────────────────
        // 7. BossMeteor 스크립트 Setup
        // ─────────────────────────────────────────────
        auto* meteorScript = meteorGO->GetComponent<BossMeteor>();
        if (meteorScript)
        {
            meteorScript->Setup(boss, finalFallPos, warningGO);
        }

        // ─────────────────────────────────────────────
        // 8. 활성 메테오 추적 (안전장치)
        // ─────────────────────────────────────────────
        m_activeMeteor = engine::Ptr<engine::GameObject>(meteorGO);
    }

    // ═══════════════════════════════════════════════════════════════
    // 플레이어 예측 위치 계산
    // ═══════════════════════════════════════════════════════════════
    engine::Vector3 BossPattern_Meteor::CalculatePredictedPosition(BossScript* boss) const
    {
        if (!boss) return engine::Vector3::Zero;

        auto player = boss->GetTargetPlayer();
        if (!player) return engine::Vector3::Zero;

        // 플레이어 현재 위치
        engine::Vector3 playerPos = player->GetTransform()->GetWorldPosition();

        // 플레이어 Rigidbody 속도
        auto* playerRb = player->GetGameObject()->GetComponent<engine::Rigidbody>();
        if (!playerRb)
        {
            // Rigidbody 없으면 예측 없이 현재 위치 반환
            return playerPos;
        }

        engine::Vector3 playerVelocity = playerRb->GetLinearVelocity();

        // 메테오 낙하 시간 예측 (초기 속도 + 중력 고려)
        float initialSpeed = boss->GetMeteorInitialSpeed();
        float ownGravity = boss->GetMeteorOwnGravity();
        float fallHeight = boss->GetMeteorSpawnHeight();  // BossScript 설정값
        float landingY = boss->GetMeteorLandingY();
        float totalFallDistance = fallHeight - landingY;

        // 평균 낙하 시간 예측 (등가속도 운동)
        // d = v0*t + 0.5*a*t^2
        // 단순화: t ≈ d / (v0 + 0.5*a*t) → 근사치 사용
        float avgSpeed = initialSpeed + 0.5f * ownGravity * (totalFallDistance / (initialSpeed + 0.01f));
        float fallTime = totalFallDistance / (avgSpeed + 0.01f);

        // 플레이어 XZ 예측 오프셋 계산
        engine::Vector3 predictionOffset = playerVelocity * fallTime;
        
        // 예측 오프셋 크기 제한 (대시 등으로 인한 과도한 예측 방지)
        // - 플레이어 기본 이동속도 기준으로 최대 예측 거리 제한
        // - 예: moveSpeed=13, fallTime=2 → 최대 26m까지만 예측
        float baseMoveSpeed = player->GetMoveSpeed();
        float maxPredictionMagnitude = baseMoveSpeed * fallTime;
        float currentMagnitude = predictionOffset.Length();
        if (currentMagnitude > maxPredictionMagnitude)
        {
            // 방향은 유지하되 크기만 제한
            predictionOffset = predictionOffset * (maxPredictionMagnitude / currentMagnitude);
        }
        
        // 예측 강도 스케일 적용 (0.0 = 현재 위치, 1.0 = 풀 예측)
        float predictionStrength = boss->GetMeteorPredictionStrength();
        engine::Vector3 predictedPos = playerPos + predictionOffset * predictionStrength;
        predictedPos.y = 0.0f;  // Y는 무시

        return predictedPos;
    }
}
