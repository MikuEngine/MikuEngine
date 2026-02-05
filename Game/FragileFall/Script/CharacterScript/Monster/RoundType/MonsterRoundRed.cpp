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
        // 상태 정의
        // ─────────────────────────────────────────────
        AddFSMState("Idle", true);              // 기본 상태
        AddFSMState("EngageJumpReady", false);  // 점프 준비 (착지점 탐색)
        AddFSMState("EngageJump", false);       // 점프 중
        AddFSMState("EngageStop", false);       // 공격 사거리 내 정지
        AddFSMState("EngageAttack", false);     // 공격
        AddFSMState("Fragile", false);
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
        m_logicFSM->SetParameter("Fragile", m_isFragile);
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

        // Any → Fragile
        AddFSMTransition("Idle", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageJumpReady", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageJump", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageStop", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageAttack", "Fragile", "Fragile", Trigger());

        // Fragile → Dead / Revive
        AddFSMTransition("Fragile", "Dead", "Die", Trigger());
        AddFSMTransition("Fragile", "Idle", "Revive", Trigger());

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
            StopAllMovement();
            RotateTowardsPlayer();
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
            (1u << engine::PhysicsLayer::Index::Environment)  // Environment만 체크
        );
        
        // Environment와 겹치지 않으면 착지 가능
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
        // Red: 기본 Linear 총알 (추후 특수 패턴으로 변경 가능)
        m_bulletParams.type = BulletType::Linear;
        m_bulletParams.speed = m_bulletSpeed;
        m_bulletParams.lifetime = m_bulletLifetime;
        m_bulletParams.damage = m_attackDamage;
    }

    // ═══════════════════════════════════════════════════════════════
    // 공격 (Red 전용)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundRed::Attack(float deltaTime)
    {
        // 공격 애니메이션 타이머 업데이트
        m_attackAnimationTimer += deltaTime;

        // 발사 가능 상태
        if (m_fireTimer <= 0.0f)
        {
            if (m_bulletFactory && m_targetPlayer && m_targetPlayer->GetGameObject())
            {
                // Red: 단순 직선 발사 (추후 특수 패턴으로 변경)
                engine::Vector3 direction = CalculateDirectionToPlayer();
                engine::Vector3 firePosition = GetTransform()->GetWorldPosition();
                
                m_bulletFactory->LinearFireMonster(firePosition, direction, m_bulletParams);

                // 공격 애니메이션 재생
                if (m_skeletalAnimator && !m_animName_EngageAttack.empty())
                {
                    m_skeletalAnimator->Play(m_animName_EngageAttack, false, 0, 1.0f);
                }

                // 발사 쿨타임 리셋
                m_fireTimer = m_fireRate;
            }
        }

        // 공격 애니메이션 완료 체크
        if (m_attackAnimationTimer >= m_attackAnimationDuration)
        {
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
    }
}
