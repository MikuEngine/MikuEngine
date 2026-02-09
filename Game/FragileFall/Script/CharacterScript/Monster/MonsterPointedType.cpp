#include "GamePCH.h"
#include "MonsterPointedType.h"

#include "Script/CharacterScript/Common/BulletFactory.h"

#include <Framework/Asset/Prefab.h>

#include <Engine/Core/System/MyTime.h>
#include <Framework/Object/Component/AnimFSM.h>
#include <Framework/Object/Component/Pathfinding/PathfindingAgent.h>
#include <Framework/Object/Component/Pathfinding/GridMap.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/System/SystemManager.h>
#include <Framework/System/PathfindingSystem.h>
#include <Framework/Object/Component/Collider.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::Awake()
    {
        MonsterScript::Awake();
        
        // Pointed 타입 고정
        m_attackType = AttackType::Pointed;

        // 처형(Dead) 시 바로 사라지도록 딜레이 제거
        m_deathDelay = 0.0f;
        
        // 스탯은 씬 파일에서 로드됨 (Load 함수 참조)
    }

    void MonsterPointedType::Start()
    {
        MonsterScript::Start();

        // PathfindingAgent 설정 (이동 몬스터용)
        if (m_pathfindingAgent)
        {
            m_pathfindingAgent->SetPathUpdateInterval(0.5f);        // 0.5초마다 경로 재계산
            m_pathfindingAgent->SetWaypointReachDistance(0.6f);    // 속도 높을 때 오버슈트 방지 (넉넉한 도달 거리)
            m_pathfindingAgent->SetTargetMoveThreshold(2.0f);       // 목표가 2.0f 이상 움직이면 재계산
        }
        
        // GridMap 캐싱 (도망 위치 유효성 체크용)
        auto& pathfindingSystem = engine::SystemManager::Get().GetPathfindingSystem();
        m_gridMap = pathfindingSystem.GetGridMap();
    }

    // ═══════════════════════════════════════════════════════════════
    // FSM 초기화
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::InitializeFSM()
    {
        if (!m_logicFSM) return;

        // ─────────────────────────────────────────────
        // 상태 정의
        // ─────────────────────────────────────────────
        AddFSMState("Idle", true);          // 기본 상태
        AddFSMState("EngageMove", false);   // 추적 이동 (공격 사거리 밖)
        AddFSMState("EngageStop", false);   // 정지 (공격 사거리 안, 쿨타임 중)
        AddFSMState("EngageAttack", false); // 공격 중 정지
        AddFSMState("Flee", false);         // 플레이어가 근접 시 도망
        AddFSMState("Redemption", false);   // 도망 실패 시 반사 이동
        AddFSMState("Laststand", false);    // 최후의 저항 (정지 + 공격)
        AddFSMState("Fragile", false);
        AddFSMState("Dead", false);

        // ─────────────────────────────────────────────
        // StateMap 업데이트 (Transition 추가 전에 필수!)
        // ─────────────────────────────────────────────
        m_logicFSM->Initialize();  // UpdateStateMap() 호출

        // ─────────────────────────────────────────────
        // 파라미터 정의
        // ─────────────────────────────────────────────
        m_logicFSM->SetParameter("PlayerInDetectionRange", m_isPlayerInDetectionRange);
        m_logicFSM->SetParameter("PlayerInRange", m_isPlayerInRange);
        m_logicFSM->SetParameter("PlayerInFleeRange", m_isPlayerInFleeRange);
        m_logicFSM->SetParameter("CanFire", m_canFire);
        m_logicFSM->SetParameter("AttackComplete", false);
        m_logicFSM->SetParameter("Fragile", m_isFragile);
        m_logicFSM->SetParameter("Die", m_isDead);
        
        // Redemption/Laststand 전이용 파라미터
        m_logicFSM->SetParameter("NoWayOut", false);           // Flee → Redemption, Idle → Redemption
        m_logicFSM->SetParameter("RedemptionRetry", false);    // Redemption → Idle (재시도)
        m_logicFSM->SetParameter("RedemptionFailed", false);   // Redemption → Laststand (4회 실패)
        m_logicFSM->SetParameter("PathFound", false);          // Redemption → Flee, Laststand → Flee

        // ─────────────────────────────────────────────
        // 전이 정의 (StateMap이 업데이트된 후에 추가)
        // ─────────────────────────────────────────────
        
        // Idle ↔ EngageMove (플레이어 감지 거리 진입/이탈)
        AddFSMTransition("Idle", "EngageMove", "PlayerInDetectionRange", BoolTrue());
        AddFSMTransition("EngageMove", "Idle", "PlayerInDetectionRange", BoolFalse());

        // EngageMove → EngageStop (플레이어 공격 사거리 진입, 쿨타임 중)
        AddFSMTransition("EngageMove", "EngageStop", "PlayerInRange", BoolTrue(), "CanFire", BoolFalse());

        // EngageMove → EngageAttack (플레이어 공격 사거리 진입, 공격 가능)
        AddFSMTransition("EngageMove", "EngageAttack", "PlayerInRange", BoolTrue(), "CanFire", BoolTrue());

        // EngageStop → EngageMove (플레이어 공격 사거리 이탈)
        AddFSMTransition("EngageStop", "EngageMove", "PlayerInRange", BoolFalse());

        // EngageStop → EngageAttack (쿨타임 끝, 플레이어 여전히 사거리 안)
        AddFSMTransition("EngageStop", "EngageAttack", "CanFire", BoolTrue(), "PlayerInRange", BoolTrue());

        // EngageAttack → EngageMove (공격 완료, 플레이어 공격 사거리 이탈)
        AddFSMTransition("EngageAttack", "EngageMove", "AttackComplete", BoolTrue(), "PlayerInRange", BoolFalse());

        // EngageAttack → EngageStop (공격 완료, 플레이어 공격 사거리 안, 쿨타임 중)
        AddFSMTransition("EngageAttack", "EngageStop", "AttackComplete", BoolTrue(), "PlayerInRange", BoolTrue(), "CanFire", BoolFalse());

        // EngageAttack → EngageAttack (공격 완료, 플레이어 공격 사거리 안, 공격 가능 - 즉시 재공격)
        AddFSMTransition("EngageAttack", "EngageAttack", "AttackComplete", BoolTrue(), "PlayerInRange", BoolTrue(), "CanFire", BoolTrue());

        // Any → Fragile (HP 0, Fragile 트리거)
        AddFSMTransition("Idle", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageMove", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageStop", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageAttack", "Fragile", "Fragile", Trigger());

        // Any -> Flee (fleeRange, m_isPlayerInFleeRange bool)
        if (m_monsterTier == MonsterTier::Purple)
        {
            AddFSMTransition("Idle", "Flee", "PlayerInFleeRange", BoolTrue());
            AddFSMTransition("EngageMove", "Flee", "PlayerInFleeRange", BoolTrue());
            AddFSMTransition("EngageStop", "Flee", "PlayerInFleeRange", BoolTrue());
            AddFSMTransition("EngageAttack", "Flee", "PlayerInFleeRange", BoolTrue());

            AddFSMTransition("Flee", "EngageMove", "PlayerInFleeRange", BoolFalse(), "PlayerInRange", BoolFalse());
            AddFSMTransition("Flee", "EngageStop", "PlayerInFleeRange", BoolFalse(), "PlayerInRange", BoolTrue());

            // Flee → Redemption (패스찾기 10회 실패 또는 끼임 감지)
            AddFSMTransition("Flee", "Redemption", "NoWayOut", BoolTrue());
            
            // Redemption → Idle (5m 완주, 재시도 필요)
            AddFSMTransition("Redemption", "Idle", "RedemptionRetry", BoolTrue());
            
            // Redemption → Flee (5m 완주, 패스 찾음)
            AddFSMTransition("Redemption", "Flee", "PathFound", BoolTrue());
            
            // Redemption → Laststand (4회 재시도 실패)
            AddFSMTransition("Redemption", "Laststand", "RedemptionFailed", BoolTrue());
            
            // Idle → Redemption (0.1초 대기 후 자동 전이)
            AddFSMTransition("Idle", "Redemption", "NoWayOut", BoolTrue());
            
            // Laststand → Flee (패스 찾음)
            AddFSMTransition("Laststand", "Flee", "PathFound", BoolTrue());
            
            // Laststand → EngageMove (플레이어 사거리 밖)
            AddFSMTransition("Laststand", "EngageMove", "PlayerInFleeRange", BoolFalse());
            
            // Redemption/Laststand → Fragile
            AddFSMTransition("Flee", "Fragile", "Fragile", Trigger());
            AddFSMTransition("Redemption", "Fragile", "Fragile", Trigger());
            AddFSMTransition("Laststand", "Fragile", "Fragile", Trigger());
        }

        // Fragile → Dead (Execution, Die 트리거)
        AddFSMTransition("Fragile", "Dead", "Die", Trigger());

        // ─────────────────────────────────────────────
        // 초기 상태 설정
        // ─────────────────────────────────────────────
        m_logicFSM->InitializeCurrentState();  // 기본 상태로 설정
    }

    void MonsterPointedType::InitializeAnimFSM()
    {
        if (!m_animFSM) return;

        m_animFSM->ClearStates();

        // LogicFSM 상태 이름과 동일한 Anim 상태 등록 → 상태 전환 시 해당 클립 자동 재생
        // 클립: Idle, WalkForward, Fire (SkeletalAnimator에 등록된 이름)
        const float kCrossFade = 0.1f;
        const int kLayer = 0;
        const float kSpeed = 1.0f;

        m_animFSM->AddDefaultState("Idle", "Idle", true, kCrossFade, kLayer, kSpeed);
        m_animFSM->AddDefaultState("EngageMove", "WalkForward", true, kCrossFade, kLayer, kSpeed);
        m_animFSM->AddDefaultState("EngageStop", "Idle", true, kCrossFade, kLayer, kSpeed);
        m_animFSM->AddDefaultState("EngageAttack", "Fire", true, kCrossFade, kLayer, kSpeed);
        m_animFSM->AddDefaultState("Flee", "WalkForward", true, kCrossFade, kLayer, kSpeed);
        m_animFSM->AddDefaultState("Redemption", "WalkForward", true, kCrossFade, kLayer, kSpeed);
        m_animFSM->AddDefaultState("Laststand", "Idle", true, kCrossFade, kLayer, kSpeed);
        m_animFSM->AddDefaultState("Fragile", "Idle", true, kCrossFade, kLayer, kSpeed);
        m_animFSM->AddDefaultState("Dead", "Idle", true, kCrossFade, kLayer, kSpeed);
    }

    void MonsterPointedType::InitializeAnimations()
    {
        // 클립 목록(Idle, WalkForward, Fire)은 프리팹 SkeletalAnimator에서 설정
    }

    void MonsterPointedType::InitializeBullet()
    {
        switch (m_monsterTier)
        {
        case MonsterTier::Gray:
            m_bulletParams.type = BulletType::Linear;
            m_bulletParams.speed = m_bulletSpeed;
            m_bulletParams.lifetime = m_bulletLifetime;
            m_bulletParams.damage = m_attackDamage;
            m_bulletParams.scale = m_bulletScale;
            m_bulletParams.explosionRadius = m_explosionRadius;  // 부모 값 복사 (미사용)
            break;
        case MonsterTier::Blue:
            m_bulletParams.type = BulletType::Linear;
            m_bulletParams.speed = m_bulletSpeed;
            m_bulletParams.lifetime = m_bulletLifetime;
            m_bulletParams.damage = m_attackDamage;
            m_bulletParams.scale = m_bulletScale;
            m_bulletParams.explosionRadius = m_explosionRadius;  // 부모 값 복사 (미사용)
            break;
        case MonsterTier::Green:
            // ─────────────────────────────────────────────
            // 포물선 전용 파라미터
            // - launchAngle, ownGravity는 Attack() 시점에 자동 계산
            // - speed는 AttackRange 기준 자동 계산
            // - ownGravity는 에디터 설정값 (고정)
            // ─────────────────────────────────────────────
            m_bulletParams.type = BulletType::Parabolic;
            m_bulletParams.speed = CalculateParabolicSpeed();  // 자동 계산
            m_bulletParams.launchAngle = m_minLaunchAngle;     // 기본값 (Attack에서 자동 계산)
            m_bulletParams.ownGravity = m_ownGravity;          // 에디터 설정값 (고정)
            m_bulletParams.explosionRadius = m_explosionRadius; // 폭발 반경
            m_bulletParams.lifetime = m_bulletLifetime;
            m_bulletParams.damage = m_attackDamage;
            m_bulletParams.scale = m_bulletScale;
			break;
		case MonsterTier::Purple:
            m_bulletParams.type = BulletType::Linear;
            m_bulletParams.speed = m_bulletSpeed;
            m_bulletParams.lifetime = m_bulletLifetime;
            m_bulletParams.damage = m_attackDamage;
            m_bulletParams.scale = m_bulletScale;
            m_bulletParams.explosionRadius = m_explosionRadius;  // 부모 값 복사 (미사용)
            break;
        default:
            m_bulletParams.type = BulletType::Linear;
            m_bulletParams.speed = m_bulletSpeed;
            m_bulletParams.lifetime = m_bulletLifetime;
            m_bulletParams.damage = m_attackDamage;
            m_bulletParams.scale = m_bulletScale;
            m_bulletParams.explosionRadius = m_explosionRadius;  // 부모 값 복사 (미사용)
            break;
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 회전 (MonsterPointedType 전용, BaseControllerScript 로직 무관)
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::RotateTowardsDirection(const engine::Vector3& targetDirection)
    {
        engine::Vector3 dir = targetDirection;
        dir.y = 0.0f;
        if (dir.LengthSquared() < 0.0001f) return;
        dir.Normalize();

        engine::Vector3 currentForward = GetForwardDirection();
        float dot = currentForward.Dot(dir);
        dot = std::clamp(dot, -1.0f, 1.0f);
        float angleDiff = std::acos(dot);
        const float threshold = 0.017f;
        if (angleDiff < threshold) return;

        const float kRotationSpeedMultiplier = 2.0f;
        float fixedDt = engine::Time::FixedDeltaTime();
        float step = m_rotationSpeed * kRotationSpeedMultiplier * fixedDt;
        if (step > angleDiff) step = angleDiff;
        engine::Vector3 cross = currentForward.Cross(dir);
        float sign = (cross.y >= 0.0f) ? 1.0f : -1.0f;
        float finalYaw = sign * step;

        engine::Quaternion currentRot = GetTransform()->GetWorldRotation();
        engine::Vector3 euler = currentRot.ToEuler();
        euler.y += finalYaw;
        engine::Quaternion newRot = engine::Quaternion::CreateFromYawPitchRoll(euler.y, euler.x, euler.z);

        if (m_rigidbody && m_rigidbody->IsDynamic())
        {
            engine::Vector3 pos = GetTransform()->GetWorldPosition();
            m_rigidbody->ForceSetTransform(pos, newRot, false, true);
        }
        else
        {
            GetTransform()->SetLocalRotation(newRot);
        }
    }

    void MonsterPointedType::MoveTowardsPlayer()
    {
        if (!m_pathfindingAgent || !m_rigidbody || !m_targetPlayer || m_moveSpeed <= 0.0f)
            return;

        engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
        engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();
        float fixedDeltaTime = engine::Time::FixedDeltaTime();
        m_pathfindingAgent->UpdatePathfindingFixed(fixedDeltaTime, playerPos);

        engine::Vector3 nextWaypoint;
        engine::Vector3 direction;
        bool hasValidWaypoint = m_pathfindingAgent->HasPath() &&
            m_pathfindingAgent->GetCurrentWaypoint(nextWaypoint);

        if (!hasValidWaypoint)
            direction = CalculateDirectionToPlayer();
        else
        {
            // 속도가 높을 때 웨이포인트를 지나쳐 반대 방향 힘이 가해지는 것 방지 (오버슈트 시 다음 웨이포인트로 전진)
            engine::Vector3 nextInPath = m_pathfindingAgent->GetNextWaypoint();
            if (nextInPath.LengthSquared() > 0.0001f)
            {
                engine::Vector3 toCur = nextWaypoint - currentPos;
                toCur.y = 0.0f;
                engine::Vector3 toNext = nextInPath - currentPos;
                toNext.y = 0.0f;
                if (toNext.LengthSquared() < toCur.LengthSquared())
                {
                    m_pathfindingAgent->AdvanceToNextWaypoint();
                    hasValidWaypoint = m_pathfindingAgent->GetCurrentWaypoint(nextWaypoint);
                    if (!hasValidWaypoint)
                        direction = CalculateDirectionToPlayer();
                }
            }
            if (hasValidWaypoint)
            {
                engine::Vector3 toWaypoint = nextWaypoint - currentPos;
                toWaypoint.y = 0.0f;
                if (toWaypoint.LengthSquared() > 0.0001f)
                {
                    toWaypoint.Normalize();
                    direction = toWaypoint;
                }
                else
                    direction = CalculateDirectionToPlayer();
            }
        }

        direction.y = 0.0f;
        if (direction.LengthSquared() < 0.0001f) return;
        direction.Normalize();

        RotateTowardsDirection(direction);
        ApplyMovementForce(direction, m_moveSpeed);
    }

    // ═══════════════════════════════════════════════════════════════
    // 입력 처리 (FSM 파라미터 업데이트)
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::ProcessInput()
    {
        if (!m_logicFSM) return;

        // 플레이어를 찾지 못했으면 재탐색
        if (!m_targetPlayer)
        {
            FindPlayer();
        }
        
        float dist = GetDistanceToPlayer();
        // 사거리 경계 움찔 방지: 정지/공격 중일 때는 나갈 때만 1.2배로 히스테리시스 적용
        std::string state = GetCurrentState();
        float rangeBuffer = (state == "EngageStop" || state == "EngageAttack") ? 1.2f : 1.0f;

        // 플레이어와의 거리 체크
        m_isPlayerInDetectionRange = (dist <= m_detectionRange);
        m_isPlayerInRange = (dist <= m_AttackRange * rangeBuffer);
        
        // 도망 범위 조건
        if (m_monsterTier == MonsterTier::Purple)
        {
            m_isPlayerInFleeRange = false;

            if (GetCurrentState() == "Flee")  m_isPlayerInFleeRange = (dist <= m_safeRange);
            else m_isPlayerInFleeRange = (dist <= m_fleeRange);
        }

        // 발사 가능 여부 체크 (쿨타임)
        m_canFire = (m_fireTimer <= 0.0f);

        // FSM 파라미터 업데이트
        m_logicFSM->SetParameter("PlayerInDetectionRange", m_isPlayerInDetectionRange);
        m_logicFSM->SetParameter("PlayerInRange", m_isPlayerInRange);
        m_logicFSM->SetParameter("PlayerInFleeRange", m_isPlayerInFleeRange);
        m_logicFSM->SetParameter("CanFire", m_canFire);
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태별 행동 - 비물리 (Update에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::UpdateStateBasedBehavior(const std::string& state, float deltaTime)
    {
        // 발사 쿨타임 감소 (모든 상태에서, 비물리)
        if (m_fireTimer > 0.0f)
        {
            m_fireTimer -= deltaTime;
        }

        // 달리기 애니메이션 재생 속도 = 몬스터 이동 속도에 비례 (이동 상태에서만)
        const float kBaseMoveSpeed = 10.0f;  // 애니 기준 속도, 필요 시 튜닝
        if (m_skeletalAnimator && m_animFSM)
        {
            int baseLayer = m_animFSM->GetBaseLayerIndex();
            float animSpeed = 1.0f;
            if (state == "EngageMove")
                animSpeed = std::max(0.1f, m_moveSpeed / kBaseMoveSpeed);
            else if (state == "Flee")
                animSpeed = std::max(0.1f, (m_moveSpeed * m_fleeSpeedMultiplier) / kBaseMoveSpeed);
            else if (state == "Redemption")
                animSpeed = std::max(0.1f, (m_moveSpeed * m_redemptionSpeedMultiplier) / kBaseMoveSpeed);
            m_skeletalAnimator->SetLayerSpeed(baseLayer, animSpeed);
        }

        if (state == "Flee")
        {
            ExecuteFleeBehaviorNonPhysics(deltaTime);
        }
        else if (state == "EngageMove")
        {
            ExecuteEngageMoveBehaviorNonPhysics(deltaTime);
        }
        else if (state == "EngageStop")
        {
            ExecuteEngageStopBehaviorNonPhysics(deltaTime);
        }
        else if (state == "EngageAttack")
        {
            ExecuteEngageAttackBehaviorNonPhysics(deltaTime);
        }
        else if (state == "Idle")
        {
            ExecuteIdleBehaviorNonPhysics(deltaTime);
        }
        else if (state == "Redemption")
        {
            ExecuteRedemptionBehaviorNonPhysics(deltaTime);
        }
        else if (state == "Laststand")
        {
            ExecuteLaststandBehaviorNonPhysics(deltaTime);
        }
        else if (state == "Fragile")
        {
            ExecuteFragileBehaviorNonPhysics();
        }
        else if (state == "Dead")
        {
            ExecuteDeadBehaviorNonPhysics();
        }
    }

    void MonsterPointedType::ExecuteFleeBehaviorNonPhysics(float deltaTime)
    {
        // ─────────────────────────────────────────────
        // 끼임 감지 (주기적으로 위치 변화 체크)
        // ─────────────────────────────────────────────
        float elapsedSinceCheck = engine::Time::GetElapsedSeconds(m_lastFleePositionCheckTime);
        if (elapsedSinceCheck >= m_fleeStuckCheckInterval)
        {
            engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
            float distanceMoved = engine::Vector3::Distance(m_lastFleePosition, currentPos);
            
            // 이동 거리가 임계값 이하면 끼임으로 판정
            if (distanceMoved < m_fleeStuckDistanceThreshold)
            {
                // 끼임 감지! → Redemption 전이
                if (m_logicFSM)
                {
                    m_logicFSM->SetParameter("NoWayOut", true);
                }
            }
            
            // 위치 및 시간 갱신
            m_lastFleePosition = currentPos;
            m_lastFleePositionCheckTime = engine::Time::GetTimestamp();
        }
    }

    void MonsterPointedType::ExecuteEngageMoveBehaviorNonPhysics(float deltaTime)
    {
        // 비물리 처리 (없음 - 이동은 물리에서 처리)
    }

    void MonsterPointedType::ExecuteEngageStopBehaviorNonPhysics(float deltaTime)
    {
        // 비물리 처리 (없음 - 회전은 물리에서 처리)
    }

    void MonsterPointedType::ExecuteEngageAttackBehaviorNonPhysics(float deltaTime)
    {
        // 공격 타이머 및 발사 (비물리)
        Attack(deltaTime);
        
        // 공격 후 거리/패스 체크 (LastStand → EngageMove → EngageAttack 경로)
        if (m_needsPostAttackCheck)
        {
            CheckPostAttackTransition();
        }
    }

    void MonsterPointedType::ExecuteIdleBehaviorNonPhysics(float deltaTime)
    {
        // Redemption 재시도를 위한 Idle 대기
        if (m_isNoWayOut)
        {
            m_idleTimer += deltaTime;
            
            // 0.1초 후 Redemption 재전이
            if (m_idleTimer >= 0.1f)
            {
                m_idleTimer = 0.0f;
                
                if (m_logicFSM)
                {
                    m_logicFSM->SetParameter("NoWayOut", true);
                }
            }
        }
    }

    void MonsterPointedType::ExecuteFragileBehaviorNonPhysics()
    {
        // Fragile 상태: 아무 행동도 하지 않음 (Execution 대기)
    }

    void MonsterPointedType::OnFragile()
    {
        MonsterScript::OnFragile();

        if (m_fragileCrystalInstance != nullptr)
            return;

        const char* tierStr = GetMonsterTierStr(m_monsterTier);
        std::string prefabName = std::string("FragileCrystal_") + tierStr;
        engine::GameObject* crystal = engine::Prefab::Instantiate(prefabName);
        if (crystal && crystal->GetTransform() && GetTransform())
        {
            crystal->GetTransform()->SetParent(GetTransform(), false);
            m_fragileCrystalInstance = crystal;
        }
    }

    void MonsterPointedType::OnRevive()
    {
        if (m_fragileCrystalInstance != nullptr)
        {
            m_fragileCrystalInstance->Destroy();
            m_fragileCrystalInstance = nullptr;
        }
        MonsterScript::OnRevive();
    }

    void MonsterPointedType::ExecuteDeadBehaviorNonPhysics()
    {
        // 부모 클래스의 Dead 타이머 처리 (2초 후 Destroy)
        MonsterScript::ExecuteDeadBehaviorNonPhysics();
    }

    void MonsterPointedType::ExecuteRedemptionBehaviorNonPhysics(float deltaTime)
    {
        // Redemption: 플레이어 방향으로 5m 돌진 (정반사)
        
        // ─────────────────────────────────────────────
        // 5m 도달 체크
        // ─────────────────────────────────────────────
        if (m_redemptionTraveledDistance >= m_redemptionTargetDistance)
        {
            // 5m 완주! → 패스 체크
            if (TrySelectFleeTarget())
            {
                // 패스 찾음
                if (m_redemptionRetryCount >= kMaxRedemptionRetries)
                {
                    // 4회 도달 → LastStand 전이
                    if (m_logicFSM)
                    {
                        m_logicFSM->SetParameter("RedemptionFailed", true);
                    }
                }
                else
                {
                    // 4회 미만 → Flee 전이
                    if (m_logicFSM)
                    {
                        m_logicFSM->SetParameter("PathFound", true);
                    }
                }
            }
            else
            {
                // 패스 못 찾음
                if (m_redemptionRetryCount >= kMaxRedemptionRetries)
                {
                    // 4회 도달 → LastStand 전이
                    if (m_logicFSM)
                    {
                        m_logicFSM->SetParameter("RedemptionFailed", true);
                    }
                }
                else
                {
                    // 4회 미만 → Idle 전이 (0.1초 후 재시도)
                    if (m_logicFSM)
                    {
                        m_logicFSM->SetParameter("RedemptionRetry", true);
                    }
                }
            }
        }
    }

    void MonsterPointedType::ExecuteLaststandBehaviorNonPhysics(float deltaTime)
    {
        // LastStand: 결사항전
        
        // ─────────────────────────────────────────────
        // 1. Flee 패스 체크 (항상 최우선)
        // ─────────────────────────────────────────────
        if (TrySelectFleeTarget())
        {
            // 패스 찾음 → Flee 전이
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("PathFound", true);
            }
            return;
        }
        
        // ─────────────────────────────────────────────
        // 2. 플레이어 거리 체크
        // ─────────────────────────────────────────────
        if (!m_targetPlayer) return;
        
        float distanceToPlayer = GetDistanceToPlayer();
        
        if (distanceToPlayer <= m_AttackRange)
        {
            // 사거리 안: 제자리에서 공격
            if (m_canFire)
            {
                Attack(deltaTime);
            }
        }
        else
        {
            // 사거리 밖: EngageMove 전이
            m_fromLastStand = true;
            
            if (m_logicFSM)
            {
                // PlayerInFleeRange를 false로 설정해서 EngageMove 전이
                m_logicFSM->SetParameter("PlayerInFleeRange", false);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태별 행동 - 물리 (FixedUpdate에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::UpdatePhysicsStateBasedBehavior(const std::string& state)
    {
        if (state == "Flee")
        {
            ExecuteFleeBehaviorPhysics();
        }
        else if (state == "EngageMove")
        {
            ExecuteEngageMoveBehaviorPhysics();
        }
        else if (state == "EngageStop")
        {
            ExecuteEngageStopBehaviorPhysics();
        }
        else if (state == "EngageAttack")
        {
            ExecuteEngageAttackBehaviorPhysics();
        }
        else if (state == "Idle")
        {
            ExecuteIdleBehaviorPhysics();
        }
        else if (state == "Redemption")
        {
            ExecuteRedemptionBehaviorPhysics();
        }
        else if (state == "Laststand")
        {
            ExecuteLaststandBehaviorPhysics();
        }
        else if (state == "Fragile")
        {
            ExecuteFragileBehaviorPhysics();
        }
        else if (state == "Dead")
        {
            ExecuteDeadBehaviorPhysics();
        }
    }

    void MonsterPointedType::ExecuteFleeBehaviorPhysics()
    {
        // 패스파인딩 기반 도망 이동
        MoveToFleeTarget();
    }

    void MonsterPointedType::ExecuteEngageMoveBehaviorPhysics()
    {
        // 플레이어를 향해 이동 (물리)
        MoveTowardsPlayer();
    }

    void MonsterPointedType::ExecuteEngageStopBehaviorPhysics()
    {
        // 공격 사거리 안에 있지만 쿨타임 중 - 정지하고 회전만 (물리)
        StopAllMovement();
        RotateTowardsDirection(CalculateDirectionToPlayer());
    }

    void MonsterPointedType::ExecuteEngageAttackBehaviorPhysics()
    {
        // 공격 중에는 이동 불가, 회전만 가능 (물리)
        StopAllMovement();
        RotateTowardsDirection(CalculateDirectionToPlayer());
    }

    void MonsterPointedType::ExecuteIdleBehaviorPhysics()
    {
        StopAllMovement();
        StopRotation();
    }

    void MonsterPointedType::ExecuteFragileBehaviorPhysics()
    {
        // Fragile 상태: 물리적으로 정지
    }

    void MonsterPointedType::ExecuteDeadBehaviorPhysics()
    {
        StopAllMovement();
    }

    void MonsterPointedType::ExecuteRedemptionBehaviorPhysics()
    {
        if (!m_rigidbody) return;
        
        // ─────────────────────────────────────────────
        // 이동 방향으로 회전 후 이동
        // ─────────────────────────────────────────────
        RotateTowardsDirection(m_redemptionMoveDir);
        
        float speed = m_moveSpeed * m_redemptionSpeedMultiplier;
        m_rigidbody->SetLinearVelocity(m_redemptionMoveDir * speed);
        
        // ─────────────────────────────────────────────
        // 이동 거리 누적 (속도 * 시간)
        // ─────────────────────────────────────────────
        float fixedDeltaTime = engine::Time::FixedDeltaTime();
        m_redemptionTraveledDistance += speed * fixedDeltaTime;
    }

    void MonsterPointedType::ExecuteLaststandBehaviorPhysics()
    {
        // Laststand: 정지
        StopAllMovement();
        RotateTowardsDirection(CalculateDirectionToPlayer());
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태 진입 콜백
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::OnStateEntered(const std::string& state)
    {
        if (state == "Flee")
        {
            // PA 재활성화 체크
            if (m_pathfindingAgent && !m_pathfindingAgent->IsActive())
            {
                m_pathfindingAgent->SetActive(true);
            }
            
            // Flee 상태 진입 시: 초기화 및 도망 위치 선정
            m_hasFleeTarget = false;
            m_fleeAttemptCount = 0;          // 실패 카운트 리셋
            m_isNoWayOut = false;
            m_redemptionRetryCount = 0;      // Redemption 재시도 카운트 리셋
            
            // 끼임 감지 초기화
            m_lastFleePosition = GetTransform()->GetWorldPosition();
            m_lastFleePositionCheckTime = engine::Time::GetTimestamp();
            
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("NoWayOut", false);
                m_logicFSM->SetParameter("PathFound", false);
            }
            
            TrySelectFleeTarget();
        }
        else if (state == "EngageAttack")
        {
            // 공격 상태 진입 시 이동 멈추고 타이머 초기화
            StopAllMovement();
            m_attackAnimationTimer = 0.0f;
            m_logicFSM->SetParameter("AttackComplete", false);
        }
        else if (state == "EngageMove")
        {
            // PA 재활성화 체크
            if (m_pathfindingAgent && !m_pathfindingAgent->IsActive())
            {
                m_pathfindingAgent->SetActive(true);
            }
            
            // EngageMove 상태 진입 시: 패스 재계산
            if (m_pathfindingAgent && m_targetPlayer)
            {
                engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();
                m_pathfindingAgent->RequestPathImmediate(playerPos);
            }
            
            // LastStand에서 온 경우 플래그 설정
            if (m_fromLastStand)
            {
                m_needsPostAttackCheck = true;
                m_fromLastStand = false;
            }
            
            m_logicFSM->SetParameter("AttackComplete", false);
        }
        else if (state == "EngageStop")
        {
            // EngageStop 상태 진입 시 이동 멈춤
            StopAllMovement();
            m_logicFSM->SetParameter("AttackComplete", false);
        }
        else if (state == "Redemption")
        {
            // PA 비활성화
            if (m_pathfindingAgent && m_pathfindingAgent->IsActive())
            {
                m_pathfindingAgent->SetActive(false);
                m_pathfindingAgent->ClearPath();
            }
            
            // Redemption 상태 진입: 플레이어 방향으로 5m 돌진
            m_redemptionRetryCount++;  // 재시도 카운트 증가
            
            if (m_targetPlayer)
            {
                engine::Vector3 myPos = GetTransform()->GetWorldPosition();
                engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();
                
                // 플레이어 방향 (진입 시 고정)
                m_redemptionMoveDir = playerPos - myPos;
                m_redemptionMoveDir.y = 0.0f;
                if (m_redemptionMoveDir.LengthSquared() > 0.001f)
                {
                    m_redemptionMoveDir.Normalize();
                }
                else
                {
                    // 플레이어와 같은 위치면 임의 방향
                    m_redemptionMoveDir = engine::Vector3(1.0f, 0.0f, 0.0f);
                }
            }
            
            m_redemptionTraveledDistance = 0.0f;  // 거리 리셋
            
            // FSM 파라미터 초기화
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("RedemptionRetry", false);
                m_logicFSM->SetParameter("RedemptionFailed", false);
                m_logicFSM->SetParameter("PathFound", false);
            }
        }
        else if (state == "Laststand")
        {
            // Laststand 상태 진입: 결사항전
            StopAllMovement();
            m_redemptionRetryCount = 0;  // 재시도 카운트 리셋
            
            // FSM 파라미터 초기화
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("PathFound", false);
                m_logicFSM->SetParameter("RedemptionFailed", false);
            }
        }
        else if (state == "Fragile")
        {
            StopAllMovement();
            OnFragile();
        }
        else if (state == "Dead")
        {
            StopAllMovement();
            OnDeath();
        }
        else if (state == "Idle")
        {
            StopAllMovement();
            StopRotation();
            
            // Redemption 재시도를 위한 Idle인 경우 0.1초 후 자동 전이
            // (NoWayOut이 true면 Redemption 재시도용 Idle)
            if (m_isNoWayOut)
            {
                m_idleTimer = 0.0f;  // Idle 타이머 초기화 (0.1초 후 전이)
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 공격
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::Attack(float deltaTime)
    {
        // 공격 애니메이션 타이머 업데이트
        m_attackAnimationTimer += deltaTime;

        // 발사 가능 체크 (단발/연사 모두 지원)
        if (CanFireBullet())
        {
            if (m_bulletFactory && m_targetPlayer && m_targetPlayer->GetGameObject())
            {
                // 플레이어를 향해 발사
                engine::Vector3 direction = CalculateDirectionToPlayer();
                engine::Vector3 firePosition = GetTransform()->GetWorldPosition() + m_fireOffset;

                switch (m_monsterTier)
                {
                // ─────────────────────────────────────────────
                // 뾰족 회색
                // 
                // 플레이어를 천천히 추격하며 공격 사거리에 들어오면 멈춰서서 투사체를 발사
                // ─────────────────────────────────────────────
                case MonsterTier::Gray:
                {
                    m_bulletFactory->LinearFireMonster(firePosition, direction, m_bulletParams);
                    break;
                }
                // ─────────────────────────────────────────────
                // 뾰족 초록 (둔탁녹색과 동일한 포물선 발사 방식)
                // 
                // 플레이어를 추격하며 멈춰 서서 투사체를 발사하고 착탄 지점에 범위 공격
                // - 에디터 설정: m_ownGravity (5~20)
                // - 자동 계산: speed (AttackRange 기준), launchAngle (거리 선형 매핑)
                // ─────────────────────────────────────────────
                case MonsterTier::Green:
                {
                    // 발사 오프셋 설정
                    float bulletStartOffsetY = 1.5f;
                    
                    engine::Vector3 bulletStartPos = firePosition;
                    bulletStartPos.y = bulletStartOffsetY;
                    
                    // 착탄점 설정 (플레이어 XZ, Y=0)
                    engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();
                    engine::Vector3 targetPos(playerPos.x, 0.0f, playerPos.z);
                    
                    // 발사각 및 속도 자동 계산 (새 로직)
                    float angleDeg = 0.0f;
                    float calculatedSpeed = 0.0f;
                    CalculateParabolicLaunchAngle(bulletStartPos, targetPos, angleDeg, calculatedSpeed);
                    
                    // BulletParams에 값 설정
                    m_bulletParams.speed = calculatedSpeed;           // 역산된 속도
                    m_bulletParams.launchAngle = angleDeg;            // 계산된 발사각 (도)
                    m_bulletParams.ownGravity = m_ownGravity;         // 에디터 설정값 (고정)
                    m_bulletParams.minLaunchAngle = m_minLaunchAngle;
                    m_bulletParams.maxLaunchAngle = m_maxLaunchAngle;
                    m_bulletParams.explosionRadius = m_explosionRadius; // 폭발 반경
                    
                    // 실제 발사
                    m_bulletFactory->ParabolicFireMonster(bulletStartPos, direction, m_bulletParams);
                    break;
                }
                // ─────────────────────────────────────────────
                // 뾰족 파랑
                // 
                // 플레이어를 추격하며 멈춰 서서 3방향 투사체 발사
                // - m_spreadAngle: 좌우 퍼짐 각도 (인스펙터에서 설정 가능)
                // ─────────────────────────────────────────────
                case MonsterTier::Blue:
                {
                    m_bulletFactory->ThreewayFireMonster(firePosition, direction, m_spreadAngle, m_bulletParams);
                    break;
				}
                // ─────────────────────────────────────────────
                // 뾰족 빨강
                // 
				// 공격 범위에 들어오면 플레이어 주변에게 투사체를 난사 (8 ~ 15발)
                // ─────────────────────────────────────────────
                case MonsterTier::Red:
                {
                    int projectileCount = 8 + (rand() % 8);  // 8~15발 랜덤
                    constexpr float spreadAngle = DirectX::XMConvertToRadians(60.0f);  // ±30도
                    
                    m_bulletFactory->BurstFireMonster(
                        firePosition,
                        direction,
                        projectileCount,
                        spreadAngle * 0.5f,  // 함수는 ±범위를 받으므로 절반 전달
                        0.1f,                // lifetimeModMin
                        0.8f,                // lifetimeModMax (0.1 + 0.7)
                        0.6f,                // speedModMin
                        1.0f,                // speedModMax (0.6 + 0.4)
                        m_bulletParams
                    );
                    break;
				}
                // ─────────────────────────────────────────────
                // 뾰족 보라
                // 
                // 플레이어에게 멀어지려고 하며 가까이 오면 도망, 긴 사거리에서 플레이어를 조준, 공격
                // ─────────────────────────────────────────────
                case MonsterTier::Purple:
                {
                    m_bulletFactory->LinearFireMonster(firePosition, direction, m_bulletParams);
                    break;
                }
                default:
                    m_bulletFactory->LinearFireMonster(firePosition, direction, m_bulletParams);
                    break;
                }

                // 뾰족 타입은 StaticMesh를 사용하므로 공격 애니메이션 재생 불필요
                
                // ─────────────────────────────────────────────
                // BulletFactory 발사 직후 체크 (LastStand → EngageMove 경로)
                // ─────────────────────────────────────────────
                if (m_needsPostAttackCheck)
                {
                    CheckPostAttackTransition();
                }

                // 연발 모드일 때만 타이머 리셋 (단발 모드는 타이머 무시)
                if (!m_isDoSingleShot && m_fireRate > 0.0f)
                {
                    m_fireTimer = m_fireRate;
                }
            }
        }
        else if (!m_isDoSingleShot && m_fireRate > 0.0f)
        {
            // 연발 모드: 타이머 감소
            m_fireTimer -= deltaTime;
        }
          
        // 공격 애니메이션 완료 체크
        if (m_attackAnimationTimer >= m_attackAnimationDuration)
        {
            m_logicFSM->SetParameter("AttackComplete", true);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 행동 제한
    // ═══════════════════════════════════════════════════════════════
    bool MonsterPointedType::CanMove() const
    {
        std::string state = GetCurrentState();
        // EngageStop, EngageAttack 상태에서는 이동 불가
        return state != "Fragile" && state != "Dead" && state != "EngageStop" && state != "EngageAttack";
    }

    bool MonsterPointedType::CanAttack() const
    {
        std::string state = GetCurrentState();
        return state == "EngageAttack" && state != "Fragile" && state != "Dead";
    }

    // ═══════════════════════════════════════════════════════════════
    // 헬퍼 함수
    // ═══════════════════════════════════════════════════════════════
    bool MonsterPointedType::IsPlayerInDetectionRange() const
    {
        return IsTargetInRange(m_targetPlayer ? m_targetPlayer->GetGameObject() : nullptr, m_detectionRange);
    }

    // ═══════════════════════════════════════════════════════════════
    // 뾰족 보라 - 패스파인딩 기반 도망
    // ═══════════════════════════════════════════════════════════════
    bool MonsterPointedType::IsPositionSafeForFlee(const engine::Vector3& position) const
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
        if (m_fleeSafetyMargin <= 0)
        {
            return true;
        }
        
        // 주변 셀 체크 (NxN 범위)
        for (int dz = -m_fleeSafetyMargin; dz <= m_fleeSafetyMargin; ++dz)
        {
            for (int dx = -m_fleeSafetyMargin; dx <= m_fleeSafetyMargin; ++dx)
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
    
    bool MonsterPointedType::TrySelectFleeTarget()
    {
        if (!m_targetPlayer || !m_pathfindingAgent) return false;

        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();

        // 플레이어 → 몬스터 방향 (도망 기본 방향 = 180도)
        engine::Vector3 awayFromPlayer = myPos - playerPos;
        awayFromPlayer.y = 0.0f;
        awayFromPlayer.Normalize();

        // 기본 방향의 각도 계산 (XZ 평면)
        float baseAngle = std::atan2(awayFromPlayer.z, awayFromPlayer.x);
        
        // ─────────────────────────────────────────────
        // Wall 충돌 정보 유효성 체크 (3초 이내)
        // ─────────────────────────────────────────────
        bool useWallInfo = false;
        float wallAwayAngle = 0.0f;
        
        if (m_hasWallCollisionInfo)
        {
            float elapsedTime = engine::Time::GetElapsedSeconds(m_lastWallCollisionTime);
            if (elapsedTime < m_wallCollisionInfoTimeout)
            {
                // 벽 반대 방향 (벽 노말 방향) 계산
                wallAwayAngle = std::atan2(m_lastWallCollisionNormal.z, m_lastWallCollisionNormal.x);
                useWallInfo = true;
            }
            else
            {
                // 타임아웃 - 정보 무효화
                m_hasWallCollisionInfo = false;
            }
        }

        // ─────────────────────────────────────────────
        // 도망 위치 탐색 (Wall 정보 우선)
        // ─────────────────────────────────────────────
        constexpr int kWallPriorityAttempts = 5;  // 벽 반대 방향 우선 시도
        constexpr int kNormalAttempts = 5;        // 일반 시도
        constexpr int kMaxAttempts = 10;
        
        for (int attempt = 0; attempt < kMaxAttempts; ++attempt)
        {
            float finalAngle;
            
            // 전반부 5회: 벽 반대 방향 우선 (±45도)
            if (useWallInfo && attempt < kWallPriorityAttempts)
            {
                // 벽 반대 방향 기준 ±45도
                float randomOffsetDeg = (static_cast<float>(rand()) / RAND_MAX) * 90.0f - 45.0f;
                float randomOffsetRad = DirectX::XMConvertToRadians(randomOffsetDeg);
                finalAngle = wallAwayAngle + randomOffsetRad;
            }
            // 후반부 5회: 플레이어 반대 방향 (±90도)
            else
            {
                // 플레이어 반대 방향 기준 ±90도
                float randomOffsetDeg = (static_cast<float>(rand()) / RAND_MAX) * 180.0f - 90.0f;
                float randomOffsetRad = DirectX::XMConvertToRadians(randomOffsetDeg);
                finalAngle = baseAngle + randomOffsetRad;
            }

            // 랜덤 거리: min ~ max 범위
            float randomDistance = m_fleeDistanceMin + 
                (static_cast<float>(rand()) / RAND_MAX) * (m_fleeDistanceMax - m_fleeDistanceMin);

            // 도망 목표 위치 계산
            engine::Vector3 targetPos;
            targetPos.x = myPos.x + randomDistance * std::cos(finalAngle);
            targetPos.y = myPos.y;
            targetPos.z = myPos.z + randomDistance * std::sin(finalAngle);
            
            // 안전 마진 체크 (대각선 경계 + 주변 여유 공간)
            if (!IsPositionSafeForFlee(targetPos))
            {
                continue;  // 안전하지 않으면 다음 시도
            }

            // 즉시 패스파인딩 시도 (지연 없이 바로 계산)
            m_pathfindingAgent->RequestPathImmediate(targetPos);
            
            // 패스가 유효한지 확인
            if (m_pathfindingAgent->HasPath())
            {
                m_fleeTargetPos = targetPos;
                m_hasFleeTarget = true;
                m_fleeAttemptCount = 0;  // 성공 시 카운트 리셋
                return true;
            }
        }

        // 10회 실패: 실패 카운트 누적
        m_fleeAttemptCount += kMaxAttempts;
        m_hasFleeTarget = false;
        
        // 10회 누적 실패 시 NoWayOut → Redemption 전이
        if (m_fleeAttemptCount >= kMaxFleeAttempts)
        {
            m_isNoWayOut = true;
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("NoWayOut", true);
            }
        }
        
        return false;
    }

    void MonsterPointedType::MoveToFleeTarget()
    {
        if (!m_pathfindingAgent || !m_hasFleeTarget) return;

        // 패스가 있으면 패스파인딩 이동
        if (m_pathfindingAgent->HasPath())
        {
            engine::Vector3 waypoint;
            if (m_pathfindingAgent->GetCurrentWaypoint(waypoint))
            {
                // 웨이포인트로 이동
                engine::Vector3 myPos = GetTransform()->GetWorldPosition();
                engine::Vector3 dir = waypoint - myPos;
                dir.y = 0.0f;
                
                float distToWaypoint = dir.Length();
                if (distToWaypoint > 0.1f)
                {
                    dir.Normalize();
                    RotateTowardsDirection(dir);
                    if (m_rigidbody)
                    {
                        float speed = m_moveSpeed * m_fleeSpeedMultiplier;
                        m_rigidbody->SetLinearVelocity(dir * speed);
                    }
                }
                
                // 웨이포인트 도달 체크
                if (distToWaypoint < 1.0f)
                {
                    m_pathfindingAgent->AdvanceToNextWaypoint();
                }
            }
        }
        else
        {
            // 패스가 없으면 새 도망 위치 선정 시도
            if (!TrySelectFleeTarget())
            {
                // 실패 시: 정지 (대체 행동)
                StopAllMovement();
            }
        }

        // 도망 목표 도달 체크
        if (m_hasFleeTarget)
        {
            engine::Vector3 myPos = GetTransform()->GetWorldPosition();
            float distToTarget = engine::Vector3::Distance(myPos, m_fleeTargetPos);
            
            if (distToTarget < 2.0f)  // 도망 목표 근처 도달
            {
                // 플레이어가 아직 임계거리 안에 있으면 새 도망 위치 선정
                if (m_isPlayerInFleeRange)
                {
                    TrySelectFleeTarget();
                }
                // 임계거리 밖이면 FSM이 알아서 EngageMove로 전이
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 공격 후 전이 체크 (LastStand → EngageMove 경로용)
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::CheckPostAttackTransition()
    {
        m_needsPostAttackCheck = false;  // 플래그 해제
        
        if (!m_targetPlayer) return;
        
        // ─────────────────────────────────────────────
        // 거리 체크
        // ─────────────────────────────────────────────
        float distanceToPlayer = GetDistanceToPlayer();
        
        if (distanceToPlayer > m_fleeRange)
        {
            // Flee 거리: Flee 패스 체크
            if (TrySelectFleeTarget())
            {
                // 패스 있음 → Flee 전이
                if (m_logicFSM)
                {
                    m_logicFSM->SetParameter("PlayerInFleeRange", true);
                }
            }
            // 패스 없음 → EngageMove 유지 (계속 추격)
        }
        // Engage 거리 → EngageMove 유지 (계속 추격)
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백 (Redemption 반사용)
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::OnCollisionEnter(const engine::CollisionInfo& info)
    {
        if (!info.gameObject) return;
        
        // ─────────────────────────────────────────────
        // Wall 충돌 정보 저장 (모든 상태에서, 도망 방향 우선순위용)
        // ─────────────────────────────────────────────
        auto* collider = info.gameObject->GetComponent<engine::Collider>();
        if (collider)
        {
            uint32_t layer = collider->GetLayer();
            if (layer == engine::PhysicsLayer::Index::Wall)
            {
                // 충돌 노말 추출
                for (const auto& contact : info.contacts)
                {
                    engine::Vector3 normal = -contact.normal;
                    normal.y = 0.0f;
                    if (normal.LengthSquared() > 0.0001f)
                    {
                        normal.Normalize();
                        m_lastWallCollisionNormal = normal;
                        m_lastWallCollisionTime = engine::Time::GetTimestamp();
                        m_hasWallCollisionInfo = true;
                        break;
                    }
                }
            }
        }
        
        // ─────────────────────────────────────────────
        // Redemption 상태에서만 반사 처리
        // ─────────────────────────────────────────────
        std::string currentState = GetCurrentState();
        if (currentState != "Redemption") return;
        
        // ─────────────────────────────────────────────
        // 레이어 체크: Wall/Environment만 반사
        // ─────────────────────────────────────────────
        auto* otherCollider = info.collider.Get();
        if (!otherCollider) return;
        
        uint32_t layer = otherCollider->GetLayer();
        if (layer != engine::PhysicsLayer::Index::Wall &&
            layer != engine::PhysicsLayer::Index::Environment)
        {
            return;  // Player/Enemy는 무시 (통과)
        }

        // ─────────────────────────────────────────────
        // 충돌 노말 추출
        // ─────────────────────────────────────────────
        engine::Vector3 collisionNormal = engine::Vector3::Zero;
        for (const auto& contact : info.contacts)
        {
            engine::Vector3 normal = -contact.normal;
            normal.y = 0.0f;
            if (normal.LengthSquared() > 0.0001f)
            {
                collisionNormal = normal;
                collisionNormal.Normalize();
                break;
            }
        }
        
        // 노말이 없으면 위치 기반으로 계산
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
            else
            {
                return;  // 유효한 노말 없음
            }
        }
        
        // ─────────────────────────────────────────────
        // 노말을 X축 또는 Z축으로 스냅
        // ─────────────────────────────────────────────
        engine::Vector3 snappedNormal = SnapNormalToAxis(collisionNormal);
        
        // ─────────────────────────────────────────────
        // 정반사 계산: R = V - 2(V·N)N
        // ─────────────────────────────────────────────
        m_redemptionMoveDir = ReflectDirection(m_redemptionMoveDir, snappedNormal);
        m_redemptionMoveDir.Normalize();
    }

    // ═══════════════════════════════════════════════════════════════
    // 반사 헬퍼 함수 (MonsterRoundGreen 방식)
    // ═══════════════════════════════════════════════════════════════
    engine::Vector3 MonsterPointedType::SnapNormalToAxis(const engine::Vector3& normal) const
    {
        float absX = std::abs(normal.x);
        float absZ = std::abs(normal.z);
        
        if (absX >= absZ)
        {
            return engine::Vector3((normal.x >= 0.0f) ? 1.0f : -1.0f, 0.0f, 0.0f);
        }
        else
        {
            return engine::Vector3(0.0f, 0.0f, (normal.z >= 0.0f) ? 1.0f : -1.0f);
        }
    }

    engine::Vector3 MonsterPointedType::ReflectDirection(const engine::Vector3& direction, const engine::Vector3& normal) const
    {
        float dotProduct = direction.Dot(normal);
        engine::Vector3 reflected = direction - 2.0f * dotProduct * normal;
        return reflected;
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::OnGui()
    {
        ImGui::Indent();
        
        ImGui::Text("=== MonsterPointedType ===");
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Type: Pointed (Static, With Pathfinding)");
        
        // 부모 클래스 OnGui 호출 (공통 설정)
        MonsterScript::OnGui();
        
        // ─────────────────────────────────────────────
        // Pointed 전용 설정 - Monster Tier 선택
        // ─────────────────────────────────────────────
        // 공격 타입 (읽기 전용 - Pointed 고정)
        ImGui::Separator();
        ImGui::Text("Type:");
        ImGui::BeginDisabled(true);
        if (ImGui::BeginCombo("Attack Type", GetAttackTypeStr(m_attackType)))
        {
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();

        if (ImGui::BeginCombo("Monster Tier", GetMonsterTierStr(m_monsterTier)))
        {
            for (int i = 0; i < (int)MonsterTier::Max; ++i)
            {
                MonsterTier currentTier = (MonsterTier)i;
                bool isSelected = (m_monsterTier == currentTier);

                if (ImGui::Selectable(GetMonsterTierStr(currentTier), isSelected))
                {
                    m_monsterTier = currentTier;
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // ─────────────────────────────────────────────
        // 3방향 발사 설정 (Blue일 때만 표시)
        // ─────────────────────────────────────────────
        if (m_monsterTier == MonsterTier::Blue)
        {
            ImGui::Separator();
            ImGui::Text("=== Threeway Bullet Settings ===");
            
            ImGui::DragFloat("Spread Angle", &m_spreadAngle, 0.01f, 0.0f, 1.5f, "%.2f rad");
            
            float spreadDeg = m_spreadAngle * 180.0f / 3.14159265f;
            ImGui::Text("  = %.1f degrees", spreadDeg);
            
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
                "(Left: -%.2f rad, Center: 0, Right: +%.2f rad)", m_spreadAngle, m_spreadAngle);
        }

        // ─────────────────────────────────────────────
        // Flee 설정 (Purple일 때만 표시)
        // ─────────────────────────────────────────────
        if (m_monsterTier == MonsterTier::Purple)
        {
            ImGui::Separator();
            ImGui::Text("=== Flee Settings (Purple) ===");
            
            ImGui::DragFloat("Flee Range", &m_fleeRange, 0.5f, 1.0f, 50.0f, "%.1f m");
            ImGui::DragFloat("Safe Range", &m_safeRange, 0.5f, 1.0f, 50.0f, "%.1f m");
            ImGui::DragFloat("Flee Speed Mult", &m_fleeSpeedMultiplier, 0.1f, 0.5f, 3.0f, "%.1fx");
            
            ImGui::Spacing();
            ImGui::Text("Pathfinding Flee:");
            ImGui::DragFloat("Flee Dist Min", &m_fleeDistanceMin, 1.0f, 5.0f, 100.0f, "%.0f m");
            ImGui::DragFloat("Flee Dist Max", &m_fleeDistanceMax, 1.0f, 10.0f, 150.0f, "%.0f m");
            ImGui::DragInt("Safety Margin", &m_fleeSafetyMargin, 0.1f, 0, 5);
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("0 = No margin\n1 = 3x3 cells (recommended)\n2 = 5x5 cells\nHigher = safer but fewer positions");
            }
            ImGui::DragFloat("Wall Info Timeout", &m_wallCollisionInfoTimeout, 0.1f, 0.5f, 10.0f, "%.1f sec");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("How long wall collision info is used for flee direction priority");
            }
            
            ImGui::Spacing();
            ImGui::Text("Flee Stuck Detection:");
            ImGui::DragFloat("Stuck Check Interval", &m_fleeStuckCheckInterval, 0.1f, 0.5f, 10.0f, "%.1f sec");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("How often to check if monster is stuck during Flee");
            }
            ImGui::DragFloat("Stuck Distance Threshold", &m_fleeStuckDistanceThreshold, 0.1f, 0.1f, 5.0f, "%.1f m");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("If moved less than this distance, consider stuck");
            }
            
            ImGui::Spacing();
            ImGui::Text("Redemption/Laststand:");
            ImGui::DragFloat("Redemption Speed Mult", &m_redemptionSpeedMultiplier, 0.1f, 0.5f, 5.0f, "%.1fx");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Speed multiplier during Redemption dash (default 2.0x)");
            }
            ImGui::DragFloat("Redemption Distance", &m_redemptionTargetDistance, 0.5f, 1.0f, 10.0f, "%.1f m");
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Target distance for Redemption dash (default 5.0m)");
            }
            
            // 런타임 상태 표시
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Runtime (read-only):");
            ImGui::Text("  Has Flee Target: %s", m_hasFleeTarget ? "Yes" : "No");
            if (m_hasFleeTarget)
            {
                ImGui::Text("  Target Pos: (%.1f, %.1f, %.1f)", 
                    m_fleeTargetPos.x, m_fleeTargetPos.y, m_fleeTargetPos.z);
                
                engine::Vector3 myPos = GetTransform()->GetWorldPosition();
                float dist = engine::Vector3::Distance(myPos, m_fleeTargetPos);
                ImGui::Text("  Distance to Target: %.1f m", dist);
            }
            
            ImGui::Text("  Flee Attempt Count: %d / %d", m_fleeAttemptCount, kMaxFleeAttempts);
            ImGui::Text("  Is No Way Out: %s", m_isNoWayOut ? "Yes" : "No");
            ImGui::Text("  Redemption Retry Count: %d / %d", m_redemptionRetryCount, kMaxRedemptionRetries);
            ImGui::Text("  Redemption Distance: %.2f / %.2f m", m_redemptionTraveledDistance, m_redemptionTargetDistance);
            ImGui::Text("  From LastStand: %s", m_fromLastStand ? "Yes" : "No");
            ImGui::Text("  Needs Post Attack Check: %s", m_needsPostAttackCheck ? "Yes" : "No");
            
            ImGui::Spacing();
            ImGui::Text("  Wall Collision Info:");
            ImGui::Text("    Has Info: %s", m_hasWallCollisionInfo ? "Yes" : "No");
            if (m_hasWallCollisionInfo)
            {
                float elapsedTime = engine::Time::GetElapsedSeconds(m_lastWallCollisionTime);
                ImGui::Text("    Elapsed: %.2f / %.2f sec", elapsedTime, m_wallCollisionInfoTimeout);
                ImGui::Text("    Normal: (%.2f, %.2f)", m_lastWallCollisionNormal.x, m_lastWallCollisionNormal.z);
            }
            
            ImGui::Spacing();
            ImGui::Text("  Flee Stuck Detection:");
            float elapsedSinceCheck = engine::Time::GetElapsedSeconds(m_lastFleePositionCheckTime);
            ImGui::Text("    Check Timer: %.2f / %.2f sec", elapsedSinceCheck, m_fleeStuckCheckInterval);
            engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
            float currentDistance = engine::Vector3::Distance(m_lastFleePosition, currentPos);
            ImGui::Text("    Distance Moved: %.2f / %.2f m", currentDistance, m_fleeStuckDistanceThreshold);
            ImGui::TextColored(
                currentDistance < m_fleeStuckDistanceThreshold ? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(0.3f, 1.0f, 0.3f, 1.0f),
                "    Status: %s", currentDistance < m_fleeStuckDistanceThreshold ? "STUCK!" : "Moving");
        }

        // 런타임 정보
        ImGui::Separator();
        ImGui::Text("Runtime Info:");
        ImGui::Text("Current State: %s", GetCurrentState().c_str());
        ImGui::Text("Player Found: %s", m_targetPlayer ? "Yes" : "No");
        if (m_targetPlayer)
        {
            float distToPlayer = GetDistanceToPlayer();
            ImGui::Text("Distance to Player: %.2f", distToPlayer);
            ImGui::Text("Path Distance to Player: %.2f", GetPathDistanceToPlayer());
            ImGui::Text("Player In Detection Range: %s", IsPlayerInDetectionRange() ? "Yes" : "No");
            ImGui::Text("Player In Attack Range: %s", IsPlayerInRange() ? "Yes" : "No");
            ImGui::Text("Looking at Player: %s", IsLookingAtPlayer() ? "Yes" : "No");
        }
        ImGui::Text("Can Fire: %s", m_canFire ? "Yes" : "No");
        ImGui::Text("Fire Timer: %.2f", m_fireTimer);
        ImGui::Text("Attack Anim Timer: %.2f / %.2f", m_attackAnimationTimer, m_attackAnimationDuration);
        
        // Pathfinding 정보
        if (m_pathfindingAgent)
        {
            ImGui::Separator();
            ImGui::Text("=== Pathfinding Info ===");
            ImGui::Text("Has Path: %s", m_pathfindingAgent->HasPath() ? "Yes" : "No");
            if (m_pathfindingAgent->HasPath())
            {
                ImGui::Text("Path Length: %d", static_cast<int>(m_pathfindingAgent->GetPath().size()));
                engine::Vector3 waypoint;
                if (m_pathfindingAgent->GetCurrentWaypoint(waypoint))
                {
                    ImGui::Text("Current Waypoint: (%.1f, %.1f, %.1f)", waypoint.x, waypoint.y, waypoint.z);
                }
            }
        }
        
        ImGui::Unindent();
        
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Spacing();
    }

    void MonsterPointedType::Save(engine::json& j) const
    {
        MonsterScript::Save(j);
        
        j["DetectionRange"] = m_detectionRange;
        j["AttackAnimationDuration"] = m_attackAnimationDuration;
        
        // Flee 설정 (Purple용)
        j["FleeRange"] = m_fleeRange;
        j["SafeRange"] = m_safeRange;
        j["FleeSpeedMultiplier"] = m_fleeSpeedMultiplier;
        j["FleeDistanceMin"] = m_fleeDistanceMin;
        j["FleeDistanceMax"] = m_fleeDistanceMax;
        j["FleeSafetyMargin"] = m_fleeSafetyMargin;
        j["WallCollisionInfoTimeout"] = m_wallCollisionInfoTimeout;
        j["FleeStuckCheckInterval"] = m_fleeStuckCheckInterval;
        j["FleeStuckDistanceThreshold"] = m_fleeStuckDistanceThreshold;
        
        // Redemption/Laststand 설정 (Purple용)
        j["RedemptionSpeedMultiplier"] = m_redemptionSpeedMultiplier;
        j["RedemptionTargetDistance"] = m_redemptionTargetDistance;
    }

    void MonsterPointedType::Load(const engine::json& j)
    {
        MonsterScript::Load(j);
        
        m_detectionRange = j.value("DetectionRange", 15.0f);
        m_attackAnimationDuration = j.value("AttackAnimationDuration", 1.0f);
        
        // Flee 설정 (Purple용)
        m_fleeRange = j.value("FleeRange", 10.0f);
        m_safeRange = j.value("SafeRange", 14.0f);
        m_fleeSpeedMultiplier = j.value("FleeSpeedMultiplier", 1.7f);
        m_fleeDistanceMin = j.value("FleeDistanceMin", 20.0f);
        m_fleeDistanceMax = j.value("FleeDistanceMax", 70.0f);
        m_fleeSafetyMargin = j.value("FleeSafetyMargin", 1);
        m_wallCollisionInfoTimeout = j.value("WallCollisionInfoTimeout", 3.0f);
        m_fleeStuckCheckInterval = j.value("FleeStuckCheckInterval", 2.0f);
        m_fleeStuckDistanceThreshold = j.value("FleeStuckDistanceThreshold", 0.5f);
        
        // Redemption/Laststand 설정 (Purple용)
        m_redemptionSpeedMultiplier = j.value("RedemptionSpeedMultiplier", 2.0f);
        m_redemptionTargetDistance = j.value("RedemptionTargetDistance", 5.0f);
    }
}
