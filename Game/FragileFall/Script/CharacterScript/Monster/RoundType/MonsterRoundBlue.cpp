#include "GamePCH.h"
#include "MonsterRoundBlue.h"

#include "Script/CharacterScript/Player/PlayerControllerScript.h"

#include <Framework/Object/Component/Renderer/SkeletalMeshRenderer.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Physics/PhysicsLayer.h>
#include <Engine/Core/System/MyTime.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::Awake()
    {
        MonsterRoundType::Awake();
        
        // Blue 등급 고정
        m_monsterTier = MonsterTier::Blue;
    }

    void MonsterRoundBlue::Start()
    {
        MonsterRoundType::Start();

        // Blue 등급 색상 설정 (파란색)
        // SkeletalMeshRenderer만 SetBaseColor 지원
        if (m_meshType == RoundMeshType::Skeletal)
        {
            if (auto* meshRenderer = GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>())
            {
                meshRenderer->SetBaseColor(engine::Vector4(0.0f, 0.5f, 1.0f, 1.0f));
            }
        }
        // StaticMeshRenderer는 현재 SetBaseColor 미지원
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::OnCollisionEnter(const engine::CollisionInfo& info)
    {
        if (!info.gameObject) return;
        
        // 총알 레이어 무시 (Projectile, EnemyProjectile)
        auto* collider = info.collider.Get();
        uint32_t layer = 0;
        if (collider)
        {
            layer = collider->GetLayer();
            if (layer == engine::PhysicsLayer::Index::Projectile ||
                layer == engine::PhysicsLayer::Index::EnemyProjectile)
            {
                return;  // 총알 충돌 무시
            }
        }
        
        // ─────────────────────────────────────────────
        // 플레이어 충돌 시 데미지 처리 (Fragile/Dead가 아닌 모든 상태)
        // ─────────────────────────────────────────────
        if (!m_isFragile && !m_isDead)
        {
            auto* player = info.gameObject->GetComponent<PlayerControllerScript>();
            if (player)
            {
                float elapsedSinceLastDamage = engine::Time::GetElapsedSeconds(m_lastDamageTime);
                if (elapsedSinceLastDamage >= m_damageCooldown)
                {
                    player->TakeDamage(static_cast<int>(m_attackDamage));
                    m_lastDamageTime = engine::Time::GetTimestamp();
                }
            }
        }
        
        // ─────────────────────────────────────────────
        // 상태별 충돌 처리
        // ─────────────────────────────────────────────
        std::string currentState = GetCurrentState();
        
        if (currentState == "IdleMove")
        {
            // Environment 레이어 무시
            if (layer == engine::PhysicsLayer::Index::Environment)
            {
                return;
            }
            // 그 외 충돌 → 방향 전환
            m_collisionOccurred = true;
        }
        else if (currentState == "EngageMove")
        {
            // Environment 레이어 무시
            if (layer == engine::PhysicsLayer::Index::Environment)
            {
                return;
            }
            
            // Player 또는 Wall과 충돌 → Idle로 전이
            if (layer == engine::PhysicsLayer::Index::Player ||
                layer == engine::PhysicsLayer::Index::Wall)
            {
                m_engageCollisionOccurred = true;
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // Blue 전용 FSM 초기화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::InitializeFSM()
    {
        if (!m_logicFSM) return;

        // ─────────────────────────────────────────────
        // 상태 정의
        // ─────────────────────────────────────────────
        AddFSMState("Idle", true);           // 기본 상태 (대기 + 플레이어 무시)
        AddFSMState("IdleMove", false);      // 곡선 배회
        AddFSMState("EngageMove", false);    // 플레이어에게 돌진
        AddFSMState("Fragile", false);
        AddFSMState("Dead", false);

        // ─────────────────────────────────────────────
        // StateMap 업데이트 (Transition 추가 전에 필수!)
        // ─────────────────────────────────────────────
        m_logicFSM->Initialize();

        // ─────────────────────────────────────────────
        // 파라미터 정의
        // ─────────────────────────────────────────────
        m_logicFSM->SetParameter("IdleTimerComplete", false);     // Idle 대기 시간 완료
        m_logicFSM->SetParameter("PlayerDetected", false);        // 플레이어 감지 (무시 시간 이후)
        m_logicFSM->SetParameter("EngageComplete", false);        // 돌진 완료 (충돌 or 목표 도달)
        m_logicFSM->SetParameter("Fragile", m_isFragile);
        m_logicFSM->SetParameter("Die", m_isDead);

        // ─────────────────────────────────────────────
        // 전이 정의
        // ─────────────────────────────────────────────
        
        // Idle → IdleMove (대기 시간 완료)
        AddFSMTransition("Idle", "IdleMove", "IdleTimerComplete", BoolTrue());
        
        // IdleMove → EngageMove (플레이어 감지)
        AddFSMTransition("IdleMove", "EngageMove", "PlayerDetected", BoolTrue());
        
        // EngageMove → Idle (돌진 완료)
        AddFSMTransition("EngageMove", "Idle", "EngageComplete", BoolTrue());

        // Any → Fragile (HP 0, Fragile 트리거)
        AddFSMTransition("Idle", "Fragile", "Fragile", Trigger());
        AddFSMTransition("IdleMove", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageMove", "Fragile", "Fragile", Trigger());

        // Fragile → Dead (Execution, Die 트리거)
        AddFSMTransition("Fragile", "Dead", "Die", Trigger());
        
        // Fragile → Idle (부활, Revive 트리거)
        AddFSMTransition("Fragile", "Idle", "Revive", Trigger());

        // ─────────────────────────────────────────────
        // 초기 상태 설정
        // ─────────────────────────────────────────────
        m_logicFSM->InitializeCurrentState();
    }

    // ═══════════════════════════════════════════════════════════════
    // 입력 처리 - 플레이어 감지 + 무시 타이머
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ProcessInput()
    {
        if (!m_logicFSM) return;
        
        // 플레이어를 찾지 못했으면 재탐색
        if (!m_targetPlayer)
        {
            FindPlayer();
        }
        
        // IdleMove 상태에서만 플레이어 감지
        std::string currentState = GetCurrentState();
        if (currentState == "IdleMove")
        {
            // 감지 가능 여부 확인 (무시 시간 종료)
            if (CanDetectPlayer())
            {
                // 플레이어가 감지 범위 안에 있는지 확인
                bool playerInRange = IsPlayerInDetectionRange();
                
                if (playerInRange)
                {
                    m_logicFSM->SetParameter("PlayerDetected", true);
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // IdleMove 상태 비물리 행동
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ExecuteIdleMoveBehaviorNonPhysics(float deltaTime)
    {
        // 무시 타이머 업데이트 (Idle에서 시작된 타이머 계속)
        UpdatePlayerIgnoreTimer(deltaTime);
        
        // 충돌 발생 처리
        if (m_collisionOccurred)
        {
            ChangeDirectionOnCollision();
            ResetRoamingParameters();
            m_collisionOccurred = false;
        }
        
        // Roaming 타이머 업데이트
        m_roamingTimer += deltaTime;
        
        // Roaming 지속 시간 완료 → 새로운 파라미터로 갱신
        if (m_roamingTimer >= m_roamingDuration)
        {
            ResetRoamingParameters();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // IdleMove 상태 물리 행동
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ExecuteIdleMoveBehaviorPhysics()
    {
        if (!m_rigidbody) return;

        float fixedDeltaTime = engine::Time::FixedDeltaTime();
        
        // ─────────────────────────────────────────────
        // 1. 방향 회전 (매 FixedUpdate마다 각도 누적)
        // ─────────────────────────────────────────────
        float angleChangeDeg = m_turnScale * fixedDeltaTime * static_cast<float>(m_turnDirection);
        float angleChangeRad = angleChangeDeg * 3.14159265f / 180.0f;
        m_currentAngle += angleChangeRad;
        
        // 각도 정규화 (0 ~ 2π)
        const float TWO_PI = 2.0f * 3.14159265f;
        while (m_currentAngle < 0.0f) m_currentAngle += TWO_PI;
        while (m_currentAngle >= TWO_PI) m_currentAngle -= TWO_PI;
        
        // ─────────────────────────────────────────────
        // 2. 현재 각도로 방향 벡터 계산 및 이동
        // ─────────────────────────────────────────────
        engine::Vector3 direction = GetDirectionVector();
        MoveInDirection(direction, m_moveSpeed);
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageMove 상태 비물리 행동
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ExecuteEngageMoveBehaviorNonPhysics(float deltaTime)
    {
        if (!m_logicFSM) return;
        
        // 충돌 발생 시 (Player/Wall) → Idle로 전이
        if (m_engageCollisionOccurred)
        {
            m_logicFSM->SetParameter("EngageComplete", true);
            m_engageCollisionOccurred = false;
            return;
        }
        
        // 목표 도달 시 → Idle로 전이
        if (HasReachedEngageTarget())
        {
            m_logicFSM->SetParameter("EngageComplete", true);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageMove 상태 물리 행동
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ExecuteEngageMoveBehaviorPhysics()
    {
        if (!m_rigidbody) return;
        if (!m_hasEngageTarget) return;
        
        // 고정 방향으로 돌진
        MoveInDirection(m_engageDirection, m_engageMoveSpeed);
    }

    // ═══════════════════════════════════════════════════════════════
    // IdleMove 초기화 (완전 랜덤 방향)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::InitializeIdleMove()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        // 초기 방향: 완전 랜덤 (0 ~ 360도)
        std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265f);
        m_currentAngle = angleDist(gen);
        
        // Roaming 파라미터 초기화
        ResetRoamingParameters();
        
        m_roamingTimer = 0.0f;
        m_collisionOccurred = false;
    }

    // ═══════════════════════════════════════════════════════════════
    // Roaming 파라미터 재설정
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ResetRoamingParameters()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        // 1. m_roamingDuration 설정
        std::uniform_real_distribution<float> durationDist(m_roamingDurationMin, m_roamingDurationMax);
        m_roamingDuration = durationDist(gen);
        
        // 2. 좌/우 방향 선택 (50% 확률)
        std::uniform_int_distribution<int> dirDist(0, 1);
        m_turnDirection = (dirDist(gen) == 0) ? 1 : -1;
        
        // 3. m_turnScale 설정 (범위 내 랜덤)
        std::uniform_real_distribution<float> turnDist(m_turnScaleMin, m_maxTurnScale);
        m_turnScale = turnDist(gen);
        
        // 타이머 리셋
        m_roamingTimer = 0.0f;
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 시 방향 전환 (90~180도 랜덤)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ChangeDirectionOnCollision()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        // 1. 좌/우 선택 (50% 확률)
        std::uniform_int_distribution<int> dirDist(0, 1);
        bool turnLeft = (dirDist(gen) == 0);
        
        // 2. 90~180도 사이 랜덤 각도 (라디안)
        std::uniform_real_distribution<float> angleDist(90.0f, 180.0f);
        float angleChangeDeg = angleDist(gen);
        float angleChangeRad = angleChangeDeg * 3.14159265f / 180.0f;
        
        // 3. 방향 적용
        if (turnLeft)
        {
            m_currentAngle += angleChangeRad;
        }
        else
        {
            m_currentAngle -= angleChangeRad;
        }
        
        // 각도 정규화 (0 ~ 2π)
        const float TWO_PI = 2.0f * 3.14159265f;
        while (m_currentAngle < 0.0f) m_currentAngle += TWO_PI;
        while (m_currentAngle >= TWO_PI) m_currentAngle -= TWO_PI;
    }

    // ═══════════════════════════════════════════════════════════════
    // 현재 각도 → 방향 벡터 변환
    // ═══════════════════════════════════════════════════════════════
    engine::Vector3 MonsterRoundBlue::GetDirectionVector() const
    {
        return engine::Vector3(
            std::cos(m_currentAngle),
            0.0f,
            std::sin(m_currentAngle)
        );
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageMove 초기화 - 고정 목표 위치 계산
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::InitializeEngageMove()
    {
        m_hasEngageTarget = false;
        m_engageCollisionOccurred = false;
        
        if (!m_targetPlayer || !m_targetPlayer->GetGameObject())
        {
            return;
        }
        
        // 현재 위치
        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        
        // 플레이어 위치 (진입 시점 고정)
        engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();
        
        // 방향 벡터 계산
        engine::Vector3 direction = playerPos - myPos;
        direction.y = 0.0f;  // 수평 이동만
        
        float distance = direction.Length();
        if (distance < 0.001f)
        {
            return;  // 너무 가까움
        }
        
        // 정규화
        direction.Normalize();
        m_engageDirection = direction;
        
        // 목표 위치: 플레이어 방향으로 거리 * 배율만큼
        float targetDistance = distance * m_engageTargetMultiplier;
        m_engageTargetPosition = myPos + direction * targetDistance;
        
        m_hasEngageTarget = true;
    }

    // ═══════════════════════════════════════════════════════════════
    // 목표 도달 여부 확인
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundBlue::HasReachedEngageTarget() const
    {
        if (!m_hasEngageTarget) return true;  // 목표 없으면 도달로 처리
        
        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        engine::Vector3 diff = m_engageTargetPosition - myPos;
        diff.y = 0.0f;  // 수평 거리만
        
        float distSq = diff.LengthSquared();
        float thresholdSq = m_engageArrivalThreshold * m_engageArrivalThreshold;
        
        return distSq <= thresholdSq;
    }

    // ═══════════════════════════════════════════════════════════════
    // 플레이어 무시 시작 (Idle 진입 시)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::StartPlayerIgnore()
    {
        m_isIgnoringPlayer = true;
        m_playerIgnoreTimer = 0.0f;
    }

    // ═══════════════════════════════════════════════════════════════
    // 무시 타이머 업데이트
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::UpdatePlayerIgnoreTimer(float deltaTime)
    {
        if (m_isIgnoringPlayer)
        {
            m_playerIgnoreTimer += deltaTime;
            
            if (m_playerIgnoreTimer >= m_playerIgnoreDuration)
            {
                m_isIgnoringPlayer = false;
                m_playerIgnoreTimer = 0.0f;
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 플레이어 감지 가능 여부
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundBlue::CanDetectPlayer() const
    {
        return !m_isIgnoringPlayer;
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태 진입 콜백 오버라이드
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::OnStateEntered(const std::string& state)
    {
        // 부모 클래스 호출
        MonsterRoundType::OnStateEntered(state);
        
        if (state == "Idle")
        {
            // Idle 진입 시 플레이어 무시 시작
            StartPlayerIgnore();
            
            // FSM 파라미터 초기화
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("PlayerDetected", false);
                m_logicFSM->SetParameter("EngageComplete", false);
            }
        }
        else if (state == "IdleMove")
        {
            InitializeIdleMove();
            
            // FSM 파라미터 초기화
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("PlayerDetected", false);
            }
        }
        else if (state == "EngageMove")
        {
            InitializeEngageMove();
            
            // FSM 파라미터 초기화
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("EngageComplete", false);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::OnGui()
    {
        ImGui::Text("=== MonsterRoundBlue ===");
        ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "Tier: Blue (Curved + Dash)");
        
        // 부모 클래스 OnGui 호출
        MonsterRoundType::OnGui();
        
        // Blue 전용 설정 - Roaming
        ImGui::Separator();
        ImGui::Text("=== Blue Roaming Settings ===");
        ImGui::DragFloat("Roaming Duration Min", &m_roamingDurationMin, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Roaming Duration Max", &m_roamingDurationMax, 0.1f, 0.1f, 10.0f);
        
        ImGui::Separator();
        ImGui::Text("=== Blue Turn Scale Settings ===");
        ImGui::DragFloat("Turn Scale Min (deg/s)", &m_turnScaleMin, 1.0f, 1.0f, 90.0f);
        ImGui::DragFloat("Turn Scale Max (deg/s)", &m_maxTurnScale, 1.0f, 1.0f, 180.0f);
        
        // Blue 전용 설정 - EngageMove
        ImGui::Separator();
        ImGui::Text("=== Blue EngageMove Settings ===");
        ImGui::DragFloat("Engage Move Speed", &m_engageMoveSpeed, 0.5f, 1.0f, 30.0f);
        ImGui::DragFloat("Player Ignore Duration", &m_playerIgnoreDuration, 0.1f, 0.1f, 10.0f);
        
        ImGui::Separator();
        ImGui::Text("=== Blue Damage Settings ===");
        ImGui::DragFloat("Damage Cooldown", &m_damageCooldown, 0.1f, 0.1f, 5.0f);
        
        // 런타임 정보 - IdleMove
        ImGui::Separator();
        ImGui::Text("=== Blue Runtime (IdleMove) ===");
        float angleDeg = m_currentAngle * 180.0f / 3.14159265f;
        ImGui::Text("Current Angle: %.1f deg", angleDeg);
        ImGui::Text("Turn Direction: %s", (m_turnDirection > 0) ? "Left (CCW)" : "Right (CW)");
        ImGui::Text("Turn Scale: %.1f deg/s", m_turnScale);
        ImGui::Text("Roaming Timer: %.2f / %.2f", m_roamingTimer, m_roamingDuration);
        
        engine::Vector3 dir = GetDirectionVector();
        ImGui::Text("Direction Vector: (%.2f, %.2f, %.2f)", dir.x, dir.y, dir.z);
        
        // 런타임 정보 - 플레이어 무시
        ImGui::Separator();
        ImGui::Text("=== Blue Runtime (Player Ignore) ===");
        ImGui::Text("Ignoring Player: %s", m_isIgnoringPlayer ? "Yes" : "No");
        if (m_isIgnoringPlayer)
        {
            ImGui::Text("Ignore Timer: %.2f / %.2f", m_playerIgnoreTimer, m_playerIgnoreDuration);
        }
        
        // 런타임 정보 - EngageMove
        ImGui::Separator();
        ImGui::Text("=== Blue Runtime (EngageMove) ===");
        ImGui::Text("Has Target: %s", m_hasEngageTarget ? "Yes" : "No");
        if (m_hasEngageTarget)
        {
            ImGui::Text("Target: (%.2f, %.2f, %.2f)", 
                m_engageTargetPosition.x, m_engageTargetPosition.y, m_engageTargetPosition.z);
            ImGui::Text("Direction: (%.2f, %.2f, %.2f)", 
                m_engageDirection.x, m_engageDirection.y, m_engageDirection.z);
        }
    }

    void MonsterRoundBlue::Save(engine::json& j) const
    {
        MonsterRoundType::Save(j);
        
        // Blue 전용 데이터 저장
        j["RoamingDurationMin"] = m_roamingDurationMin;
        j["RoamingDurationMax"] = m_roamingDurationMax;
        j["TurnScaleMin"] = m_turnScaleMin;
        j["MaxTurnScale"] = m_maxTurnScale;
        j["DamageCooldown"] = m_damageCooldown;
        j["EngageMoveSpeed"] = m_engageMoveSpeed;
        j["PlayerIgnoreDuration"] = m_playerIgnoreDuration;
    }

    void MonsterRoundBlue::Load(const engine::json& j)
    {
        MonsterRoundType::Load(j);
        
        // Blue 전용 데이터 로드
        m_roamingDurationMin = j.value("RoamingDurationMin", 1.0f);
        m_roamingDurationMax = j.value("RoamingDurationMax", 3.0f);
        m_turnScaleMin = j.value("TurnScaleMin", 10.0f);
        m_maxTurnScale = j.value("MaxTurnScale", 45.0f);
        m_damageCooldown = j.value("DamageCooldown", 1.0f);
        m_engageMoveSpeed = j.value("EngageMoveSpeed", 10.0f);
        m_playerIgnoreDuration = j.value("PlayerIgnoreDuration", 1.0f);
    }
}
