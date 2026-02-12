#include "GamePCH.h"
#include "MonsterRoundBlue.h"

#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/CharacterScript/Common/BulletFactory.h"

#include <Framework/Object/Component/Renderer/SkeletalMeshRenderer.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Object/Component/Pathfinding/PathfindingAgent.h>
#include <Framework/Object/Component/Pathfinding/GridMap.h>
#include <Framework/System/SystemManager.h>
#include <Framework/System/PathfindingSystem.h>
#include <Framework/Physics/PhysicsLayer.h>
#include <Framework/Physics/PhysicsSystem.h>
#include <Engine/Core/System/MyTime.h>
#include <random>


namespace game
{
    namespace
    {
        engine::Vector3 SnapNormalToAxisForDiagonal(const engine::Vector3& normal)
        {
            const float absX = std::abs(normal.x);
            const float absZ = std::abs(normal.z);

            if (absX >= absZ)
            {
                return engine::Vector3((normal.x >= 0.0f) ? 1.0f : -1.0f, 0.0f, 0.0f);
            }
            return engine::Vector3(0.0f, 0.0f, (normal.z >= 0.0f) ? 1.0f : -1.0f);
        }

        engine::Vector3 ReflectDirectionByNormal(const engine::Vector3& direction, const engine::Vector3& normal)
        {
            const float dot = direction.Dot(normal);
            return direction - 2.0f * dot * normal;
        }
    }

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

        if (auto* collider = GetGameObject()->GetComponent<engine::Collider>())
        {
            collider->SetLayer(engine::PhysicsLayer::Index::EnemyNonPath);
        }

        // Blue 등급 색상 설정 (파란색)
        if (m_meshType == RoundMeshType::Skeletal)
        {
            if (auto* meshRenderer = GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>())
            {
                meshRenderer->SetBaseColor(engine::Vector4(0.0f, 0.5f, 1.0f, 1.0f));
            }
        }
        
        // PathfindingAgent 설정 (IdleMove용)
        if (m_pathfindingAgent)
        {
            m_pathfindingAgent->SetPathUpdateInterval(0.5f);
            m_pathfindingAgent->SetWaypointReachDistance(1.0f);
            m_pathfindingAgent->SetTargetMoveThreshold(1.0f);
        }
        
        // GridMap 캐싱 (목표 위치 안전성 체크용)
        auto& pathfindingSystem = engine::SystemManager::Get().GetPathfindingSystem();
        m_gridMap = pathfindingSystem.GetGridMap();
        
        // 게임 시작 시 플레이어 무시 상태로 초기화
        // (InitializeCurrentState는 OnStateEntered를 호출하지 않으므로 수동 설정 필요)
        StartPlayerIgnore();

        if (GetTransform())
        {
            m_idleMoveLastPosition = GetTransform()->GetWorldPosition();
        }
        m_idleMoveStuckTimer = 0.0f;
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::OnCollisionEnter(const engine::CollisionInfo& info)
    {
        if (!info.gameObject) return;
        
        // ─────────────────────────────────────────────
        auto* collider = info.collider.Get();
        uint32_t layer = 0;
        if (collider)
        {
            layer = collider->GetLayer();
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
                    player->TakeDamage(m_attackDamage);
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
        bool isObstacle = (layer == engine::PhysicsLayer::Index::SubWall ||
                          layer == engine::PhysicsLayer::Index::Environment);
        
        if (currentState == "IdleMove")
        {
            // SubWall, Environment과의 충돌만 처리 (Enemy 제외)
            if (isObstacle)
            {
                m_collisionOccurred = true;
                m_lastCollisionNormal = collisionNormal;  // 반사 이동용 노말 저장
            }
        }
        else if (currentState == "EngageMove")
        {
            // EngageMove는 충돌 가능한 모든 오브젝트 충돌 시 종료
            if (collider)
            {
                m_engageCollisionOccurred = true;
                m_lastCollisionNormal = collisionNormal;
            }
        }
        else if (currentState == "EngageCollision" || currentState == "EngageArrival")
        {
            // 전이 상태에서도 충돌 가능한 모든 오브젝트와의 충돌을 재바운드 트리거로 사용
            if (collider)
            {
                m_transitionCollisionOccurred = true;
                m_lastCollisionNormal = collisionNormal;
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
        // 상태 정의 (Fragile 없음)
        // ─────────────────────────────────────────────
        AddFSMState("Idle", true);
        AddFSMState("IdleMove", false);
        AddFSMState("EngageMove", false);
        AddFSMState("EngageCollision", false);   // 충돌로 돌진 종료 → 회전+감속
        AddFSMState("EngageArrival", false);     // 목표 도달로 돌진 종료 → 직진+감속
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

        // Any → Dead (Round 타입은 Fragile 없이 바로 Dead로 전이)
        AddFSMTransition("Idle", "Dead", "Die", Trigger());
        AddFSMTransition("IdleMove", "Dead", "Die", Trigger());
        AddFSMTransition("EngageMove", "Dead", "Die", Trigger());
        AddFSMTransition("EngageCollision", "Dead", "Die", Trigger());
        AddFSMTransition("EngageArrival", "Dead", "Die", Trigger());

        m_logicFSM->InitializeCurrentState();
    }

    // ═══════════════════════════════════════════════════════════════
    // 총알 초기화 (Dead 시 ThreewayFire용)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::InitializeBullet()
    {
        // Blue는 평소에 총알 발사 없음, Dead 시에만 ThreewayFire
        m_bulletParams.type = BulletType::Linear;
        m_bulletParams.speed = m_bulletSpeed;
        m_bulletParams.lifetime = m_bulletLifetime;
        m_bulletParams.damage = m_attackDamage;
        m_bulletParams.scale = m_bulletScale;
        m_bulletParams.explosionRadius = m_explosionRadius;  // 부모 값 복사 (미사용이지만 일관성 유지)
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
                bool playerInRange = CanEnterEngageByRaycast();
                if (playerInRange)
                {
                    // IdleMove 상태 초기화
                    m_hasIdleMoveTarget = false;
                    m_idleMoveTimer = 0.0f;
                    m_isReflecting = false;
                    m_reflectTimer = 0.0f;
                    m_collisionOccurred = false;
                    
                    // PathfindingAgent 비활성화
                    if (m_pathfindingAgent)
                    {
                        m_pathfindingAgent->ClearPath();
                    }
                    
                    // FSM 전환
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
        
        // ─────────────────────────────────────────────
        // 플레이어 감지 체크 (매 프레임)
        // ─────────────────────────────────────────────
        if (!m_isIgnoringPlayer && CanEnterEngageByRaycast())
        {
            // IdleMove 상태 초기화
            m_hasIdleMoveTarget = false;
            m_idleMoveTimer = 0.0f;
            m_isReflecting = false;
            m_reflectTimer = 0.0f;
            m_collisionOccurred = false;
            
            // PathfindingAgent 비활성화
            if (m_pathfindingAgent)
            {
                m_pathfindingAgent->ClearPath();
            }
            
            // FSM 전환
            m_logicFSM->SetParameter("PlayerDetected", true);
            return;  // 즉시 EngageMove로 전환
        }
        
        // 반사 이동 중
        if (m_isReflecting)
        {
            m_reflectTimer += deltaTime;
            
            if (m_reflectTimer >= m_reflectDuration)
            {
                // 반사 이동 완료 → 새 목표 설정
                m_isReflecting = false;
                TrySetIdleMoveTarget();
            }
            return;
        }
        
        // 충돌 발생 시 반사 이동 시작
        if (m_collisionOccurred)
        {
            HandleIdleMoveCollision(m_lastCollisionNormal);
            m_collisionOccurred = false;
            m_idleMoveStuckTimer = 0.0f;
            return;
        }
        
        // 목표가 없거나 완료되었으면 새 목표 설정
        if (!m_hasIdleMoveTarget || IsIdleMoveComplete())
        {
            TrySetIdleMoveTarget();
            return;
        }
        
        // 이동 타이머 업데이트
        m_idleMoveTimer += deltaTime;

        // 1초 이상 유의미한 이동이 없으면 경로를 즉시 재탐색한다.
        // 횟수 제한 없이 계속 재시도 가능하다.
        if (GetTransform())
        {
            const engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
            engine::Vector3 delta = currentPos - m_idleMoveLastPosition;
            delta.y = 0.0f;

            const float minMoveDistSq = m_idleMoveMinMoveDistance * m_idleMoveMinMoveDistance;
            if (delta.LengthSquared() < minMoveDistSq)
            {
                m_idleMoveStuckTimer += deltaTime;
            }
            else
            {
                m_idleMoveStuckTimer = 0.0f;
            }

            m_idleMoveLastPosition = currentPos;

            if (m_idleMoveStuckTimer >= m_idleMoveStuckThreshold)
            {
                m_idleMoveStuckTimer = 0.0f;
                m_hasIdleMoveTarget = false;
                TrySetIdleMoveTarget();
                return;
            }
        }
        
        // waypoint 도달 체크
        if (HasReachedCurrentWaypoint())
        {
            AdvanceToNextWaypoint();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // IdleMove 상태 물리 행동 (진동하며 waypoint로 이동)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ExecuteIdleMoveBehaviorPhysics()
    {
        if (!m_rigidbody) return;
        
        // 반사 이동 중 (거리 기반 속도 계산)
        if (m_isReflecting)
        {
            // 2m를 m_reflectDuration(0.5초) 동안 이동
            float reflectSpeed = m_collisionReflectDistance / m_reflectDuration;
            MoveInDirection(m_reflectDirection, reflectSpeed);
            return;
        }

        // 충돌 감지 시 힘 적용 중단 (벽 관통 방지)
        if (m_collisionOccurred)
        {
            StopAllMovement();
            return;
        }
        
        // 목표가 없으면 정지
        if (!m_hasIdleMoveTarget)
        {
            StopAllMovement();
            return;
        }
        
        float fixedDeltaTime = engine::Time::FixedDeltaTime();
        
        // ─────────────────────────────────────────────
        // 1. waypoint로 향하는 기본 방향 벡터
        // ─────────────────────────────────────────────
        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        engine::Vector3 toWaypoint = m_currentWaypointTarget - myPos;
        toWaypoint.y = 0.0f;
        
        if (toWaypoint.LengthSquared() < 0.001f)
        {
            StopAllMovement();
            return;
        }
        
        toWaypoint.Normalize();
        
        // ─────────────────────────────────────────────
        // 2. 이동 방향에 수직인 벡터 계산 (좌우 진동용)
        // ─────────────────────────────────────────────
        engine::Vector3 perpendicular(-toWaypoint.z, 0.0f, toWaypoint.x);
        
        // ─────────────────────────────────────────────
        // 3. 사인파 진동 계산
        // ─────────────────────────────────────────────
        const float TWO_PI = 2.0f * 3.14159265f;
        
        // 위상 업데이트 (고정 주기: 1Hz)
        m_oscillationPhase += TWO_PI * 1.0f * fixedDeltaTime;
        
        // 위상이 2π를 넘으면 새 파장 시작
        if (m_oscillationPhase >= TWO_PI)
        {
            ResetOscillationPhase();
        }
        
        // 사인파 계산: sin(phase) * amplitude
        float sinValue = std::sin(m_oscillationPhase);
        float lateralOffset = sinValue * m_currentOscillationAmplitude;
        
        // ─────────────────────────────────────────────
        // 4. 전진 + 진동을 독립적으로 계산하여 합성
        // ─────────────────────────────────────────────
        
        // 전진 벡터 (일정한 속도)
        engine::Vector3 forwardVelocity = toWaypoint * m_moveSpeed;
        
        // 진동 벡터 (진폭 → 속도 변환, 배율 적용)
        engine::Vector3 lateralVelocity = perpendicular * lateralOffset * m_oscillationSpeedMultiplier;
        
        // 두 벡터 합성
        engine::Vector3 finalVelocity = forwardVelocity + lateralVelocity;
        finalVelocity.y = 0.0f;
        
        // ─────────────────────────────────────────────
        // 5. 이동 적용 (합성된 속도로)
        // ─────────────────────────────────────────────
        if (finalVelocity.LengthSquared() > 0.001f)
        {
            float totalSpeed = finalVelocity.Length();
            finalVelocity.Normalize();
            MoveInDirection(finalVelocity, totalSpeed);
        }
        else
        {
            // fallback: 전진만
            MoveInDirection(toWaypoint, m_moveSpeed);
        }
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
        
        // 충돌 감지 시 힘 적용 중단 (벽 관통 방지)
        // 상태 전이는 NonPhysics에서 처리됨
        if (m_engageCollisionOccurred)
        {
            StopAllMovement();
            return;
        }
        
        MoveInDirection(m_engageDirection, m_engageMoveSpeedScaled);
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

        // 전이 상태에서 재충돌이 나면 현재 전이 진행 방향을 다시 90도 바운드한다.
        // 감속 타이머는 유지하여 정지까지 이어간다.
        const bool hadTransitionCollision = m_transitionCollisionOccurred;
        if (hadTransitionCollision)
        {
            engine::Vector3 baseDir = m_transitionMoveDirection;
            baseDir.y = 0.0f;
            if (baseDir.LengthSquared() > 0.0001f)
            {
                baseDir.Normalize();
            }
            else
            {
                baseDir = m_engageDirection;
                baseDir.y = 0.0f;
                if (baseDir.LengthSquared() > 0.0001f)
                {
                    baseDir.Normalize();
                }
                else
                {
                    baseDir = engine::Vector3(1.0f, 0.0f, 0.0f);
                }
            }

            engine::Vector3 bouncedDir = baseDir;
            if (m_lastCollisionNormal.LengthSquared() > 0.0001f)
            {
                const engine::Vector3 snappedNormal = SnapNormalToAxisForDiagonal(m_lastCollisionNormal);
                bouncedDir = ReflectDirectionByNormal(baseDir, snappedNormal);
                bouncedDir.y = 0.0f;
            }

            if (bouncedDir.LengthSquared() > 0.0001f)
            {
                bouncedDir.Normalize();
            }
            else
            {
                // 이레귤러 충돌 폴백(요청으로 일단 비활성화)
                // bouncedDir = engine::Vector3(-baseDir.z, 0.0f, baseDir.x);
                bouncedDir = baseDir;
            }

            m_transitionMoveDirection = bouncedDir;
        }

        const float currentSpeed = CalculateTransitionSpeed();
        m_transitionCollisionOccurred = false;
        if (currentSpeed <= 0.001f)
        {
            StopAllMovement();
            return;
        }

        MoveInDirection(m_transitionMoveDirection, currentSpeed);
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

        // EngageArrival에서도 재충돌 시 동일하게 90도 바운드 후 감속을 계속한다.
        const bool hadTransitionCollision = m_transitionCollisionOccurred;
        if (hadTransitionCollision)
        {
            engine::Vector3 baseDir = m_transitionMoveDirection;
            baseDir.y = 0.0f;
            if (baseDir.LengthSquared() > 0.0001f)
            {
                baseDir.Normalize();
            }
            else
            {
                baseDir = m_engageDirection;
                baseDir.y = 0.0f;
                if (baseDir.LengthSquared() > 0.0001f)
                {
                    baseDir.Normalize();
                }
                else
                {
                    baseDir = engine::Vector3(1.0f, 0.0f, 0.0f);
                }
            }

            engine::Vector3 bouncedDir = baseDir;
            if (m_lastCollisionNormal.LengthSquared() > 0.0001f)
            {
                const engine::Vector3 snappedNormal = SnapNormalToAxisForDiagonal(m_lastCollisionNormal);
                bouncedDir = ReflectDirectionByNormal(baseDir, snappedNormal);
                bouncedDir.y = 0.0f;
            }

            if (bouncedDir.LengthSquared() > 0.0001f)
            {
                bouncedDir.Normalize();
            }
            else
            {
                // 이레귤러 충돌 폴백(요청으로 일단 비활성화)
                // bouncedDir = engine::Vector3(-baseDir.z, 0.0f, baseDir.x);
                bouncedDir = baseDir;
            }

            m_transitionMoveDirection = bouncedDir;
        }

        const float currentSpeed = CalculateTransitionSpeed();
        m_transitionCollisionOccurred = false;
        if (currentSpeed <= 0.001f)
        {
            StopAllMovement();
            return;
        }

        MoveInDirection(m_transitionMoveDirection, currentSpeed);
    }

    // ═══════════════════════════════════════════════════════════════
    // IdleMove 초기화 (PA로 목표 설정)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::InitializeIdleMove()
    {
        m_hasIdleMoveTarget = false;
        m_idleMoveTimer = 0.0f;
        m_collisionOccurred = false;
        m_isReflecting = false;
        m_idleMoveStuckTimer = 0.0f;
        if (GetTransform())
        {
            m_idleMoveLastPosition = GetTransform()->GetWorldPosition();
        }
        
        // PA로 목표 설정 시도
        TrySetIdleMoveTarget();
    }

    // ═══════════════════════════════════════════════════════════════
    // 목표 위치 안전성 체크 (GridMap + 안전 마진)
    // - MonsterPointedType Purple의 IsPositionSafeForFlee 방식
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundBlue::IsPositionSafeForIdleMove(const engine::Vector3& position) const
    {
        if (!m_gridMap) return true;  // GridMap 없으면 체크 안함
        
        int centerX, centerZ;
        m_gridMap->WorldToGrid(position, centerX, centerZ);
        
        // 중심 셀이 유효하지 않거나 이동 불가능하면 즉시 false
        if (!m_gridMap->IsValid(centerX, centerZ) || !m_gridMap->IsWalkable(centerX, centerZ))
        {
            return false;
        }
        
        // 안전 마진이 0이면 중심만 체크
        if (m_targetSafetyMargin <= 0)
        {
            return true;
        }
        
        // 주변 셀 체크 (NxN 범위)
        for (int dz = -m_targetSafetyMargin; dz <= m_targetSafetyMargin; ++dz)
        {
            for (int dx = -m_targetSafetyMargin; dx <= m_targetSafetyMargin; ++dx)
            {
                int checkX = centerX + dx;
                int checkZ = centerZ + dz;
                
                // 범위를 벗어나거나 이동 불가능한 셀이 있으면 안전하지 않음
                if (!m_gridMap->IsValid(checkX, checkZ) || !m_gridMap->IsWalkable(checkX, checkZ))
                {
                    return false;
                }
            }
        }
        
        return true;
    }
    
    // ═══════════════════════════════════════════════════════════════
    // PA로 목표 위치 설정 시도 (최대 10회 재시도)
    // - 플레이어 방향으로 6~12m 떨어진 안전한 위치를 목표로 설정
    // - PA로 경로 계산 후 첫 번째 waypoint를 현재 목표로 설정
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundBlue::TrySetIdleMoveTarget()
    {
        if (!m_pathfindingAgent || !m_targetPlayer || !m_targetPlayer->GetGameObject())
        {
            return false;
        }
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();
        
        // 플레이어 방향 벡터
        engine::Vector3 toPlayer = playerPos - myPos;
        toPlayer.y = 0.0f;
        
        if (toPlayer.LengthSquared() < 0.001f)
        {
            return false;  // 플레이어가 너무 가까움
        }
        
        toPlayer.Normalize();
        
        // 최대 10회 재시도
        constexpr int kMaxAttempts = 10;
        std::uniform_real_distribution<float> distDist(m_targetDistanceMin, m_targetDistanceMax);
        
        for (int attempt = 0; attempt < kMaxAttempts; ++attempt)
        {
            // 6~12m 범위 내 랜덤 거리 선정
            float randomDistance = distDist(gen);
            
            // 목표 위치 계산 (플레이어 방향으로)
            engine::Vector3 targetPos = myPos + toPlayer * randomDistance;
            
            // 안전 마진 체크 (장애물 회피)
            if (!IsPositionSafeForIdleMove(targetPos))
            {
                continue;  // 안전하지 않으면 다음 시도
            }
            
            // PA로 경로 계산 (즉시 계산)
            m_pathfindingAgent->RequestPathImmediate(targetPos);
            
            // 경로가 유효한지 확인
            if (!m_pathfindingAgent->HasPath())
            {
                continue;  // 경로 없으면 다음 시도
            }
            
            // 첫 번째 waypoint를 현재 목표로 설정
            if (!m_pathfindingAgent->GetCurrentWaypoint(m_currentWaypointTarget))
            {
                continue;  // waypoint 없으면 다음 시도
            }
            
            // 성공!
            m_idleMoveTargetPosition = targetPos;
            m_hasIdleMoveTarget = true;
            m_idleMoveTimer = 0.0f;
            
            // 진동 위상 초기화 (새 파장 시작)
            ResetOscillationPhase();
            
            return true;
        }
        
        // 10회 실패 시
        m_hasIdleMoveTarget = false;
        return false;
    }

    // ═══════════════════════════════════════════════════════════════
    // 진동 위상 초기화 (새 파장 시작)
    // - 새로운 진폭 랜덤 선정
    // - 위상 0으로 리셋
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ResetOscillationPhase()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        std::uniform_real_distribution<float> ampDist(m_oscillationAmplitudeMin, m_oscillationAmplitudeMax);
        m_currentOscillationAmplitude = ampDist(gen);
        
        m_oscillationPhase = 0.0f;
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 현재 waypoint 도달 여부
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundBlue::HasReachedCurrentWaypoint() const
    {
        if (!m_hasIdleMoveTarget) return true;
        
        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        engine::Vector3 diff = m_currentWaypointTarget - myPos;
        diff.y = 0.0f;
        
        return diff.LengthSquared() <= (m_targetReachDistance * m_targetReachDistance);
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 다음 waypoint로 전환
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::AdvanceToNextWaypoint()
    {
        if (!m_pathfindingAgent) return;
        
        m_pathfindingAgent->AdvanceToNextWaypoint();
        
        // 다음 waypoint 가져오기
        if (m_pathfindingAgent->GetCurrentWaypoint(m_currentWaypointTarget))
        {
            // 새 waypoint로 전환 시 진동 위상 초기화 (새 파장 시작)
            ResetOscillationPhase();
        }
        else
        {
            // 더 이상 waypoint가 없음 → 목표 완료
            m_hasIdleMoveTarget = false;
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // IdleMove 완료 여부 (목표 도달 또는 시간 초과)
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundBlue::IsIdleMoveComplete() const
    {
        if (!m_hasIdleMoveTarget) return true;
        if (m_idleMoveTimer >= m_idleMoveTimeLimit) return true;
        
        // 마지막 waypoint 도달 확인
        if (!m_pathfindingAgent || !m_pathfindingAgent->HasPath())
        {
            return true;
        }
        
        return false;
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 충돌 처리 → 반사 이동
    // - 진동 정지
    // - 충돌 노말 기반 90도 방향으로 반사
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::HandleIdleMoveCollision(const engine::Vector3& collisionNormal)
    {
        m_isReflecting = true;
        m_reflectTimer = 0.0f;
        m_reflectDirection = CalculateReflectDirection(collisionNormal);
        m_hasIdleMoveTarget = false;  // 목표 무효화
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 노말 기반 90도 반사 방향 계산
    // - 충돌 노말에서 90도 회전 (좌/우 50% 랜덤)
    // ═══════════════════════════════════════════════════════════════
    engine::Vector3 MonsterRoundBlue::CalculateReflectDirection(const engine::Vector3& collisionNormal) const
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        // 충돌 노말이 유효하지 않으면 랜덤 방향
        if (collisionNormal.LengthSquared() < 0.0001f)
        {
            std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265f);
            float randomAngle = angleDist(gen);
            return engine::Vector3(std::cos(randomAngle), 0.0f, std::sin(randomAngle));
        }
        
        // 충돌 노말을 90도 회전 (좌/우 랜덤)
        engine::Vector3 normal = collisionNormal;
        normal.y = 0.0f;
        normal.Normalize();
        
        std::uniform_int_distribution<int> dirDist(0, 1);
        bool turnLeft = (dirDist(gen) == 0);
        
        // 90도 회전: (x, z) → (turnLeft ? -z : z, turnLeft ? x : -x)
        engine::Vector3 reflected;
        if (turnLeft)
        {
            reflected.x = -normal.z;
            reflected.y = 0.0f;
            reflected.z = normal.x;
        }
        else
        {
            reflected.x = normal.z;
            reflected.y = 0.0f;
            reflected.z = -normal.x;
        }
        
        reflected.Normalize();
        return reflected;
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageMove 초기화 (PA 비활성화)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::InitializeEngageMove()
    {
        m_hasEngageTarget = false;
        m_engageCollisionOccurred = false;
        m_engageArrivalOccurred = false;
        m_engageMoveSpeedScaled = m_engageMoveSpeed;
        
        // PA 경로 초기화 (EngageMove는 PA 사용 안 함)
        if (m_pathfindingAgent)
        {
            m_pathfindingAgent->ClearPath();
        }
        
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
        UpdateEngageMoveSpeedScale(targetDistance);
        
        m_hasEngageTarget = true;
    }

    void MonsterRoundBlue::UpdateEngageMoveSpeedScale(float engageMoveRange)
    {
        m_engageMoveSpeedScaled = m_engageMoveSpeed;

        if (!GetTransform() || !GetGameObject() || engageMoveRange <= 0.0001f)
        {
            return;
        }

        const float scanRange = engageMoveRange * 2.0f;
        const float fullSpeedThreshold = engageMoveRange * 1.5f;
        if (scanRange <= 0.0001f || fullSpeedThreshold <= 0.0001f)
        {
            return;
        }

        engine::Vector3 origin = GetTransform()->GetWorldPosition();
        origin.y += 1.0f;

        engine::PhysicsSystem& physicsSystem = engine::SystemManager::Get().GetPhysicsSystem();
        std::vector<engine::RaycastHit> allHits;
        physicsSystem.RaycastAll(origin, m_engageDirection, scanRange, allHits, engine::PhysicsLayer::Mask::All);

        engine::GameObject* selfGO = GetGameObject();
        float lastCollisionDistance = -1.0f;

        for (const auto& h : allHits)
        {
            if (!h.hasHit || !h.collider.Get() || !h.gameObject.Get() || h.gameObject.Get() == selfGO)
            {
                continue;
            }

            const uint32_t hitLayer = h.collider.Get()->GetLayer();
            if (hitLayer == engine::PhysicsLayer::Index::SubWall ||
                hitLayer == engine::PhysicsLayer::Index::Environment ||
                hitLayer == engine::PhysicsLayer::Index::Enemy ||
                hitLayer == engine::PhysicsLayer::Index::Player ||
                h.gameObject->GetInterface<IDamageable>() != nullptr)
            {
                if (h.distance > lastCollisionDistance)
                {
                    lastCollisionDistance = h.distance;
                }
            }
        }

        if (lastCollisionDistance < 0.0f)
        {
            lastCollisionDistance = scanRange;
        }

        const float d = std::max(0.0f, std::min(lastCollisionDistance, fullSpeedThreshold));
        const float ratio = 0.5f + 0.5f * (d / fullSpeedThreshold);  // d=0 -> 0.5, d>=1.5R -> 1.0
        m_engageMoveSpeedScaled = m_engageMoveSpeed * ratio;
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
    // - Green과 동일한 축 스냅 정반사 방향으로 전환
    // - 현재 속도에서 시작해 2초 감속
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::InitializeEngageCollision()
    {
        m_engageTransitionTimer = 0.0f;
        m_transitionStartSpeed = m_engageMoveSpeedScaled;
        m_transitionCollisionOccurred = false;

        engine::Vector3 baseDir = m_engageDirection;
        baseDir.y = 0.0f;
        if (baseDir.LengthSquared() > 0.0001f)
        {
            baseDir.Normalize();
        }
        else
        {
            baseDir = engine::Vector3(1.0f, 0.0f, 0.0f);
        }

        engine::Vector3 reflectedDir = baseDir;
        if (m_lastCollisionNormal.LengthSquared() > 0.0001f)
        {
            const engine::Vector3 snappedNormal = SnapNormalToAxisForDiagonal(m_lastCollisionNormal);
            reflectedDir = ReflectDirectionByNormal(baseDir, snappedNormal);
            reflectedDir.y = 0.0f;
        }

        if (reflectedDir.LengthSquared() > 0.0001f)
        {
            reflectedDir.Normalize();
        }
        else
        {
            // 이레귤러 충돌 폴백(요청으로 일단 비활성화)
            // reflectedDir = engine::Vector3(-baseDir.z, 0.0f, baseDir.x);
            reflectedDir = baseDir;
        }

        m_transitionMoveDirection = reflectedDir;
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageArrival 초기화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::InitializeEngageArrival()
    {
        m_engageTransitionTimer = 0.0f;
        m_transitionStartSpeed = m_engageMoveSpeedScaled;
        m_transitionCollisionOccurred = false;

        m_transitionMoveDirection = m_engageDirection;
        m_transitionMoveDirection.y = 0.0f;
        if (m_transitionMoveDirection.LengthSquared() > 0.0001f)
        {
            m_transitionMoveDirection.Normalize();
        }
        else
        {
            m_transitionMoveDirection = engine::Vector3(1.0f, 0.0f, 0.0f);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 감속 중 현재 속도 계산 (선형 보간)
    // ═══════════════════════════════════════════════════════════════
    float MonsterRoundBlue::CalculateTransitionSpeed() const
    {
        if (m_engageTransitionDuration <= 0.0001f)
        {
            return 0.0f;
        }

        float t = m_engageTransitionTimer / m_engageTransitionDuration;
        t = std::min(t, 1.0f);

        if (m_transitionCollisionOccurred)
        {
            t = std::min(1.0f, t * m_transitionCollisionBrakeMultiplier);
        }

        return m_transitionStartSpeed * (1.0f - t);
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

    bool MonsterRoundBlue::CanEnterEngageByRaycast() const
    {
        if (!m_targetPlayer || !m_targetPlayer->GetGameObject() || !GetTransform())
        {
            return false;
        }

        engine::GameObject* selfGO = GetGameObject();
        engine::GameObject* playerGO = m_targetPlayer->GetGameObject();
        if (!selfGO || !playerGO)
        {
            return false;
        }

        engine::Vector3 origin = GetTransform()->GetWorldPosition();
        origin.y += 1.0f;

        engine::Vector3 target = m_targetPlayer->GetTransform()->GetWorldPosition();
        target.y += 1.0f;

        engine::Vector3 toPlayer = target - origin;
        toPlayer.y = 0.0f;

        const float distanceToPlayer = toPlayer.Length();
        if (distanceToPlayer < 0.0001f)
        {
            return true;
        }

        if (distanceToPlayer > m_detectionRange)
        {
            return false;
        }

        toPlayer.Normalize();

        engine::PhysicsSystem& physicsSystem = engine::SystemManager::Get().GetPhysicsSystem();
        std::vector<engine::RaycastHit> allHits;
        physicsSystem.RaycastAll(origin, toPlayer, distanceToPlayer, allHits, engine::PhysicsLayer::Mask::All);

        std::vector<engine::RaycastHit> validHits;
        validHits.reserve(allHits.size());
        for (const auto& h : allHits)
        {
            if (!h.hasHit || !h.gameObject.Get() || h.gameObject.Get() == selfGO || !h.collider.Get())
            {
                continue;
            }
            validHits.push_back(h);
        }

        std::sort(validHits.begin(), validHits.end(),
            [](const engine::RaycastHit& a, const engine::RaycastHit& b)
            {
                return a.distance < b.distance;
            });

        for (const auto& h : validHits)
        {
            engine::GameObject* hitGO = h.gameObject.Get();
            if (!hitGO)
            {
                continue;
            }

            if (hitGO == playerGO)
            {
                return true;
            }

            const uint32_t hitLayer = h.collider.Get()->GetLayer();
            if (hitLayer == engine::PhysicsLayer::Index::Player)
            {
                return true;
            }

            if (hitLayer == engine::PhysicsLayer::Index::SubWall ||
                hitLayer == engine::PhysicsLayer::Index::Environment ||
                hitLayer == engine::PhysicsLayer::Index::Enemy ||
                hitGO->GetInterface<IDamageable>() != nullptr)
            {
                return false;
            }
        }

        return false;
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
        else if (state == "Dead")
        {
            // Dead 진입 시 3방향 총알 발사 (단발)
            if (m_bulletFactory && m_targetPlayer && m_targetPlayer->GetGameObject())
            {
                engine::Vector3 direction = CalculateDirectionToPlayer();
                engine::Vector3 firePosition = GetTransform()->GetWorldPosition();
                
                // 3방향 발사 (spreadAngle은 부모의 m_spreadAngle 사용)
                m_bulletFactory->ThreewayFireMonster(firePosition, direction, m_spreadAngle, m_bulletParams);

                // 사운드
                m_remainShotSoundCount = 3;
                m_shotSoundTimer = 0.0f;
            }
        }
        else if (state == "IdleMove")
        {
            // IdleMove 진입 시 PA로 목표 설정
            InitializeIdleMove();
            
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
                m_logicFSM->SetParameter("PlayerDetected", false);
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
        ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "Tier: Blue (Oscillation + Dash)");
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "[No Bullet Attack - Contact Damage Only]");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Fires 3-way bullets on death)");
        
        MonsterRoundType::OnGui();
        
        // IdleMove 진동 이동 설정
        ImGui::Separator();
        ImGui::Text("=== Blue IdleMove Settings ===");
        ImGui::DragFloat("Time Limit (sec)", &m_idleMoveTimeLimit, 0.1f, 1.0f, 20.0f);
        ImGui::DragFloat("Waypoint Reach Dist", &m_targetReachDistance, 0.1f, 0.5f, 5.0f);
        
        ImGui::Spacing();
        ImGui::Text("Target Distance (from Monster to Goal):");
        ImGui::DragFloat("  Min Distance", &m_targetDistanceMin, 0.5f, 1.0f, 50.0f);
        ImGui::DragFloat("  Max Distance", &m_targetDistanceMax, 0.5f, 1.0f, 50.0f);
        
        ImGui::Spacing();
        ImGui::DragInt("Target Safety Margin", &m_targetSafetyMargin, 0.05f, 0, 5);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Obstacle avoidance margin (0=None, 1=3x3 cells, 2=5x5 cells)");
        }
        
        // 진동 설정
        ImGui::Separator();
        ImGui::Text("=== Blue Oscillation Settings ===");
        ImGui::DragFloat("Amplitude Min", &m_oscillationAmplitudeMin, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Amplitude Max", &m_oscillationAmplitudeMax, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Speed Multiplier", &m_oscillationSpeedMultiplier, 0.5f, 0.0f, 20.0f);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Controls lateral oscillation speed (higher = faster side-to-side movement)");
        }
        
        // 충돌 반사 설정
        ImGui::Separator();
        ImGui::Text("=== Blue Collision Settings ===");
        ImGui::DragFloat("Reflect Distance", &m_collisionReflectDistance, 0.1f, 0.5f, 10.0f);
        ImGui::DragFloat("Damage Cooldown", &m_damageCooldown, 0.1f, 0.1f, 5.0f);
        
        // EngageMove 설정
        ImGui::Separator();
        ImGui::Text("=== Blue EngageMove Settings ===");
        ImGui::DragFloat("Engage Move Speed", &m_engageMoveSpeed, 0.5f, 1.0f, 30.0f);
        ImGui::DragFloat("Player Ignore Duration", &m_playerIgnoreDuration, 0.1f, 0.1f, 10.0f);
        
        // 런타임 정보 - IdleMove
        ImGui::Separator();
        ImGui::Text("=== Blue Runtime (IdleMove) ===");
        ImGui::Text("Has Target: %s", m_hasIdleMoveTarget ? "Yes" : "No");
        ImGui::Text("Is Reflecting: %s", m_isReflecting ? "Yes" : "No");
        
        if (m_hasIdleMoveTarget)
        {
            ImGui::Text("Target Goal: (%.2f, %.2f, %.2f)", 
                m_idleMoveTargetPosition.x, m_idleMoveTargetPosition.y, m_idleMoveTargetPosition.z);
            ImGui::Text("Current Waypoint: (%.2f, %.2f, %.2f)", 
                m_currentWaypointTarget.x, m_currentWaypointTarget.y, m_currentWaypointTarget.z);
            ImGui::Text("Timer: %.2f / %.2f sec", m_idleMoveTimer, m_idleMoveTimeLimit);
            
            engine::Vector3 myPos = GetTransform()->GetWorldPosition();
            float distToWaypoint = engine::Vector3::Distance(myPos, m_currentWaypointTarget);
            ImGui::Text("Dist to Waypoint: %.2f m", distToWaypoint);
        }
        
        if (m_isReflecting)
        {
            ImGui::Text("Reflect Timer: %.2f / %.2f sec", m_reflectTimer, m_reflectDuration);
            ImGui::Text("Reflect Dir: (%.2f, %.2f, %.2f)", 
                m_reflectDirection.x, m_reflectDirection.y, m_reflectDirection.z);
        }
        
        ImGui::Spacing();
        ImGui::Text("Oscillation:");
        ImGui::Text("  Phase: %.2f rad", m_oscillationPhase);
        ImGui::Text("  Current Amplitude: %.2f", m_currentOscillationAmplitude);
        
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
        
        // IdleMove 진동 이동 설정
        j["IdleMoveTimeLimit"] = m_idleMoveTimeLimit;
        j["TargetReachDistance"] = m_targetReachDistance;
        j["TargetDistanceMin"] = m_targetDistanceMin;
        j["TargetDistanceMax"] = m_targetDistanceMax;
        j["TargetSafetyMargin"] = m_targetSafetyMargin;
        
        // 진동 설정
        j["OscillationAmplitudeMin"] = m_oscillationAmplitudeMin;
        j["OscillationAmplitudeMax"] = m_oscillationAmplitudeMax;
        j["OscillationSpeedMultiplier"] = m_oscillationSpeedMultiplier;
        
        // 충돌 반사 설정
        j["CollisionReflectDistance"] = m_collisionReflectDistance;
        
        // EngageMove 설정
        j["DamageCooldown"] = m_damageCooldown;
        j["EngageMoveSpeed"] = m_engageMoveSpeed;
        j["PlayerIgnoreDuration"] = m_playerIgnoreDuration;
    }

    void MonsterRoundBlue::Load(const engine::json& j)
    {
        MonsterRoundType::Load(j);
        
        // IdleMove 진동 이동 설정
        m_idleMoveTimeLimit = j.value("IdleMoveTimeLimit", 7.0f);
        m_targetReachDistance = j.value("TargetReachDistance", 2.0f);
        m_targetDistanceMin = j.value("TargetDistanceMin", 6.0f);
        m_targetDistanceMax = j.value("TargetDistanceMax", 12.0f);
        m_targetSafetyMargin = j.value("TargetSafetyMargin", 1);
        
        // 진동 설정
        m_oscillationAmplitudeMin = j.value("OscillationAmplitudeMin", 0.5f);
        m_oscillationAmplitudeMax = j.value("OscillationAmplitudeMax", 3.0f);
        m_oscillationSpeedMultiplier = j.value("OscillationSpeedMultiplier", 5.0f);
        
        // 충돌 반사 설정
        m_collisionReflectDistance = j.value("CollisionReflectDistance", 2.0f);
        
        // EngageMove 설정
        m_damageCooldown = j.value("DamageCooldown", 1.0f);
        m_engageMoveSpeed = j.value("EngageMoveSpeed", 10.0f);
        m_playerIgnoreDuration = j.value("PlayerIgnoreDuration", 1.0f);
        
        // ─────────────────────────────────────────────
        // Blue 전용: 단발 발사 모드 (Dead 시 3방향 발사)
        // - 기본값 true (부모에서 false로 로드되므로 덮어쓰기)
        // - m_isDoSingleShot이 true이므로 CanFireBullet()에서 m_fireTimer를 무시
        // - m_fireRate는 실제로 사용되지 않지만 명시적으로 0으로 설정
        // ─────────────────────────────────────────────
        m_isDoSingleShot = j.value("IsDoSingleShot", true);
        m_fireRate = 0.0f;  // m_isDoSingleShot = true로 인해 실제 사용되지 않음 (명시적 설정)
    }
}
