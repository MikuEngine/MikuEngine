#include "GamePCH.h"
#include "MonsterRoundRed.h"

#include "Script/CharacterScript/Common/BulletFactory.h"

#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Object/Component/Renderer/SkeletalMeshRenderer.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Physics/PhysicsSystem.h>
#include <Framework/System/SystemManager.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundRed::Awake()
    {
        MonsterRoundType::Awake();
        
        // Red 등급 고정
        m_monsterTier = MonsterTier::Red;
    }

    void MonsterRoundRed::Start()
    {
        MonsterRoundType::Start();

        // Red 등급 색상 설정 (빨간색)
        if (auto* meshRenderer = GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>())
        {
            meshRenderer->SetBaseColor(engine::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
        }
        
        // 원래 레이어 저장
        if (auto* collider = GetGameObject()->GetComponent<engine::Collider>())
        {
            m_originalLayer = collider->GetLayer();
        }
        
        // 착지점 디버그 체커 찾기
        if (auto* scene = engine::SceneManager::Get().GetScene())
        {
            m_landingChecker = scene->FindGameObject("LandingPointCheckDebugTrg");
        }
        
        // 초기 Y 위치 보정 (지면 + 오프셋)
        CorrectYPosition();
    }

    // ═══════════════════════════════════════════════════════════════
    // FSM 초기화 (Red 전용 - 점프 상태 추가)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundRed::InitializeFSM()
    {
        if (!m_logicFSM) return;

        // ─────────────────────────────────────────────
        // 상태 정의 (Fragile 없음)
        // ─────────────────────────────────────────────
        AddFSMState("Idle", true);              // 기본 상태
        AddFSMState("EngageJumpReady", false);  // 점프 준비 (착지점 탐색)
        AddFSMState("EngageJump", false);       // 점프 중
        AddFSMState("EngageStop", false);       // 공격 사거리 내 정지
        AddFSMState("EngageAttack", false);     // 공격
        AddFSMState("Dead", false);

        // ─────────────────────────────────────────────
        // StateMap 업데이트
        // ─────────────────────────────────────────────
        m_logicFSM->Initialize();

        // ─────────────────────────────────────────────
        // 파라미터 정의
        // ─────────────────────────────────────────────
        m_logicFSM->SetParameter("IdleTimerComplete", false);
        m_logicFSM->SetParameter("PlayerInRange", m_isPlayerInRange);
        m_logicFSM->SetParameter("JumpReady", false);          // 점프 준비 완료
        m_logicFSM->SetParameter("JumpComplete", false);       // 점프 착지 완료
        m_logicFSM->SetParameter("CanFire", m_canFire);
        m_logicFSM->SetParameter("AttackComplete", false);
        m_logicFSM->SetParameter("Die", m_isDead);

        // ─────────────────────────────────────────────
        // 전이 정의
        // ─────────────────────────────────────────────
        
        // Idle → EngageJumpReady (대기 완료)
        AddFSMTransition("Idle", "EngageJumpReady", "IdleTimerComplete", BoolTrue());
        
        // EngageJumpReady → EngageJump (점프 준비 완료)
        AddFSMTransition("EngageJumpReady", "EngageJump", "JumpReady", BoolTrue());
        
        // EngageJump → EngageJumpReady (착지 완료, 사거리 밖)
        AddFSMTransition("EngageJump", "EngageJumpReady", "JumpComplete", BoolTrue(), "PlayerInRange", BoolFalse());
        
        // EngageJump → EngageStop (착지 완료, 사거리 안, 쿨타임 중)
        AddFSMTransition("EngageJump", "EngageStop", "JumpComplete", BoolTrue(), "PlayerInRange", BoolTrue(), "CanFire", BoolFalse());
        
        // EngageJump → EngageAttack (착지 완료, 사거리 안, 공격 가능)
        AddFSMTransition("EngageJump", "EngageAttack", "JumpComplete", BoolTrue(), "PlayerInRange", BoolTrue(), "CanFire", BoolTrue());
        
        // EngageStop → EngageJumpReady (사거리 이탈)
        AddFSMTransition("EngageStop", "EngageJumpReady", "PlayerInRange", BoolFalse());
        
        // EngageStop → EngageAttack (공격 가능)
        AddFSMTransition("EngageStop", "EngageAttack", "CanFire", BoolTrue(), "PlayerInRange", BoolTrue());
        
        // EngageAttack → EngageJumpReady (공격 완료, 사거리 이탈)
        AddFSMTransition("EngageAttack", "EngageJumpReady", "AttackComplete", BoolTrue(), "PlayerInRange", BoolFalse());
        
        // EngageAttack → EngageStop (공격 완료, 사거리 안, 쿨타임)
        AddFSMTransition("EngageAttack", "EngageStop", "AttackComplete", BoolTrue(), "PlayerInRange", BoolTrue(), "CanFire", BoolFalse());
        
        // EngageAttack → EngageAttack (공격 완료, 사거리 안, 즉시 재공격)
        AddFSMTransition("EngageAttack", "EngageAttack", "AttackComplete", BoolTrue(), "PlayerInRange", BoolTrue(), "CanFire", BoolTrue());

        // Any → Dead (Round 타입은 Fragile 없이 바로 Dead로 전이)
        AddFSMTransition("Idle", "Dead", "Die", Trigger());
        AddFSMTransition("EngageJumpReady", "Dead", "Die", Trigger());
        AddFSMTransition("EngageJump", "Dead", "Die", Trigger());
        AddFSMTransition("EngageStop", "Dead", "Die", Trigger());
        AddFSMTransition("EngageAttack", "Dead", "Die", Trigger());

        // ─────────────────────────────────────────────
        // 초기 상태 설정
        // ─────────────────────────────────────────────
        m_logicFSM->InitializeCurrentState();
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 입력 처리
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundRed::ProcessInput()
    {
        if (!m_logicFSM) return;

        // 플레이어 찾기
        if (!m_targetPlayer)
        {
            FindPlayer();
        }

        // 플레이어와의 거리 체크
        m_isPlayerInRange = IsPlayerInRange();
        m_canFire = (m_fireTimer <= 0.0f);

        // FSM 파라미터 업데이트
        m_logicFSM->SetParameter("PlayerInRange", m_isPlayerInRange);
        m_logicFSM->SetParameter("CanFire", m_canFire);
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태 진입 콜백
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundRed::OnStateEntered(const std::string& state)
    {
        // 부모 호출
        MonsterRoundType::OnStateEntered(state);
        
        if (state == "EngageJumpReady")
        {
            // 점프 준비 초기화
            m_jumpPrepareTimer = 0.0f;
            m_hasValidLandingPos = false;
            
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("JumpReady", false);
            }
        }
        else if (state == "EngageJump")
        {
            // 점프 시작
            if (m_hasValidLandingPos)
            {
                StartJump(m_targetLandingPos);
            }
            
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("JumpComplete", false);
            }
        }
        else if (state == "EngageAttack")
        {
            // ─────────────────────────────────────────────
            // 공격점프 시스템 초기화
            // ─────────────────────────────────────────────
            m_fallAtkPhase = FallAttackPhase::Prepare;
            m_fallAtkTimer = 0.0f;
            m_fallAtkJumping = false;
            m_fallAtkLandDelayStarted = false;
            m_fallAtkStartPos = engine::Vector3::Zero;
            m_fallAtkTargetPos = engine::Vector3::Zero;
            
            // Environment 충돌 무시 (공격점프~착지 동안)
            if (auto* collider = GetGameObject()->GetComponent<engine::Collider>())
            {
                m_originalLayer = collider->GetLayer();
                collider->SetLayer(engine::PhysicsLayer::Index::JumpingEnemy);
            }
            
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("AttackComplete", false);
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 상태별 행동 - 비물리
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundRed::UpdateStateBasedBehavior(const std::string& state, float deltaTime)
    {
        // 발사 쿨타임 감소
        if (m_fireTimer > 0.0f)
        {
            m_fireTimer -= deltaTime;
        }

        if (state == "EngageJumpReady")
        {
            ExecuteEngageJumpReadyBehaviorNonPhysics(deltaTime);
        }
        else if (state == "EngageJump")
        {
            ExecuteEngageJumpBehaviorNonPhysics(deltaTime);
        }
        else if (state == "EngageStop")
        {
            // 정지 상태 (비물리에서는 특별한 처리 없음)
        }
        else if (state == "EngageAttack")
        {
            Attack(deltaTime);
        }
        else if (state == "Idle")
        {
            ExecuteIdleBehaviorNonPhysics();
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
    
    void MonsterRoundRed::ExecuteEngageJumpReadyBehaviorNonPhysics(float deltaTime)
    {
        m_jumpPrepareTimer += deltaTime;
        
        // 준비 시간 대기
        if (m_jumpPrepareTimer < m_jumpPrepareTime)
        {
            return;
        }
        
        // ─────────────────────────────────────────────
        // 착지점 탐색 (OverlapSphere - 즉시 완료!)
        // ─────────────────────────────────────────────
        if (!m_targetPlayer) return;
        
        engine::Vector3 dirToPlayer = CalculateDirectionToPlayer();
        float jumpDist = m_maxJumpStepDistance * 0.7f;
        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        engine::Vector3 idealLanding = myPos + dirToPlayer * jumpDist;
        
        // 착지점 검증
        engine::Vector3 validLanding;
        m_hasValidLandingPos = TryFindValidLandingPosition(
            idealLanding, 
            dirToPlayer, 
            validLanding
        );
        
        if (m_hasValidLandingPos)
        {
            m_targetLandingPos = validLanding;
            
            // 랜딩체커 시각화 (디버그)
            if (m_landingChecker && m_landingChecker->GetTransform())
            {
                m_landingChecker->GetTransform()->SetLocalPosition(m_targetLandingPos);
            }
            
            // EngageJump 전이
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("JumpReady", true);
            }
        }
        else
        {
            // 착지 불가 - 대기 상태 유지 (다음 프레임에 재시도)
            m_jumpPrepareTimer = 0.0f;
        }
    }
    
    void MonsterRoundRed::ExecuteEngageJumpBehaviorNonPhysics(float deltaTime)
    {
        if (!m_isJumping) return;
        
        // ─────────────────────────────────────────────
        // 1. 점프 후 0.05초 이후부터 Y 좌표 추적
        // ─────────────────────────────────────────────
        float elapsedTime = engine::Time::GetElapsedSeconds(m_jumpStartTime);
        if (elapsedTime < m_jumpCheckDelay)
        {
            return;  // 아직 체크 시작 전
        }
        
        // ─────────────────────────────────────────────
        // 2. Y <= 임계값이 되는 순간, 착지 신호 보냄
        // ─────────────────────────────────────────────
        float currentY = GetTransform()->GetWorldPosition().y;
        
        if (currentY <= m_landingYThreshold && !m_landingSignal)
        {
            m_landingSignal = true;  // FixedUpdate에서 처리
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 상태별 행동 - 물리
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundRed::UpdatePhysicsStateBasedBehavior(const std::string& state)
    {
        if (state == "EngageJumpReady")
        {
            ExecuteEngageJumpReadyBehaviorPhysics();
        }
        else if (state == "EngageJump")
        {
            ExecuteEngageJumpBehaviorPhysics();
            // 점프 중에는 Y 보정 안 함 (포물선 운동 중)
            return;
        }
        else if (state == "EngageStop")
        {
            StopAllMovement();
            RotateTowardsPlayer();
        }
        else if (state == "EngageAttack")
        {
            // ─────────────────────────────────────────────
            // 공격점프 물리 처리 (단계별)
            // ─────────────────────────────────────────────
            switch (m_fallAtkPhase)
            {
            case FallAttackPhase::Prepare:
                ExecuteFallAttackPreparePhysics();
                break;
            case FallAttackPhase::Jump:
                ExecuteFallAttackJumpPhysics();
                return;  // Y 보정 안 함
            case FallAttackPhase::Fall:
                ExecuteFallAttackFallPhysics();
                return;  // Y 보정 안 함
            case FallAttackPhase::Land:
                ExecuteFallAttackLandPhysics();
                break;
            default:
                StopAllMovement();
                break;
            }
        }
        else if (state == "Idle")
        {
            ExecuteIdleBehaviorPhysics();
        }
        else if (state == "Fragile")
        {
            ExecuteFragileBehaviorPhysics();
        }
        else if (state == "Dead")
        {
            ExecuteDeadBehaviorPhysics();
        }
        
        // ─────────────────────────────────────────────
        // 모든 상태에서 Y 위치 체크 (점프 중 제외)
        // ─────────────────────────────────────────────
        CorrectYPosition();
    }
    
    void MonsterRoundRed::ExecuteEngageJumpReadyBehaviorPhysics()
    {
        // 준비 중에는 정지
        StopAllMovement();
        
        // 플레이어 방향 바라보기
        RotateTowardsPlayer();
        
        // Y 위치 유지 (지면 + 오프셋)
        CorrectYPosition();
    }
    
    void MonsterRoundRed::ExecuteEngageJumpBehaviorPhysics()
    {
        if (!m_isJumping || !m_rigidbody) return;
        
        // ─────────────────────────────────────────────
        // 중력 적용 (착지 신호 여부와 무관하게 계속 적용)
        // ─────────────────────────────────────────────
        engine::Vector3 gravityForce(0.0f, -m_ownGravity, 0.0f);
        m_rigidbody->AddForce(gravityForce, engine::ForceMode::Acceleration);
        
        // ─────────────────────────────────────────────
        // 착지 신호 받으면 Y 위치 체크
        // ─────────────────────────────────────────────
        if (m_landingSignal)
        {
            engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
            float targetY = m_groundY + m_groundYOffset;
            
            // ─────────────────────────────────────────────
            // Y 좌표가 목표 높이에 도달하면 착지 판정
            // ─────────────────────────────────────────────
            if (currentPos.y <= targetY)
            {
                // 정확한 위치로 고정
                currentPos.y = targetY;
                GetTransform()->SetLocalPosition(currentPos);
                
                // 착지 완료
                OnLanding();
                
                if (m_logicFSM)
                {
                    m_logicFSM->SetParameter("JumpComplete", true);
                }
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 착지점 검증 (OverlapSphere)
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundRed::CheckLandingPosition(const engine::Vector3& position)
    {
        engine::PhysicsSystem& physics = engine::SystemManager::Get().GetPhysicsSystem();
        
        std::vector<engine::Collider*> overlaps;
        bool hasOverlap = physics.OverlapSphere(
            position,
            m_landingCheckRadius,
            overlaps,
            (1u << engine::PhysicsLayer::Index::Environment) | (1u << engine::PhysicsLayer::Index::Boss)  // Environment, Boss 체크
        );
        
        // Environment/Boss와 겹치지 않으면 착지 가능
        return !hasOverlap;
    }
    
    bool MonsterRoundRed::TryFindValidLandingPosition(
        const engine::Vector3& idealPos,
        const engine::Vector3& moveDirection,
        engine::Vector3& outLandingPos)
    {
        // ─────────────────────────────────────────────
        // 1단계: 이상적인 위치 (0.7배)
        // ─────────────────────────────────────────────
        if (CheckLandingPosition(idealPos))
        {
            outLandingPos = idealPos;
            return true;
        }
        
        // ─────────────────────────────────────────────
        // 2단계: 원형 순회 (12방향, 0.3배 거리)
        // ─────────────────────────────────────────────
        float searchRadius = m_maxJumpStepDistance * 0.3f;
        constexpr int kSampleCount = 12;
        constexpr float kAngleStep = DirectX::XM_2PI / kSampleCount;  // 30도
        
        for (int i = 0; i < kSampleCount; ++i)
        {
            float angle = kAngleStep * i;
            
            engine::Vector3 offset;
            offset.x = searchRadius * std::cos(angle);
            offset.y = 0.0f;
            offset.z = searchRadius * std::sin(angle);
            
            engine::Vector3 candidatePos = idealPos + offset;
            
            if (CheckLandingPosition(candidatePos))
            {
                outLandingPos = candidatePos;
                return true;
            }
        }
        
        // ─────────────────────────────────────────────
        // 3단계: 밀어내기 (0.5m씩, 최대 10m)
        // ─────────────────────────────────────────────
        engine::Vector3 pushPos = idealPos;
        float pushStep = 0.5f;
        int maxPushAttempts = 20;
        
        for (int i = 0; i < maxPushAttempts; ++i)
        {
            pushPos += moveDirection * pushStep;
            
            if (CheckLandingPosition(pushPos))
            {
                outLandingPos = pushPos;
                return true;
            }
        }
        
        // 모든 시도 실패
        outLandingPos = idealPos;
        return false;
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 점프 시작/종료
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundRed::StartJump(const engine::Vector3& landingPos)
    {
        m_isJumping = true;
        m_landingSignal = false;
        m_jumpStartTime = engine::Time::GetTimestamp();
        
        // ─────────────────────────────────────────────
        // Environment 충돌 비활성화 (JumpingEnemy 레이어로 변경)
        // ─────────────────────────────────────────────
        auto* collider = GetGameObject()->GetComponent<engine::Collider>();
        if (collider)
        {
            m_originalLayer = collider->GetLayer();
            collider->SetLayer(engine::PhysicsLayer::Index::JumpingEnemy);
        }
        
        // ─────────────────────────────────────────────
        // 포물선 초속도 계산 (m_launchAngle 사용)
        // ─────────────────────────────────────────────
        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        
        // XZ 평면 거리
        float horizontalDist = engine::Vector3::Distance(
            engine::Vector3(myPos.x, 0, myPos.z),
            engine::Vector3(landingPos.x, 0, landingPos.z)
        );
        
        // 각도를 라디안으로 변환
        constexpr float kDegToRad = 3.14159265f / 180.0f;
        float angleRad = m_launchAngle * kDegToRad;
        
        // 포물선 공식: v = sqrt(d * g / sin(2θ))
        float sin2Angle = std::sin(2.0f * angleRad);
        if (sin2Angle < 0.0001f) sin2Angle = 0.0001f;  // 0으로 나누기 방지
        
        float jumpSpeed = std::sqrt((horizontalDist * m_ownGravity) / sin2Angle);
        
        // 방향 계산
        engine::Vector3 direction = landingPos - myPos;
        direction.y = 0;
        if (direction.LengthSquared() > 0.0001f)
        {
            direction.Normalize();
        }
        else
        {
            direction = engine::Vector3::UnitX;
        }
        
        // 초기 속도 설정 (m_launchAngle)
        float cosAngle = std::cos(angleRad);
        float sinAngle = std::sin(angleRad);
        
        engine::Vector3 velocity = direction * (jumpSpeed * cosAngle);
        velocity.y = jumpSpeed * sinAngle;
        
        // Rigidbody에 속도 설정
        if (m_rigidbody)
        {
            m_rigidbody->SetLinearVelocity(velocity);
        }
    }
    
    void MonsterRoundRed::OnLanding()
    {
        m_isJumping = false;
        m_landingSignal = false;
        
        // ─────────────────────────────────────────────
        // 레이어 복원 (Environment 충돌 재활성화)
        // ─────────────────────────────────────────────
        auto* collider = GetGameObject()->GetComponent<engine::Collider>();
        if (collider)
        {
            collider->SetLayer(m_originalLayer);
        }
        
        // 착지 시 속도 감쇠 (부드러운 착지)
        if (m_rigidbody)
        {
            engine::Vector3 vel = m_rigidbody->GetLinearVelocity();
            vel.y = 0.0f;  // Y 속도 제거
            vel *= 0.5f;   // 수평 속도 감쇠
            m_rigidbody->SetLinearVelocity(vel);
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // Y 위치 보정 (지면 + 오프셋 유지)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundRed::CorrectYPosition()
    {
        float targetY = m_groundY + m_groundYOffset;
        engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
        
        // ─────────────────────────────────────────────
        // 긴급 상황: Y <= 0 (지면 아래로 가라앉음)
        // ─────────────────────────────────────────────
        if (currentPos.y <= 0.0f)
        {
            currentPos.y = targetY;
            
            // ForceSetPosition으로 강제 복구 (속도도 제거)
            if (m_rigidbody)
            {
                m_rigidbody->ForceSetPosition(currentPos, true);
            }
            else
            {
                GetTransform()->SetLocalPosition(currentPos);
            }
            
            return;  // 긴급 복구 완료
        }
        
        // ─────────────────────────────────────────────
        // 일반 상황: 목표 Y와 차이가 있으면 부드럽게 보정
        // ─────────────────────────────────────────────
        if (std::abs(currentPos.y - targetY) > m_landingThreshold)
        {
            currentPos.y = targetY;
            GetTransform()->SetLocalPosition(currentPos);
            
            // Y 속도도 제거 (떠있는 상태 유지)
            if (m_rigidbody)
            {
                engine::Vector3 vel = m_rigidbody->GetLinearVelocity();
                vel.y = 0.0f;
                m_rigidbody->SetLinearVelocity(vel);
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 총알 초기화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundRed::InitializeBullet()
    {
        // Red: 착지 시 4방향 Curved 발사 (angularSpeed=0, 직선)
        m_bulletParams.type = BulletType::Curve;
        m_bulletParams.speed = m_bulletSpeed;
        m_bulletParams.lifetime = m_bulletLifetime;
        m_bulletParams.damage = m_attackDamage;
        m_bulletParams.angularSpeed = 0.0f;                  // 회전 없음 (직선)
        m_bulletParams.radiusGrowthRate = m_bulletSpeed;     // 반지름 증가율 (m/s)
        m_bulletParams.scale = m_bulletScale;
        m_bulletParams.explosionRadius = m_explosionRadius;  // 부모 값 복사 (미사용이지만 일관성 유지)
    }

    // ═══════════════════════════════════════════════════════════════
    // 공격 (Red 전용 - 공격점프 시스템)
    // 
    // 로직:
    //   1. Prepare: 대기 시간 후 플레이어 좌표 캡처
    //   2. Jump: 상방 각도로 점프, XZ 도달 시 Velocity 0
    //   3. Fall: 수직 낙하 (OwnFallAtkGravity)
    //   4. Land: 착지 시 4방향 총알 발사, 사거리 체크 후 반복/전이
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundRed::Attack(float deltaTime)
    {
        switch (m_fallAtkPhase)
        {
        case FallAttackPhase::Prepare:
            ExecuteFallAttackPrepare(deltaTime);
            break;
            
        case FallAttackPhase::Jump:
            ExecuteFallAttackJump(deltaTime);
            break;
            
        case FallAttackPhase::Fall:
            ExecuteFallAttackFall(deltaTime);
            break;
            
        case FallAttackPhase::Land:
            ExecuteFallAttackLand(deltaTime);
            break;
            
        case FallAttackPhase::None:
        default:
            // 공격 상태가 아님 - 초기화 필요
            m_fallAtkPhase = FallAttackPhase::Prepare;
            m_fallAtkTimer = 0.0f;
            break;
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 공격 단계별 비물리 처리 (Update에서 호출)
    // ═══════════════════════════════════════════════════════════════
    
    void MonsterRoundRed::ExecuteFallAttackPrepare(float deltaTime)
    {
        // 준비 타이머 증가
        m_fallAtkTimer += deltaTime;
        
        // 대기 시간 완료
        if (m_fallAtkTimer >= m_fallAtkPrepareTime)
        {
            // 플레이어 좌표 캡처
            CaptureAttackLandingPosition();
            
            // 공격점프 시작
            StartFallAttackJump();
            
            // 다음 단계로 전이
            m_fallAtkPhase = FallAttackPhase::Jump;
        }
    }
    
    void MonsterRoundRed::ExecuteFallAttackJump(float deltaTime)
    {
        // XZ 도달 판정
        if (HasReachedAttackLandingXZ())
        {
            // Velocity 0으로 설정 (공중에서 정지)
            if (m_rigidbody)
            {
                m_rigidbody->SetLinearVelocity(engine::Vector3::Zero);
            }
            
            // 낙하 시작 Y 좌표 저장 (선형 보간 계산용)
            m_fallAtkStartY = GetTransform()->GetWorldPosition().y;
            
            // 다음 단계로 전이
            m_fallAtkPhase = FallAttackPhase::Fall;
        }
    }
    
    void MonsterRoundRed::ExecuteFallAttackFall(float deltaTime)
    {
        // 착지 판정
        if (CheckFallAttackLanding())
        {
            // 착지 처리 (총알 발사 포함)
            OnFallAttackLanding();
            
            // 다음 단계로 전이
            m_fallAtkPhase = FallAttackPhase::Land;
        }
    }
    
    void MonsterRoundRed::ExecuteFallAttackLand(float deltaTime)
    {
        // ─────────────────────────────────────────────
        // 착지 후딜레이 처리
        // ─────────────────────────────────────────────
        if (!m_fallAtkLandDelayStarted)
        {
            // 후딜레이 시작
            m_fallAtkLandDelayStarted = true;
            m_fallAtkTimer = 0.0f;
        }
        
        m_fallAtkTimer += deltaTime;
        
        // 후딜레이 완료 전이면 대기
        if (m_fallAtkTimer < m_fallAtkLandDelay)
        {
            return;
        }
        
        // ─────────────────────────────────────────────
        // 후딜레이 완료 → 사거리 체크
        // ─────────────────────────────────────────────
        m_isPlayerInRange = IsPlayerInRange();
        m_fallAtkLandDelayStarted = false;  // 플래그 리셋
        
        if (m_isPlayerInRange)
        {
            // 사거리 내: 다시 Prepare로 (재공격)
            m_fallAtkPhase = FallAttackPhase::Prepare;
            m_fallAtkTimer = 0.0f;
        }
        else
        {
            // 사거리 밖: 공격 완료 → FSM이 다른 상태로 전이
            m_fallAtkPhase = FallAttackPhase::None;
            m_fallAtkJumping = false;
            
            // 레이어 복원 (Environment 충돌 재활성화)
            if (auto* collider = GetGameObject()->GetComponent<engine::Collider>())
            {
                collider->SetLayer(m_originalLayer);
            }
            
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("AttackComplete", true);
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 행동 제한
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundRed::CanMove() const
    {
        std::string state = GetCurrentState();
        // 점프 중에는 이동 불가 (물리 법칙만 적용)
        return state == "EngageJumpReady";
    }
    
    bool MonsterRoundRed::CanAttack() const
    {
        std::string state = GetCurrentState();
        return state == "EngageAttack" && !m_isFragile && !m_isDead;
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 공격 단계별 물리 처리 (FixedUpdate에서 호출)
    // ═══════════════════════════════════════════════════════════════
    
    void MonsterRoundRed::ExecuteFallAttackPreparePhysics()
    {
        // 준비 중: 정지 + 플레이어 방향 회전
        StopAllMovement();
        RotateTowardsPlayer();
    }
    
    void MonsterRoundRed::ExecuteFallAttackJumpPhysics()
    {
        // 공격점프 중: 의사중력 적용 (포물선 운동)
        if (m_rigidbody)
        {
            constexpr float kFallAtkJumpGravity = 40.0f;  // 공격점프 전용 중력 (고정)
            engine::Vector3 gravity(0.0f, -kFallAtkJumpGravity, 0.0f);
            m_rigidbody->AddForce(gravity, engine::ForceMode::Acceleration);
        }
    }
    
    void MonsterRoundRed::ExecuteFallAttackFallPhysics()
    {
        if (!m_rigidbody) return;
        
        // ─────────────────────────────────────────────
        // 높이 기반 선형 보간 낙하 속도
        // ─────────────────────────────────────────────
        float currentY = GetTransform()->GetWorldPosition().y;
        float targetY = m_groundY + m_groundYOffset;
        
        // 진행률 계산 (0 = 시작, 1 = 착지)
        float totalHeight = m_fallAtkStartY - targetY;
        float t = 0.0f;
        
        if (totalHeight > 0.001f)
        {
            t = (m_fallAtkStartY - currentY) / totalHeight;
            t = std::clamp(t, 0.0f, 1.0f);
        }
        
        // 선형 보간: 초기 속도 → 종점 속도
        float fallSpeed = m_fallAtkInitialFallSpeed + 
            (m_fallAtkTerminalFallSpeed - m_fallAtkInitialFallSpeed) * t;
        
        // 하방 속도 설정 (직접 Velocity 지정)
        m_rigidbody->SetLinearVelocity(engine::Vector3(0.0f, -fallSpeed, 0.0f));
    }
    
    void MonsterRoundRed::ExecuteFallAttackLandPhysics()
    {
        // 착지 후: 정지
        StopAllMovement();
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 공격점프 헬퍼 함수
    // ═══════════════════════════════════════════════════════════════
    
    void MonsterRoundRed::CaptureAttackLandingPosition()
    {
        if (!m_targetPlayer) return;
        
        // 플레이어 좌표 캡처 (XZ만 사용, Y는 착지 판정에서 m_groundY + m_groundYOffset)
        engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();
        m_fallAtkTargetPos = playerPos;
        
        // ─────────────────────────────────────────────
        // 플레이어 이동 감지 → 예측 착지점 계산
        // ─────────────────────────────────────────────
        if (auto* playerRigidbody = m_targetPlayer->GetGameObject()->GetComponent<engine::Rigidbody>())
        {
            engine::Vector3 playerVel = playerRigidbody->GetLinearVelocity();
            playerVel.y = 0.0f;  // XZ 평면만 고려
            
            float speed = std::sqrt(playerVel.x * playerVel.x + playerVel.z * playerVel.z);
            
            if (speed >= m_fallAtkMoveThreshold)
            {
                // 플레이어 이동 중 → 이동 방향으로 m_fallAtkPredictOffset만큼 앞
                engine::Vector3 moveDir = playerVel;
                moveDir.Normalize();
                m_fallAtkTargetPos.x += moveDir.x * m_fallAtkPredictOffset;
                m_fallAtkTargetPos.z += moveDir.z * m_fallAtkPredictOffset;
            }
            // 플레이어 정지 중 → 현재 위치 그대로 사용
        }
        
        m_fallAtkTargetPos.y = m_groundY + m_groundYOffset;  // 착지 높이로 설정
        
        // 시작 위치 저장 (XZ 도달 판정용)
        m_fallAtkStartPos = GetTransform()->GetWorldPosition();
        
        // ─────────────────────────────────────────────
        // 착지점 마커 이동 (디버그 시각화)
        // ─────────────────────────────────────────────
        if (m_landingChecker && m_landingChecker->GetTransform())
        {
            m_landingChecker->GetTransform()->SetLocalPosition(m_fallAtkTargetPos);
        }
    }
    
    void MonsterRoundRed::StartFallAttackJump()
    {
        if (!m_rigidbody) return;
        
        m_fallAtkJumping = true;
        
        // ─────────────────────────────────────────────
        // 공격점프 초속도 계산
        // ─────────────────────────────────────────────
        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        
        // XZ 방향 계산 (착지점 방향)
        engine::Vector3 direction = m_fallAtkTargetPos - myPos;
        direction.y = 0.0f;
        if (direction.LengthSquared() > 0.0001f)
        {
            direction.Normalize();
        }
        else
        {
            direction = engine::Vector3::UnitX;
        }
        
        // 각도를 라디안으로 변환
        constexpr float kDegToRad = 3.14159265f / 180.0f;
        float angleRad = m_fallAtkLaunchAngle * kDegToRad;
        
        // 초기 속도 설정 (상방 각도)
        float cosAngle = std::cos(angleRad);
        float sinAngle = std::sin(angleRad);
        
        engine::Vector3 velocity = direction * (m_fallAtkJumpSpeed * cosAngle);
        velocity.y = m_fallAtkJumpSpeed * sinAngle;
        
        // Rigidbody에 속도 설정
        m_rigidbody->SetLinearVelocity(velocity);
    }
    
    bool MonsterRoundRed::HasReachedAttackLandingXZ() const
    {
        engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
        
        // XZ 평면에서 계산
        float startX = m_fallAtkStartPos.x;
        float startZ = m_fallAtkStartPos.z;
        float targetX = m_fallAtkTargetPos.x;
        float targetZ = m_fallAtkTargetPos.z;
        float currentX = currentPos.x;
        float currentZ = currentPos.z;
        
        // 시작점 → 착지점 방향 벡터
        float toTargetX = targetX - startX;
        float toTargetZ = targetZ - startZ;
        float totalDist = std::sqrt(toTargetX * toTargetX + toTargetZ * toTargetZ);
        
        if (totalDist < 0.001f) return true;  // 거의 같은 위치
        
        // 정규화
        float dirX = toTargetX / totalDist;
        float dirZ = toTargetZ / totalDist;
        
        // 시작점 → 현재 위치 벡터
        float toCurrentX = currentX - startX;
        float toCurrentZ = currentZ - startZ;
        
        // 착지점 기준 - threshold 위치까지의 거리
        float thresholdDist = totalDist - m_fallAtkLandingThreshold;
        
        // 현재 위치의 투영 거리 (내적)
        float projectedDist = toCurrentX * dirX + toCurrentZ * dirZ;
        
        // threshold 거리를 넘었는지 판정
        return projectedDist >= thresholdDist;
    }
    
    bool MonsterRoundRed::CheckFallAttackLanding() const
    {
        float targetY = m_groundY + m_groundYOffset;
        float currentY = GetTransform()->GetWorldPosition().y;
        
        return currentY <= targetY;
    }
    
    void MonsterRoundRed::OnFallAttackLanding()
    {
        LOG_INFO("[MonsterRoundRed] OnFallAttackLanding called!");
        
        // ─────────────────────────────────────────────
        // Y 위치 보정
        // ─────────────────────────────────────────────
        engine::Vector3 pos = GetTransform()->GetWorldPosition();
        pos.y = m_groundY + m_groundYOffset;
        GetTransform()->SetLocalPosition(pos);
        
        // ─────────────────────────────────────────────
        // 속도 0
        // ─────────────────────────────────────────────
        if (m_rigidbody)
        {
            m_rigidbody->SetLinearVelocity(engine::Vector3::Zero);
        }
        
        m_fallAtkJumping = false;
        
        // ─────────────────────────────────────────────
        // 4방향 총알 발사
        // ─────────────────────────────────────────────
        FireFallAttackBullets();
    }
    
    void MonsterRoundRed::FireFallAttackBullets()
    {
        if (!m_bulletFactory)
        {
            LOG_ERROR("[MonsterRoundRed] FireFallAttackBullets: m_bulletFactory is NULL!");
            return;
        }
        
        LOG_INFO("[MonsterRoundRed] FireFallAttackBullets: Firing 4-way bullets!");
        
        engine::Vector3 firePos = GetTransform()->GetWorldPosition();
        
        // ─────────────────────────────────────────────
        // CurvedFireMonster 호출
        // - angularSpeed = 0: 나선 회전 없음 (직선)
        // - radiusGrowthRate = bulletSpeed: 4방향 직선 발사
        // - m_bulletParams에 설정된 모든 값 전달 (speed, lifetime, damage 등)
        // ─────────────────────────────────────────────
        
        // InitializeBullet()에서 설정된 값 확인
        // - angularSpeed = 0.0 (회전 없음)
        // - radiusGrowthRate = m_bulletSpeed (직선 속도)
        m_bulletFactory->CurvedFireMonster(
            firePos,
            m_bulletParams.angularSpeed,          // 0.0 (회전 없음)
            m_bulletParams.radiusGrowthRate,      // m_bulletSpeed (직선 발사)
            m_bulletParams                        // 모든 파라미터 전달
        );

        // 사운드
        m_remainShotSoundCount = 3;
        m_shotSoundTimer = 0.0f;
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundRed::OnGui()
    {
        ImGui::Text("=== MonsterRoundRed ===");
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Tier: Red");
        
        // 부모 클래스 OnGui 호출
        MonsterRoundType::OnGui();
        
        // Red 전용 설정
        ImGui::Separator();
        ImGui::Text("=== Jump Settings ===");
        ImGui::DragFloat("Max Jump Distance", &m_maxJumpStepDistance, 0.5f, 1.0f, 50.0f, "%.1f m");
        ImGui::DragFloat("Jump Prepare Time", &m_jumpPrepareTime, 0.05f, 0.1f, 2.0f, "%.2f sec");
        ImGui::DragFloat("Launch Angle", &m_launchAngle, 1.0f, 15.0f, 75.0f, "%.1f deg");
        ImGui::DragFloat("Own Gravity", &m_ownGravity, 0.1f, 1.0f, 30.0f, "%.1f m/s^2");
        ImGui::DragFloat("Landing Check Radius", &m_landingCheckRadius, 0.1f, 0.1f, 5.0f, "%.1f m");
        
        ImGui::Separator();
        ImGui::Text("=== Landing Detection Settings ===");
        ImGui::DragFloat("Ground Y", &m_groundY, 0.1f, -10.0f, 10.0f, "%.2f m");
        ImGui::DragFloat("Ground Y Offset", &m_groundYOffset, 0.1f, 0.0f, 5.0f, "%.2f m");
        ImGui::DragFloat("Jump Check Delay", &m_jumpCheckDelay, 0.01f, 0.0f, 1.0f, "%.3f sec");
        ImGui::DragFloat("Landing Y Threshold", &m_landingYThreshold, 0.1f, 0.0f, 5.0f, "%.2f m");
        ImGui::DragFloat("Landing Threshold", &m_landingThreshold, 0.001f, 0.0f, 0.1f, "%.4f m");
        
        // 최대 사거리 표시 (45도 포물선, 이상적인 경우)
        constexpr float kDegToRad = 3.14159265f / 180.0f;
        float angleRad = m_launchAngle * kDegToRad;
        float sin2Angle = std::sin(2.0f * angleRad);
        float theoreticalMaxRange = 0.0f;
        if (sin2Angle > 0.0001f)
        {
            float maxSpeed = std::sqrt((m_maxJumpStepDistance * m_ownGravity) / sin2Angle);
            theoreticalMaxRange = (maxSpeed * maxSpeed * sin2Angle) / m_ownGravity;
        }
        ImGui::Text("Theoretical Max Range: %.1f m (no damping)", theoreticalMaxRange);
        
        // ─────────────────────────────────────────────
        // 공격점프 설정
        // ─────────────────────────────────────────────
        ImGui::Separator();
        ImGui::Text("=== Fall Attack Settings ===");
        ImGui::DragFloat("Prepare Time", &m_fallAtkPrepareTime, 0.05f, 0.0f, 3.0f, "%.2f sec");
        ImGui::DragFloat("Launch Angle##FallAtk", &m_fallAtkLaunchAngle, 1.0f, 45.0f, 85.0f, "%.1f deg");
        ImGui::DragFloat("Jump Speed##FallAtk", &m_fallAtkJumpSpeed, 1.0f, 10.0f, 100.0f, "%.1f m/s");
        ImGui::DragFloat("XZ Landing Threshold", &m_fallAtkLandingThreshold, 0.01f, 0.01f, 2.0f, "%.2f m");
        
        ImGui::Separator();
        ImGui::Text("=== Fall Attack Prediction ===");
        ImGui::DragFloat("Predict Offset", &m_fallAtkPredictOffset, 0.1f, 0.0f, 10.0f, "%.1f m");
        ImGui::DragFloat("Move Threshold", &m_fallAtkMoveThreshold, 0.1f, 0.1f, 5.0f, "%.1f m/s");
        
        ImGui::Separator();
        ImGui::Text("=== Fall Speed (Height-based Lerp) ===");
        ImGui::DragFloat("Initial Fall Speed", &m_fallAtkInitialFallSpeed, 1.0f, 1.0f, 100.0f, "%.1f m/s");
        ImGui::DragFloat("Terminal Fall Speed", &m_fallAtkTerminalFallSpeed, 1.0f, 1.0f, 150.0f, "%.1f m/s");
        
        ImGui::Separator();
        ImGui::Text("=== Fall Attack Timing ===");
        ImGui::DragFloat("Land Delay", &m_fallAtkLandDelay, 0.1f, 0.0f, 5.0f, "%.1f sec");
        
        // ─────────────────────────────────────────────
        // 공격점프 사거리 (이동 점프와 독립)
        // ─────────────────────────────────────────────
        ImGui::DragFloat("Attack Range (Fall Atk)", &m_AttackRange, 0.5f, 1.0f, 50.0f, "%.1f m");
        
        // 공격점프 런타임 정보
        ImGui::Separator();
        ImGui::Text("=== Fall Attack Runtime ===");
        
        const char* phaseNames[] = { "None", "Prepare", "Jump", "Fall", "Land" };
        int phaseIndex = static_cast<int>(m_fallAtkPhase);
        if (phaseIndex >= 0 && phaseIndex < 5)
        {
            ImGui::Text("Phase: %s", phaseNames[phaseIndex]);
        }
        else
        {
            ImGui::Text("Phase: Unknown (%d)", phaseIndex);
        }
        
        ImGui::Text("Prepare Timer: %.2f / %.2f", m_fallAtkTimer, m_fallAtkPrepareTime);
        ImGui::Text("Is Fall Attack Jumping: %s", m_fallAtkJumping ? "Yes" : "No");
        
        if (m_fallAtkTargetPos != engine::Vector3::Zero)
        {
            ImGui::Text("Target Pos: (%.1f, %.1f, %.1f)", 
                m_fallAtkTargetPos.x, m_fallAtkTargetPos.y, m_fallAtkTargetPos.z);
        }
        
        // 런타임 정보
        ImGui::Separator();
        ImGui::Text("=== Jump Runtime ===");
        ImGui::Text("Is Jumping: %s", m_isJumping ? "Yes" : "No");
        ImGui::Text("Landing Signal: %s", m_landingSignal ? "YES" : "No");
        ImGui::Text("Has Valid Landing: %s", m_hasValidLandingPos ? "Yes" : "No");
        
        if (m_hasValidLandingPos)
        {
            ImGui::Text("Target Landing: (%.1f, %.1f, %.1f)", 
                m_targetLandingPos.x, m_targetLandingPos.y, m_targetLandingPos.z);
            
            engine::Vector3 myPos = GetTransform()->GetWorldPosition();
            float distToLanding = engine::Vector3::Distance(myPos, m_targetLandingPos);
            ImGui::Text("Distance to Landing: %.1f m", distToLanding);
        }
        
        ImGui::Text("Jump Prepare Timer: %.2f / %.2f", m_jumpPrepareTimer, m_jumpPrepareTime);
        
        if (m_isJumping)
        {
            float elapsedTime = engine::Time::GetElapsedSeconds(m_jumpStartTime);
            ImGui::Text("Jump Elapsed Time: %.3f sec", elapsedTime);
        }
        
        engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
        ImGui::Text("Current Y: %.4f m", currentPos.y);
        ImGui::Text("Distance to Ground: %.4f m", std::abs(currentPos.y - m_groundY));
        
        if (m_rigidbody)
        {
            engine::Vector3 vel = m_rigidbody->GetLinearVelocity();
            ImGui::Text("Velocity: (%.1f, %.1f, %.1f)", vel.x, vel.y, vel.z);
            ImGui::Text("Y Velocity: %.2f m/s", vel.y);
        }
    }

    void MonsterRoundRed::Save(engine::json& j) const
    {
        MonsterRoundType::Save(j);
        
        // Red 전용 데이터 저장
        j["MaxJumpStepDistance"] = m_maxJumpStepDistance;
        j["JumpPrepareTime"] = m_jumpPrepareTime;
        j["LaunchAngle"] = m_launchAngle;
        j["OwnGravity"] = m_ownGravity;
        j["LandingCheckRadius"] = m_landingCheckRadius;
        
        // 착지 감지 설정
        j["GroundY"] = m_groundY;
        j["GroundYOffset"] = m_groundYOffset;
        j["JumpCheckDelay"] = m_jumpCheckDelay;
        j["LandingYThreshold"] = m_landingYThreshold;
        j["LandingThreshold"] = m_landingThreshold;
        
        // 공격점프 설정
        j["FallAtkPrepareTime"] = m_fallAtkPrepareTime;
        j["FallAtkLaunchAngle"] = m_fallAtkLaunchAngle;
        j["FallAtkJumpSpeed"] = m_fallAtkJumpSpeed;
        j["FallAtkLandingThreshold"] = m_fallAtkLandingThreshold;
        j["FallAtkPredictOffset"] = m_fallAtkPredictOffset;
        j["FallAtkMoveThreshold"] = m_fallAtkMoveThreshold;
        j["FallAtkInitialFallSpeed"] = m_fallAtkInitialFallSpeed;
        j["FallAtkTerminalFallSpeed"] = m_fallAtkTerminalFallSpeed;
        j["FallAtkLandDelay"] = m_fallAtkLandDelay;
        // AttackRange는 부모 클래스에서 저장됨 (공격점프 사거리로 사용)
    }

    void MonsterRoundRed::Load(const engine::json& j)
    {
        MonsterRoundType::Load(j);
        
        // Red 전용 데이터 로드
        m_maxJumpStepDistance = j.value("MaxJumpStepDistance", 10.0f);
        m_jumpPrepareTime = j.value("JumpPrepareTime", 0.5f);
        m_launchAngle = j.value("LaunchAngle", 45.0f);
        m_ownGravity = j.value("OwnGravity", 9.81f);
        m_landingCheckRadius = j.value("LandingCheckRadius", 0.7f);
        
        // 착지 감지 설정
        m_groundY = j.value("GroundY", 0.0f);
        m_groundYOffset = j.value("GroundYOffset", 1.5f);
        m_jumpCheckDelay = j.value("JumpCheckDelay", 0.05f);
        m_landingYThreshold = j.value("LandingYThreshold", 1.7f);
        m_landingThreshold = j.value("LandingThreshold", 0.005f);
        
        // 공격점프 설정
        m_fallAtkPrepareTime = j.value("FallAtkPrepareTime", 0.3f);
        m_fallAtkLaunchAngle = j.value("FallAtkLaunchAngle", 75.0f);
        m_fallAtkJumpSpeed = j.value("FallAtkJumpSpeed", 40.0f);
        m_fallAtkLandingThreshold = j.value("FallAtkLandingThreshold", 0.1f);
        m_fallAtkPredictOffset = j.value("FallAtkPredictOffset", 1.5f);
        m_fallAtkMoveThreshold = j.value("FallAtkMoveThreshold", 1.0f);
        m_fallAtkInitialFallSpeed = j.value("FallAtkInitialFallSpeed", 20.0f);
        m_fallAtkTerminalFallSpeed = j.value("FallAtkTerminalFallSpeed", 50.0f);
        m_fallAtkLandDelay = j.value("FallAtkLandDelay", 1.0f);
        // AttackRange는 부모 클래스에서 로드됨 (공격점프 사거리로 사용)
        
        // ─────────────────────────────────────────────
        // Red 전용: 단발 발사 모드 (착지 시 4방향 발사)
        // - 기본값 true (부모에서 false로 로드되므로 덮어쓰기)
        // ─────────────────────────────────────────────
        m_isDoSingleShot = j.value("IsDoSingleShot", true);
        m_fireRate = 0.0f;  // 단발 모드이므로 fireRate는 사용 안함
    }
}
