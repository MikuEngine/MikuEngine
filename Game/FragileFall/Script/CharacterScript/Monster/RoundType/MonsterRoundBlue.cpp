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
        if (m_meshType == RoundMeshType::Skeletal)
        {
            if (auto* meshRenderer = GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>())
            {
                meshRenderer->SetBaseColor(engine::Vector4(0.0f, 0.5f, 1.0f, 1.0f));
            }
        }
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
                return;
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
        // 충돌 노말 추출 (EngageCollision용)
        // ─────────────────────────────────────────────
        engine::Vector3 collisionNormal = engine::Vector3::Zero;
        for (const auto& contact : info.contacts)
        {
            engine::Vector3 normal = -contact.normal;  // 반전: 몬스터 입장
            normal.y = 0.0f;
            if (normal.LengthSquared() > 0.0001f)
            {
                collisionNormal = normal;
                collisionNormal.Normalize();
                break;
            }
        }
        
        // 노말이 없으면 위치 기반 계산
        if (collisionNormal.LengthSquared() < 0.0001f)
        {
            engine::Vector3 myPos = GetTransform()->GetWorldPosition();
            engine::Vector3 otherPos = info.gameObject->GetTransform()->GetWorldPosition();
            collisionNormal = otherPos - myPos;
            collisionNormal.y = 0.0f;
            if (collisionNormal.LengthSquared() > 0.0001f)
            {
                collisionNormal.Normalize();
            }
        }
        
        // ─────────────────────────────────────────────
        // 상태별 충돌 처리
        // ─────────────────────────────────────────────
        std::string currentState = GetCurrentState();
        
        if (currentState == "IdleMove")
        {
            if (layer == engine::PhysicsLayer::Index::Environment)
            {
                return;
            }
            m_collisionOccurred = true;
        }
        else if (currentState == "EngageMove")
        {
            if (layer == engine::PhysicsLayer::Index::Environment)
            {
                return;
            }
            
            if (layer == engine::PhysicsLayer::Index::Player ||
                layer == engine::PhysicsLayer::Index::Wall)
            {
                m_engageCollisionOccurred = true;
                m_lastCollisionNormal = collisionNormal;  // 충돌 노말 저장
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
        AddFSMState("Idle", true);
        AddFSMState("IdleMove", false);
        AddFSMState("EngageMove", false);
        AddFSMState("EngageCollision", false);   // 충돌로 돌진 종료 → 회전+감속
        AddFSMState("EngageArrival", false);     // 목표 도달로 돌진 종료 → 직진+감속
        AddFSMState("Fragile", false);
        AddFSMState("Dead", false);

        m_logicFSM->Initialize();

        // ─────────────────────────────────────────────
        // 파라미터 정의
        // ─────────────────────────────────────────────
        m_logicFSM->SetParameter("IdleTimerComplete", false);
        m_logicFSM->SetParameter("PlayerDetected", false);
        m_logicFSM->SetParameter("EngageCollision", false);    // 충돌로 돌진 종료
        m_logicFSM->SetParameter("EngageArrival", false);      // 목표 도달로 돌진 종료
        m_logicFSM->SetParameter("TransitionComplete", false); // Collision/Arrival 완료
        m_logicFSM->SetParameter("Fragile", m_isFragile);
        m_logicFSM->SetParameter("Die", m_isDead);

        // ─────────────────────────────────────────────
        // 전이 정의
        // ─────────────────────────────────────────────
        
        // Idle → IdleMove
        AddFSMTransition("Idle", "IdleMove", "IdleTimerComplete", BoolTrue());
        
        // IdleMove → EngageMove
        AddFSMTransition("IdleMove", "EngageMove", "PlayerDetected", BoolTrue());
        
        // EngageMove → EngageCollision (충돌)
        AddFSMTransition("EngageMove", "EngageCollision", "EngageCollision", BoolTrue());
        
        // EngageMove → EngageArrival (목표 도달)
        AddFSMTransition("EngageMove", "EngageArrival", "EngageArrival", BoolTrue());
        
        // EngageCollision → Idle (1초 후)
        AddFSMTransition("EngageCollision", "Idle", "TransitionComplete", BoolTrue());
        
        // EngageArrival → Idle (1초 후)
        AddFSMTransition("EngageArrival", "Idle", "TransitionComplete", BoolTrue());

        // Fragile 전이
        AddFSMTransition("Idle", "Fragile", "Fragile", Trigger());
        AddFSMTransition("IdleMove", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageMove", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageCollision", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageArrival", "Fragile", "Fragile", Trigger());

        AddFSMTransition("Fragile", "Dead", "Die", Trigger());
        AddFSMTransition("Fragile", "Idle", "Revive", Trigger());

        m_logicFSM->InitializeCurrentState();
    }

    // ═══════════════════════════════════════════════════════════════
    // 입력 처리
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ProcessInput()
    {
        if (!m_logicFSM) return;
        
        if (!m_targetPlayer)
        {
            FindPlayer();
        }
        
        std::string currentState = GetCurrentState();
        if (currentState == "IdleMove")
        {
            if (CanDetectPlayer())
            {
                bool playerInRange = IsPlayerInDetectionRange();
                if (playerInRange)
                {
                    m_logicFSM->SetParameter("PlayerDetected", true);
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태별 비물리 행동 분기 오버라이드
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::UpdateStateBasedBehavior(const std::string& state, float deltaTime)
    {
        // 부모 클래스의 공통 상태 처리
        if (state == "Idle" || state == "IdleMove" || state == "EngageMove" ||
            state == "Fragile" || state == "Dead")
        {
            MonsterRoundType::UpdateStateBasedBehavior(state, deltaTime);
        }
        
        // Blue 전용 상태 처리
        if (state == "EngageCollision")
        {
            ExecuteEngageCollisionBehaviorNonPhysics(deltaTime);
        }
        else if (state == "EngageArrival")
        {
            ExecuteEngageArrivalBehaviorNonPhysics(deltaTime);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태별 물리 행동 분기 오버라이드
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::UpdatePhysicsStateBasedBehavior(const std::string& state)
    {
        // 부모 클래스의 공통 상태 처리
        if (state == "Idle" || state == "IdleMove" || state == "EngageMove" ||
            state == "Fragile" || state == "Dead")
        {
            MonsterRoundType::UpdatePhysicsStateBasedBehavior(state);
        }
        
        // Blue 전용 상태 처리
        if (state == "EngageCollision")
        {
            ExecuteEngageCollisionBehaviorPhysics();
        }
        else if (state == "EngageArrival")
        {
            ExecuteEngageArrivalBehaviorPhysics();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // IdleMove 상태 비물리 행동
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ExecuteIdleMoveBehaviorNonPhysics(float deltaTime)
    {
        UpdatePlayerIgnoreTimer(deltaTime);
        
        if (m_collisionOccurred)
        {
            ChangeDirectionOnCollision();
            ResetRoamingParameters();
            m_collisionOccurred = false;
        }
        
        m_roamingTimer += deltaTime;
        
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
        
        float angleChangeDeg = m_turnScale * fixedDeltaTime * static_cast<float>(m_turnDirection);
        float angleChangeRad = angleChangeDeg * 3.14159265f / 180.0f;
        m_currentAngle += angleChangeRad;
        
        const float TWO_PI = 2.0f * 3.14159265f;
        while (m_currentAngle < 0.0f) m_currentAngle += TWO_PI;
        while (m_currentAngle >= TWO_PI) m_currentAngle -= TWO_PI;
        
        engine::Vector3 direction = GetDirectionVector();
        MoveInDirection(direction, m_moveSpeed);
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageMove 상태 비물리 행동
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ExecuteEngageMoveBehaviorNonPhysics(float deltaTime)
    {
        if (!m_logicFSM) return;
        
        // 충돌 발생 시 → EngageCollision으로 전이
        if (m_engageCollisionOccurred)
        {
            m_logicFSM->SetParameter("EngageCollision", true);
            m_engageCollisionOccurred = false;
            return;
        }
        
        // 목표 도달 시 → EngageArrival로 전이
        if (HasReachedEngageTarget())
        {
            m_logicFSM->SetParameter("EngageArrival", true);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageMove 상태 물리 행동
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ExecuteEngageMoveBehaviorPhysics()
    {
        if (!m_rigidbody) return;
        if (!m_hasEngageTarget) return;
        
        MoveInDirection(m_engageDirection, m_engageMoveSpeed);
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageCollision 상태 비물리 행동
    // - 1초간 회전하며 감속
    // - 플레이어 무시 (항상)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ExecuteEngageCollisionBehaviorNonPhysics(float deltaTime)
    {
        if (!m_logicFSM) return;
        
        m_engageTransitionTimer += deltaTime;
        
        if (m_engageTransitionTimer >= m_engageTransitionDuration)
        {
            m_logicFSM->SetParameter("TransitionComplete", true);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageCollision 상태 물리 행동
    // - 초기: 충돌 반대방향에서 45도 꺾인 방향
    // - 1초간: 같은 방향으로 30도 추가 회전
    // - 속도: engageSpeed → moveSpeed 선형 감속
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ExecuteEngageCollisionBehaviorPhysics()
    {
        if (!m_rigidbody) return;
        
        float t = m_engageTransitionTimer / m_engageTransitionDuration;
        t = std::min(t, 1.0f);
        
        // 각도 계산: 초기각도에서 30도 추가 회전
        float rotationRad = (m_collisionRotationAmount * t) * 3.14159265f / 180.0f;
        float currentAngle = m_collisionStartAngle + rotationRad * static_cast<float>(m_collisionTurnDirection);
        
        // 방향 벡터 계산
        engine::Vector3 direction(
            std::cos(currentAngle),
            0.0f,
            std::sin(currentAngle)
        );
        
        // 속도 계산: 선형 감속
        float currentSpeed = CalculateTransitionSpeed();
        
        MoveInDirection(direction, currentSpeed);
        
        // IdleMove 진입을 위해 m_currentAngle 업데이트
        m_currentAngle = currentAngle;
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageArrival 상태 비물리 행동
    // - 1초간 직진하며 감속
    // - 플레이어 무시 (항상)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ExecuteEngageArrivalBehaviorNonPhysics(float deltaTime)
    {
        if (!m_logicFSM) return;
        
        m_engageTransitionTimer += deltaTime;
        
        if (m_engageTransitionTimer >= m_engageTransitionDuration)
        {
            m_logicFSM->SetParameter("TransitionComplete", true);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageArrival 상태 물리 행동
    // - 대쉬 방향 유지
    // - 속도: engageSpeed → moveSpeed 선형 감속
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ExecuteEngageArrivalBehaviorPhysics()
    {
        if (!m_rigidbody) return;
        
        float currentSpeed = CalculateTransitionSpeed();
        
        MoveInDirection(m_engageDirection, currentSpeed);
    }

    // ═══════════════════════════════════════════════════════════════
    // IdleMove 초기화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::InitializeIdleMove()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265f);
        m_currentAngle = angleDist(gen);
        
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
        
        std::uniform_real_distribution<float> durationDist(m_roamingDurationMin, m_roamingDurationMax);
        m_roamingDuration = durationDist(gen);
        
        std::uniform_int_distribution<int> dirDist(0, 1);
        m_turnDirection = (dirDist(gen) == 0) ? 1 : -1;
        
        std::uniform_real_distribution<float> turnDist(m_turnScaleMin, m_maxTurnScale);
        m_turnScale = turnDist(gen);
        
        m_roamingTimer = 0.0f;
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 시 방향 전환 (90~180도 랜덤)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ChangeDirectionOnCollision()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        std::uniform_int_distribution<int> dirDist(0, 1);
        bool turnLeft = (dirDist(gen) == 0);
        
        std::uniform_real_distribution<float> angleDist(90.0f, 180.0f);
        float angleChangeDeg = angleDist(gen);
        float angleChangeRad = angleChangeDeg * 3.14159265f / 180.0f;
        
        if (turnLeft)
        {
            m_currentAngle += angleChangeRad;
        }
        else
        {
            m_currentAngle -= angleChangeRad;
        }
        
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
    // EngageMove 초기화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::InitializeEngageMove()
    {
        m_hasEngageTarget = false;
        m_engageCollisionOccurred = false;
        m_engageArrivalOccurred = false;
        
        if (!m_targetPlayer || !m_targetPlayer->GetGameObject())
        {
            return;
        }
        
        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();
        
        engine::Vector3 direction = playerPos - myPos;
        direction.y = 0.0f;
        
        float distance = direction.Length();
        if (distance < 0.001f)
        {
            return;
        }
        
        direction.Normalize();
        m_engageDirection = direction;
        
        float targetDistance = distance * m_engageTargetMultiplier;
        m_engageTargetPosition = myPos + direction * targetDistance;
        
        m_hasEngageTarget = true;
    }

    // ═══════════════════════════════════════════════════════════════
    // 목표 도달 여부 확인
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundBlue::HasReachedEngageTarget() const
    {
        if (!m_hasEngageTarget) return true;
        
        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        engine::Vector3 diff = m_engageTargetPosition - myPos;
        diff.y = 0.0f;
        
        float distSq = diff.LengthSquared();
        float thresholdSq = m_engageArrivalThreshold * m_engageArrivalThreshold;
        
        return distSq <= thresholdSq;
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageCollision 초기화
    // - 충돌 반대방향에서 45도 꺾인 방향으로 시작
    // - 좌/우 50% 랜덤
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::InitializeEngageCollision()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        m_engageTransitionTimer = 0.0f;
        
        // 충돌 반대방향 계산
        engine::Vector3 oppositeDir = -m_lastCollisionNormal;
        if (oppositeDir.LengthSquared() < 0.0001f)
        {
            // 유효하지 않으면 현재 진행방향 반대
            oppositeDir = -m_engageDirection;
        }
        oppositeDir.Normalize();
        
        // 반대방향의 각도 계산
        float baseAngle = std::atan2(oppositeDir.z, oppositeDir.x);
        
        // 좌/우 50% 랜덤
        std::uniform_int_distribution<int> dirDist(0, 1);
        m_collisionTurnDirection = (dirDist(gen) == 0) ? 1 : -1;
        
        // 초기 45도 오프셋 적용
        float offsetRad = m_collisionInitialAngleOffset * 3.14159265f / 180.0f;
        m_collisionStartAngle = baseAngle + offsetRad * static_cast<float>(m_collisionTurnDirection);
        
        // 각도 정규화
        const float TWO_PI = 2.0f * 3.14159265f;
        while (m_collisionStartAngle < 0.0f) m_collisionStartAngle += TWO_PI;
        while (m_collisionStartAngle >= TWO_PI) m_collisionStartAngle -= TWO_PI;
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageArrival 초기화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::InitializeEngageArrival()
    {
        m_engageTransitionTimer = 0.0f;
        // m_engageDirection은 이미 설정되어 있음
    }

    // ═══════════════════════════════════════════════════════════════
    // 감속 중 현재 속도 계산 (선형 보간)
    // ═══════════════════════════════════════════════════════════════
    float MonsterRoundBlue::CalculateTransitionSpeed() const
    {
        float t = m_engageTransitionTimer / m_engageTransitionDuration;
        t = std::min(t, 1.0f);
        
        // 선형 감속: engageSpeed → moveSpeed
        return m_engageMoveSpeed + (m_moveSpeed - m_engageMoveSpeed) * t;
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
    // - EngageCollision/EngageArrival 상태에서는 항상 무시
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundBlue::CanDetectPlayer() const
    {
        std::string state = GetCurrentState();
        if (state == "EngageCollision" || state == "EngageArrival")
        {
            return false;  // 항상 무시
        }
        return !m_isIgnoringPlayer;
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태 진입 콜백 오버라이드
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::OnStateEntered(const std::string& state)
    {
        MonsterRoundType::OnStateEntered(state);
        
        if (state == "Idle")
        {
            // Idle 진입 시 플레이어 무시 시작
            StartPlayerIgnore();
            
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("PlayerDetected", false);
                m_logicFSM->SetParameter("EngageCollision", false);
                m_logicFSM->SetParameter("EngageArrival", false);
                m_logicFSM->SetParameter("TransitionComplete", false);
            }
        }
        else if (state == "IdleMove")
        {
            // EngageCollision/Arrival에서 온 경우: m_currentAngle 유지 (이미 설정됨)
            // Idle에서 온 경우: 랜덤 방향
            // 구분을 위해 항상 ResetRoamingParameters만 호출
            ResetRoamingParameters();
            
            m_roamingTimer = 0.0f;
            m_collisionOccurred = false;
            
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("PlayerDetected", false);
            }
        }
        else if (state == "EngageMove")
        {
            InitializeEngageMove();
            
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("EngageCollision", false);
                m_logicFSM->SetParameter("EngageArrival", false);
            }
        }
        else if (state == "EngageCollision")
        {
            InitializeEngageCollision();
            
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("TransitionComplete", false);
            }
        }
        else if (state == "EngageArrival")
        {
            InitializeEngageArrival();
            
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("TransitionComplete", false);
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
        
        MonsterRoundType::OnGui();
        
        // Roaming 설정
        ImGui::Separator();
        ImGui::Text("=== Blue Roaming Settings ===");
        ImGui::DragFloat("Roaming Duration Min", &m_roamingDurationMin, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Roaming Duration Max", &m_roamingDurationMax, 0.1f, 0.1f, 10.0f);
        
        ImGui::Separator();
        ImGui::Text("=== Blue Turn Scale Settings ===");
        ImGui::DragFloat("Turn Scale Min (deg/s)", &m_turnScaleMin, 1.0f, 1.0f, 90.0f);
        ImGui::DragFloat("Turn Scale Max (deg/s)", &m_maxTurnScale, 1.0f, 1.0f, 180.0f);
        
        // EngageMove 설정
        ImGui::Separator();
        ImGui::Text("=== Blue EngageMove Settings ===");
        ImGui::DragFloat("Engage Move Speed", &m_engageMoveSpeed, 0.5f, 1.0f, 30.0f);
        ImGui::DragFloat("Player Ignore Duration", &m_playerIgnoreDuration, 0.1f, 0.1f, 10.0f);
        
        // Damage 설정
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
        
        // 런타임 정보 - Transition
        ImGui::Separator();
        ImGui::Text("=== Blue Runtime (Transition) ===");
        ImGui::Text("Transition Timer: %.2f / %.2f", m_engageTransitionTimer, m_engageTransitionDuration);
        ImGui::Text("Current Speed: %.2f", CalculateTransitionSpeed());
    }

    void MonsterRoundBlue::Save(engine::json& j) const
    {
        MonsterRoundType::Save(j);
        
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
        
        m_roamingDurationMin = j.value("RoamingDurationMin", 1.0f);
        m_roamingDurationMax = j.value("RoamingDurationMax", 3.0f);
        m_turnScaleMin = j.value("TurnScaleMin", 10.0f);
        m_maxTurnScale = j.value("MaxTurnScale", 45.0f);
        m_damageCooldown = j.value("DamageCooldown", 1.0f);
        m_engageMoveSpeed = j.value("EngageMoveSpeed", 10.0f);
        m_playerIgnoreDuration = j.value("PlayerIgnoreDuration", 1.0f);
    }
}
