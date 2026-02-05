#include "GamePCH.h"
#include "MonsterPointedType.h"

#include "Script/CharacterScript/Common/BulletFactory.h"

#include <Framework/Object/Component/AnimFSM.h>
#include <Framework/Object/Component/Pathfinding/PathfindingAgent.h>
#include <Framework/Object/Component/Rigidbody.h>

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
        
        // 스탯은 씬 파일에서 로드됨 (Load 함수 참조)
    }

    void MonsterPointedType::Start()
    {
        MonsterScript::Start();

        // PathfindingAgent 설정 (이동 몬스터용)
        if (m_pathfindingAgent)
        {
            m_pathfindingAgent->SetPathUpdateInterval(0.5f);        // 0.5초마다 경로 재계산
            m_pathfindingAgent->SetWaypointReachDistance(1.0f);     // waypoint 도달 거리
            m_pathfindingAgent->SetTargetMoveThreshold(2.0f);       // 목표가 2.0f 이상 움직이면 재계산
        }
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
        m_logicFSM->SetParameter("NoWayOut", false);           // Flee → Redemption
        m_logicFSM->SetParameter("RedemptionFailed", false);   // Redemption → Laststand
        m_logicFSM->SetParameter("PathFound", false);          // Laststand → Flee/EngageMove

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

            // Flee → Redemption (패스찾기 120회 실패)
            AddFSMTransition("Flee", "Redemption", "NoWayOut", BoolTrue());
            
            // Redemption → Laststand (2회 반사 후 패스찾기 실패)
            AddFSMTransition("Redemption", "Laststand", "RedemptionFailed", BoolTrue());
            
            // Laststand → Flee/EngageMove (5초마다 패스찾기 성공 시)
            AddFSMTransition("Laststand", "Flee", "PathFound", BoolTrue(), "PlayerInFleeRange", BoolTrue());
            AddFSMTransition("Laststand", "EngageMove", "PathFound", BoolTrue(), "PlayerInFleeRange", BoolFalse());
            
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
            m_bulletParams.damage = 10;
            break;
        case MonsterTier::Blue:
            m_bulletParams.type = BulletType::Linear;
            m_bulletParams.speed = m_bulletSpeed;
            m_bulletParams.lifetime = m_bulletLifetime;
            m_bulletParams.damage = 10;
            break;
        case MonsterTier::Green:
            // ─────────────────────────────────────────────
            // 포물선 전용 파라미터 (둔탁녹색과 동일한 방식)
            // - launchAngle은 Attack() 시점에 거리 기반 자동 계산
            // - speed, ownGravity는 에디터에서 설정한 값 사용
            // ─────────────────────────────────────────────
            m_bulletParams.type = BulletType::Parabolic;
            m_bulletParams.speed = m_parabolicSpeed;      // 에디터 설정값
            m_bulletParams.launchAngle = 45.0f;           // 기본값 (Attack에서 자동 계산)
            m_bulletParams.ownGravity = m_ownGravity;     // 에디터 설정값
            m_bulletParams.lifetime = m_bulletLifetime;
            m_bulletParams.damage = 15;
			break;
		case MonsterTier::Purple:
            m_bulletParams.type = BulletType::Linear;
            m_bulletParams.speed = m_bulletSpeed;
            m_bulletParams.lifetime = m_bulletLifetime;
            m_bulletParams.damage = 10;
            m_AttackRange = 18.0f;
            m_detectionRange = 30.0f;
            break;
        default:
            m_bulletParams.type = BulletType::Linear;
            m_bulletParams.speed = m_bulletSpeed;
            m_bulletParams.lifetime = m_bulletLifetime;
            m_bulletParams.damage = 10;
			break;
        }
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
        float rangeBuffer = (GetCurrentState() == "EngageStop") ? 1.2f : 1.0f;

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
            ExecuteIdleBehaviorNonPhysics();
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
        // 도망 중에도 플레이어를 바라보거나, 쿨타임 계산 등
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
    }

    void MonsterPointedType::ExecuteIdleBehaviorNonPhysics()
    {
        // 비물리 Idle 처리
    }

    void MonsterPointedType::ExecuteFragileBehaviorNonPhysics()
    {
        // Fragile 상태: 아무 행동도 하지 않음 (Execution 대기)
    }

    void MonsterPointedType::ExecuteDeadBehaviorNonPhysics()
    {
        // 부모 클래스의 Dead 타이머 처리 (2초 후 Destroy)
        MonsterScript::ExecuteDeadBehaviorNonPhysics();
    }

    void MonsterPointedType::ExecuteRedemptionBehaviorNonPhysics(float deltaTime)
    {
        // Redemption: 충돌 반사로 이동 중
        // 반사 횟수는 OnCollisionEnter에서 관리
        // 비물리에서는 특별히 할 일 없음
    }

    void MonsterPointedType::ExecuteLaststandBehaviorNonPhysics(float deltaTime)
    {
        // 5초마다 패스찾기 재시도
        m_laststandTimer += deltaTime;
        
        if (m_laststandTimer >= m_laststandRetryInterval)
        {
            m_laststandTimer = 0.0f;
            
            // 20회 패스찾기 시도
            if (!m_targetPlayer || !m_pathfindingAgent) return;

            engine::Vector3 myPos = GetTransform()->GetWorldPosition();
            engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();

            engine::Vector3 awayFromPlayer = myPos - playerPos;
            awayFromPlayer.y = 0.0f;
            awayFromPlayer.Normalize();

            float baseAngle = std::atan2(awayFromPlayer.z, awayFromPlayer.x);

            for (int attempt = 0; attempt < kLaststandPathAttempts; ++attempt)
            {
                float randomOffsetDeg = (static_cast<float>(rand()) / RAND_MAX) * 180.0f - 90.0f;
                float randomOffsetRad = DirectX::XMConvertToRadians(randomOffsetDeg);
                float finalAngle = baseAngle + randomOffsetRad;

                float randomDistance = m_fleeDistanceMin + 
                    (static_cast<float>(rand()) / RAND_MAX) * (m_fleeDistanceMax - m_fleeDistanceMin);

                // 도망 목표 위치 계산 + 맵 경계로 클램핑
                engine::Vector3 targetPos;
                targetPos.x = std::clamp(myPos.x + randomDistance * std::cos(finalAngle), m_mapBoundXMin, m_mapBoundXMax);
                targetPos.y = myPos.y;
                targetPos.z = std::clamp(myPos.z + randomDistance * std::sin(finalAngle), m_mapBoundZMin, m_mapBoundZMax);

                m_pathfindingAgent->RequestPathImmediate(targetPos);
                
                if (m_pathfindingAgent->HasPath())
                {
                    m_fleeTargetPos = targetPos;
                    m_hasFleeTarget = true;
                    m_fleeAttemptCount = 0;
                    m_isNoWayOut = false;
                    
                    // 패스 찾음 → 거리에 따라 Flee 또는 EngageMove로 전이
                    if (m_logicFSM)
                    {
                        m_logicFSM->SetParameter("PathFound", true);
                        m_logicFSM->SetParameter("RedemptionFailed", false);
                    }
                    return;
                }
            }
        }
        
        // 사거리 내 플레이어 있으면 공격
        if (m_isPlayerInRange && m_canFire)
        {
            Attack(deltaTime);
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
        RotateTowardsPlayer();
    }

    void MonsterPointedType::ExecuteEngageAttackBehaviorPhysics()
    {
        // 공격 중에는 이동 불가, 회전만 가능 (물리)
        StopAllMovement();
        RotateTowardsPlayer();
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
        // Redemption: 플레이어 반대 방향으로 1.5배속 이동
        if (m_rigidbody)
        {
            float speed = m_moveSpeed * m_redemptionSpeedMultiplier;
            m_rigidbody->SetLinearVelocity(m_redemptionMoveDir * speed);
        }
    }

    void MonsterPointedType::ExecuteLaststandBehaviorPhysics()
    {
        // Laststand: 정지
        StopAllMovement();
        
        // 플레이어 바라보기
        RotateTowardsPlayer();
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태 진입 콜백
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::OnStateEntered(const std::string& state)
    {
        if (state == "Flee")
        {
            // Flee 상태 진입 시: 초기화 및 도망 위치 선정
            m_hasFleeTarget = false;
            m_fleeAttemptCount = 0;  // 실패 카운트 리셋
            m_isNoWayOut = false;
            
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("NoWayOut", false);
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
            // EngageMove 상태 진입 시: 이동은 MoveTowardsPlayer()에서 AddForce로 처리됨
            // 물리 상태 초기화 불필요 (충돌 응답 보존을 위해)
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
            // Redemption 상태 진입: 플레이어 반대 방향으로 초기 이동 방향 설정
            if (m_targetPlayer)
            {
                engine::Vector3 myPos = GetTransform()->GetWorldPosition();
                engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();
                
                m_redemptionMoveDir = myPos - playerPos;
                m_redemptionMoveDir.y = 0.0f;
                m_redemptionMoveDir.Normalize();
            }
            m_redemptionReflectCount = 0;
            
            // FSM 파라미터 초기화
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("RedemptionFailed", false);
            }
        }
        else if (state == "Laststand")
        {
            // Laststand 상태 진입: 정지 및 타이머 초기화
            StopAllMovement();
            m_laststandTimer = 0.0f;
            
            // FSM 파라미터 초기화
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("PathFound", false);
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
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 공격
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::Attack(float deltaTime)
    {
        // 공격 애니메이션 타이머 업데이트
        m_attackAnimationTimer += deltaTime;

        if (m_fireTimer <= 0.0f)
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
                // - 에디터 설정: m_parabolicSpeed (속력), m_ownGravity (중력)
                // - 자동 계산: launchAngle (플레이어 거리 기반)
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
                    
                    // 발사각 자동 계산
                    float angleRad = 0.0f;
                    CalculateParabolicLaunchAngle(bulletStartPos, targetPos, angleRad);
                    
                    // BulletParams에 값 설정
                    m_bulletParams.speed = m_parabolicSpeed;
                    m_bulletParams.launchAngle = angleRad * 180.0f / 3.14159265f;  // 도(degree)로 변환
                    m_bulletParams.ownGravity = m_ownGravity;
                    
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
				// 공격 볌위에 들어오면 플레이어 주변에게 투사체를 난사 (8 ~ 15발)
                // ─────────────────────────────────────────────
                case MonsterTier::Red:
                {
                    int projectileCount = 8 + (rand() % 8);
                    constexpr float spreadAngle = DirectX::XMConvertToRadians(60.0f);

                    for (int i = 0; i < projectileCount; ++i)
                    {
                        float randomOffset = ((static_cast<float>(rand()) / RAND_MAX) * spreadAngle) - (spreadAngle * 0.5f);
                        DirectX::SimpleMath::Matrix rot = DirectX::SimpleMath::Matrix::CreateRotationY(randomOffset);
                        engine::Vector3 fireDir = engine::Vector3::TransformNormal(direction, rot);
                        fireDir.Normalize();

                        BulletParams individualParams = m_bulletParams;

                        float randomLifeMod = 0.1f + (static_cast<float>(rand()) / RAND_MAX) * 0.7f;
                        individualParams.lifetime *= randomLifeMod;

                        float randomSpeedMod = 0.6f + (static_cast<float>(rand()) / RAND_MAX) * 0.4f;
                        individualParams.speed *= randomSpeedMod;

                        m_bulletFactory->LinearFireMonster(firePosition, fireDir, individualParams);
                    }
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

                // 발사 쿨타임 리셋
                m_fireTimer = m_fireRate;
            }
        }
        else
        {
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

        // 최대 10회 시도
        constexpr int kMaxAttempts = 10;
        for (int attempt = 0; attempt < kMaxAttempts; ++attempt)
        {
            // 랜덤 각도: 기본 방향(180도) 기준 ±90도 범위
            // 즉, -90도 ~ +90도 오프셋 (플레이어 반대쪽 반구)
            float randomOffsetDeg = (static_cast<float>(rand()) / RAND_MAX) * 180.0f - 90.0f;
            float randomOffsetRad = DirectX::XMConvertToRadians(randomOffsetDeg);
            float finalAngle = baseAngle + randomOffsetRad;

            // 랜덤 거리: min ~ max 범위
            float randomDistance = m_fleeDistanceMin + 
                (static_cast<float>(rand()) / RAND_MAX) * (m_fleeDistanceMax - m_fleeDistanceMin);

            // 도망 목표 위치 계산 + 맵 경계로 클램핑
            engine::Vector3 targetPos;
            targetPos.x = std::clamp(myPos.x + randomDistance * std::cos(finalAngle), m_mapBoundXMin, m_mapBoundXMax);
            targetPos.y = myPos.y;
            targetPos.z = std::clamp(myPos.z + randomDistance * std::sin(finalAngle), m_mapBoundZMin, m_mapBoundZMax);

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
        
        // 120회 누적 실패 시 NoWayOut → Redemption 전이
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
    // Redemption 후 패스찾기 (60회 시도)
    // ═══════════════════════════════════════════════════════════════
    bool MonsterPointedType::TryFindPathAfterRedemption()
    {
        if (!m_targetPlayer || !m_pathfindingAgent) return false;

        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();

        engine::Vector3 awayFromPlayer = myPos - playerPos;
        awayFromPlayer.y = 0.0f;
        awayFromPlayer.Normalize();

        float baseAngle = std::atan2(awayFromPlayer.z, awayFromPlayer.x);

        // 60회 시도 (6프레임 분량)
        for (int attempt = 0; attempt < kRedemptionPathAttempts; ++attempt)
        {
            float randomOffsetDeg = (static_cast<float>(rand()) / RAND_MAX) * 180.0f - 90.0f;
            float randomOffsetRad = DirectX::XMConvertToRadians(randomOffsetDeg);
            float finalAngle = baseAngle + randomOffsetRad;

            float randomDistance = m_fleeDistanceMin + 
                (static_cast<float>(rand()) / RAND_MAX) * (m_fleeDistanceMax - m_fleeDistanceMin);

            // 도망 목표 위치 계산 + 맵 경계로 클램핑
            engine::Vector3 targetPos;
            targetPos.x = std::clamp(myPos.x + randomDistance * std::cos(finalAngle), m_mapBoundXMin, m_mapBoundXMax);
            targetPos.y = myPos.y;
            targetPos.z = std::clamp(myPos.z + randomDistance * std::sin(finalAngle), m_mapBoundZMin, m_mapBoundZMax);

            m_pathfindingAgent->RequestPathImmediate(targetPos);
            
            if (m_pathfindingAgent->HasPath())
            {
                m_fleeTargetPos = targetPos;
                m_hasFleeTarget = true;
                m_fleeAttemptCount = 0;  // 카운트 리셋
                m_isNoWayOut = false;
                return true;
            }
        }

        return false;
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백 (Redemption 반사용)
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::OnCollisionEnter(const engine::CollisionInfo& info)
    {
        // Redemption 상태에서만 반사 처리
        std::string currentState = GetCurrentState();
        if (currentState != "Redemption") return;

        if (!info.gameObject) return;

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
        
        // 반사 횟수 증가
        m_redemptionReflectCount++;
        
        // 2회 반사 후 패스찾기 시도
        if (m_redemptionReflectCount >= kRedemptionMaxReflects)
        {
            if (TryFindPathAfterRedemption())
            {
                // 패스 찾음 → Flee로 복귀
                if (m_logicFSM)
                {
                    m_logicFSM->SetParameter("NoWayOut", false);
                }
            }
            else
            {
                // 패스 못 찾음 → Laststand로 전이
                if (m_logicFSM)
                {
                    m_logicFSM->SetParameter("RedemptionFailed", true);
                }
            }
        }
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
        
        // ─────────────────────────────────────────────
        // 컴포넌트 검증 (에디터 화면에서도 체크)
        // 뾰족 타입은 StaticMesh 사용 - SkeletalAnimator/AnimFSM 불필요
        // ─────────────────────────────────────────────
        ImGui::Separator();
        ImGui::Text("=== Component Validation ===");
        
        // 에디터 모드를 위한 실시간 컴포넌트 검색 (같은 GameObject 내에서만 검색)
        engine::Rigidbody* rigidbody = m_rigidbody ? m_rigidbody : (GetGameObject() ? GetGameObject()->GetComponent<engine::Rigidbody>() : nullptr);
        engine::LogicFSM* logicFSM = m_logicFSM ? m_logicFSM : (GetGameObject() ? GetGameObject()->GetComponent<engine::LogicFSM>() : nullptr);
        BulletFactory* bulletFactory = m_bulletFactory ? m_bulletFactory : (GetGameObject() ? GetGameObject()->GetComponent<BulletFactory>() : nullptr);
        engine::PathfindingAgent* pathfindingAgent = m_pathfindingAgent ? m_pathfindingAgent : (GetGameObject() ? GetGameObject()->GetComponent<engine::PathfindingAgent>() : nullptr);
        
        // 전체 유효성 검사 (뾰족: Rigidbody, LogicFSM, BulletFactory, PathfindingAgent 필수)
        bool allValid = rigidbody && bulletFactory && logicFSM && pathfindingAgent;
        
        if (allValid)
        {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "[OK] All components are valid!");
        }
        else
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "[ERROR] Some components are missing!");
        }

        // 개별 컴포넌트 상태 표시
        ImGui::Indent();
        ImGui::Text("Rigidbody:         %s", rigidbody ? "[OK]" : "[MISSING]");
        if (!rigidbody) ImGui::SameLine(); if (!rigidbody) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");
        
        ImGui::Text("BulletFactory:     %s", bulletFactory ? "[OK]" : "[MISSING]");
        if (!bulletFactory) ImGui::SameLine(); if (!bulletFactory) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");
        
        ImGui::Text("LogicFSM:          %s", logicFSM ? "[OK]" : "[MISSING]");
        if (!logicFSM) ImGui::SameLine(); if (!logicFSM) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");
        
        ImGui::Text("PathfindingAgent:  %s", pathfindingAgent ? "[OK]" : "[MISSING]");
        if (!pathfindingAgent) ImGui::SameLine(); if (!pathfindingAgent) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");
        ImGui::Unindent();

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

        // 스탯
        ImGui::Separator();
        ImGui::Text("Stats:");
        ImGui::DragFloat("HP", &m_Hp, 0.1f, 1.0f, 10000.0f);
        ImGui::DragFloat("Attack Range", &m_AttackRange, 0.1f, 0.0f, 50.0f);
        ImGui::DragFloat("Detection Range", &m_detectionRange, 0.1f, 0.0f, 50.0f);

        // 설정
        ImGui::Separator();
        ImGui::Text("Settings:");
        ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Rotation Speed", &m_rotationSpeed, 0.1f, 0.0f, 10.0f);
        ImGui::DragFloat("Fire Rate (sec)", &m_fireRate, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Bullet Speed", &m_bulletSpeed, 0.1f, 0.1f, 100.0f);
        ImGui::DragFloat("Bullet Lifetime", &m_bulletLifetime, 0.1f, 0.5f, 10.0f);
        ImGui::DragFloat("Attack Duration", &m_attackAnimationDuration, 0.1f, 0.1f, 5.0f);

        // 뾰족 타입은 StaticMesh를 사용하므로 Animation Names 설정 불필요

        // ─────────────────────────────────────────────
        // 포물선 설정 (Green일 때만 표시 - 둔탁녹색과 동일)
        // ─────────────────────────────────────────────
        if (m_monsterTier == MonsterTier::Green)
        {
            ImGui::Separator();
            ImGui::Text("=== Parabolic Bullet Settings ===");
            
            ImGui::DragFloat("Parabolic Speed", &m_parabolicSpeed, 0.5f, 1.0f, 50.0f, "%.1f m/s");
            ImGui::DragFloat("Own Gravity", &m_ownGravity, 0.1f, 1.0f, 30.0f, "%.1f m/s^2");
            
            float maxRange = (m_parabolicSpeed * m_parabolicSpeed) / m_ownGravity;
            ImGui::Text("Max Range (at 45 deg): %.1f m", maxRange);
            
            if (maxRange < m_AttackRange)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), 
                    "WARNING: Max Range < Attack Range!");
            }
            else
            {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), 
                    "OK: Max Range >= Attack Range");
            }
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
            
            ImGui::Spacing();
            ImGui::Text("Map Bounds (Flee Target):");
            ImGui::DragFloat("X Min", &m_mapBoundXMin, 0.5f, -100.0f, 0.0f, "%.1f");
            ImGui::DragFloat("X Max", &m_mapBoundXMax, 0.5f, 0.0f, 100.0f, "%.1f");
            ImGui::DragFloat("Z Min", &m_mapBoundZMin, 0.5f, -100.0f, 0.0f, "%.1f");
            ImGui::DragFloat("Z Max", &m_mapBoundZMax, 0.5f, 0.0f, 100.0f, "%.1f");
            
            ImGui::Spacing();
            ImGui::Text("Redemption/Laststand:");
            ImGui::DragFloat("Redemption Speed Mult", &m_redemptionSpeedMultiplier, 0.1f, 0.5f, 3.0f, "%.1fx");
            ImGui::DragFloat("Laststand Retry Interval", &m_laststandRetryInterval, 0.5f, 1.0f, 10.0f, "%.1f sec");
            
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
            ImGui::Text("  Redemption Reflects: %d / %d", m_redemptionReflectCount, kRedemptionMaxReflects);
            ImGui::Text("  Laststand Timer: %.1f / %.1f", m_laststandTimer, m_laststandRetryInterval);
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
        if (pathfindingAgent)
        {
            ImGui::Separator();
            ImGui::Text("=== Pathfinding Info ===");
            ImGui::Text("Has Path: %s", pathfindingAgent->HasPath() ? "Yes" : "No");
            if (pathfindingAgent->HasPath())
            {
                ImGui::Text("Path Length: %d", static_cast<int>(pathfindingAgent->GetPath().size()));
                engine::Vector3 waypoint;
                if (pathfindingAgent->GetCurrentWaypoint(waypoint))
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
        
        // Redemption/Laststand 설정 (Purple용)
        j["RedemptionSpeedMultiplier"] = m_redemptionSpeedMultiplier;
        j["LaststandRetryInterval"] = m_laststandRetryInterval;
        
        // 맵 경계 (Purple용)
        j["MapBoundXMin"] = m_mapBoundXMin;
        j["MapBoundXMax"] = m_mapBoundXMax;
        j["MapBoundZMin"] = m_mapBoundZMin;
        j["MapBoundZMax"] = m_mapBoundZMax;
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
        
        // Redemption/Laststand 설정 (Purple용)
        m_redemptionSpeedMultiplier = j.value("RedemptionSpeedMultiplier", 1.5f);
        m_laststandRetryInterval = j.value("LaststandRetryInterval", 5.0f);
        
        // 맵 경계 (Purple용)
        m_mapBoundXMin = j.value("MapBoundXMin", -29.5f);
        m_mapBoundXMax = j.value("MapBoundXMax", 29.5f);
        m_mapBoundZMin = j.value("MapBoundZMin", -18.5f);
        m_mapBoundZMax = j.value("MapBoundZMax", 19.5f);
    }
}
