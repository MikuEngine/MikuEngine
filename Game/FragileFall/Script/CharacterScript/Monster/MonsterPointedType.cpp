#include "GamePCH.h"
#include "MonsterPointedType.h"

#include "Script/CharacterScript/Common/BulletFactory.h"

#include <Framework/Object/Component/Pathfinding/PathfindingAgent.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Object/Component/Renderer/SkeletalMeshRenderer.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Physics/PhysicsLayer.h>
#include <Engine/Core/System/MyTime.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::Awake()
    {
        MonsterScript::Awake();
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

        // 초기 Idle 애니메이션 재생
        if (m_skeletalAnimator && !m_animName_Idle.empty())
        {
            m_skeletalAnimator->Play(m_animName_Idle, true, 0, 1.0f);
        }

        // 테스트용 Tier별 색상 설정
        switch (m_monsterTier)
        {
        case MonsterTier::Gray:
            GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>()->SetBaseColor(DirectX::SimpleMath::Vector4(0.5f, 0.5f, 0.5f, 1.0f));
            break;
        case MonsterTier::Green:
            GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>()->SetBaseColor(DirectX::SimpleMath::Vector4(0.0f, 1.0f, 0.0f, 1.0f));
            break;
        case MonsterTier::Blue:
            GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>()->SetBaseColor(DirectX::SimpleMath::Vector4(0.0f, 0.0f, 1.0f, 1.0f));
            break;
        case MonsterTier::Red:
            GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>()->SetBaseColor(DirectX::SimpleMath::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
            break;
        case MonsterTier::Purple:
            GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>()->SetBaseColor(DirectX::SimpleMath::Vector4(0.5f, 0.0f, 0.5f, 1.0f));
            break;
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
        AddFSMState("Idle", true);           // 기본 상태
        AddFSMState("EngageMove", false);    // 추적 이동 (공격 사거리 밖)
        AddFSMState("EngageStop", false);    // 정지 (공격 사거리 안, 쿨타임 중)
        AddFSMState("EngageAttack", false);  // 공격 중 정지
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
        m_logicFSM->SetParameter("CanFire", m_canFire);
        m_logicFSM->SetParameter("AttackComplete", false);
        m_logicFSM->SetParameter("Fragile", m_isFragile);
        m_logicFSM->SetParameter("Die", m_isDead);

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

        // 기존 상태 클리어
        m_animFSM->ClearStates();

        // ─────────────────────────────────────────────
        // LogicFSM 상태 → 애니메이션 매핑
        // 몬스터는 UpperBody/LowerBody 구분 없이 전체 애니메이션 재생
        // AddSplitState(상태명, 하체애니, 하체루프, 상체애니, 상체루프, 상체웨이트, 크로스페이드)
        // 상체웨이트 0 = 전체 애니메이션 (상/하체 분리 안 함)
        // ─────────────────────────────────────────────
        m_animFSM->AddSplitState("Idle",         m_animName_Idle, true,  "", false, 0.0f, 0.1f);
        m_animFSM->AddSplitState("EngageMove",   m_animName_EngageMove, true, "", false, 0.0f, 0.1f);
        m_animFSM->AddSplitState("EngageStop",   m_animName_EngageStop, true, "", false, 0.0f, 0.1f);         // Idle 재생
        m_animFSM->AddSplitState("EngageAttack", m_animName_EngageAttack, true, "", false, 0.0f, 0.1f);
        m_animFSM->AddSplitState("Fragile",      m_animName_Fragile, true, "", false, 0.0f, 0.1f);         // Idle 재생
        m_animFSM->AddSplitState("Dead",         m_animName_Dead, true, "", false, 0.0f, 0.1f);         // Idle 재생
    }

    void MonsterPointedType::InitializeAnimations()
    {
        if (!m_skeletalAnimator) return;

        // SkeletalAnimator에 애니메이션 등록
        // 실제 .fbx 파일 경로는 에디터에서 설정하거나
        // 씬 파일에서 로드됨
        // 여기서는 이름만 연결
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
        case MonsterTier::Green:
            m_bulletParams.type = BulletType::Parabolic;
            m_bulletParams.ownGravity = 9.81f;
            m_bulletParams.lifetime = m_bulletLifetime;
            m_bulletParams.damage = 15;
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
        
        // 발사 가능 여부 체크 (쿨타임)
        m_canFire = (m_fireTimer <= 0.0f);

        // FSM 파라미터 업데이트
        m_logicFSM->SetParameter("PlayerInDetectionRange", m_isPlayerInDetectionRange);
        m_logicFSM->SetParameter("PlayerInRange", m_isPlayerInRange);
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

        if (state == "EngageMove")
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
        else if (state == "Fragile")
        {
            ExecuteFragileBehaviorNonPhysics();
        }
        else if (state == "Dead")
        {
            ExecuteDeadBehaviorNonPhysics();
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

    // ═══════════════════════════════════════════════════════════════
    // 상태별 행동 - 물리 (FixedUpdate에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::UpdatePhysicsStateBasedBehavior(const std::string& state)
    {
        if (state == "EngageMove")
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
        else if (state == "Fragile")
        {
            ExecuteFragileBehaviorPhysics();
        }
        else if (state == "Dead")
        {
            ExecuteDeadBehaviorPhysics();
        }
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

    // ═══════════════════════════════════════════════════════════════
    // 상태 진입 콜백
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::OnStateEntered(const std::string& state)
    {
        if (state == "EngageAttack")
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
                // 뾰족 초록
                // 
                // 플레이어를 추격하며 멈춰 서서 투사체를 발사하고 착탄 지점에 범위 공격
                // ─────────────────────────────────────────────
                case MonsterTier::Green:
                {
                    engine::Vector3 startPos = firePosition;
                    engine::Vector3 targetPos = m_targetPlayer->GetTransform()->GetWorldPosition();

                    // 수평 벡터와 거리 계산
                    engine::Vector3 diff = targetPos - startPos;
                    engine::Vector3 horizontalDiff = { diff.x, 0.0f, diff.z };
                    float distance = horizontalDiff.Length();

                    // 날아가는 시간 설정
                    float travelTime = 1.5f;

                    // 수평 속도 계산 (V = S / t)
                    float horizontalSpeed = distance / travelTime;
                    engine::Vector3 horizontalDir = horizontalDiff;

                    horizontalDir.Normalize();

                    // 수직 초기 속도 계산 (Vy = (dy + 0.5 * g * t^2) / t)
                    float dy = diff.y;
                    float verticalSpeed = (dy + 0.5f * m_bulletParams.ownGravity * travelTime * travelTime) / travelTime;

                    engine::Vector3 finalVelocity = (horizontalDir * horizontalSpeed) + (engine::Vector3::Up * verticalSpeed);
                    float finalSpeed = finalVelocity.Length();
                    finalVelocity.Normalize();

                    m_bulletParams.speed = finalSpeed;
                    m_bulletFactory->ParabolicFireMonster(startPos, finalVelocity, m_bulletParams);
                    break;
                }
                // ─────────────────────────────────────────────
                // 뾰족 파랑
                // 
                // 플레이어를 추격하며 멈춰 서서 투사체를 발사하고 4방향 or 플레이어 방향으로 3발 발사
                // ─────────────────────────────────────────────
                case MonsterTier::Blue:
                {
                    // 중앙
                    m_bulletFactory->LinearFireMonster(firePosition, direction, m_bulletParams);
                    // 좌우 15도씩 벌려서 발사
                    float angleOffset = DirectX::XMConvertToRadians(15.0f);
                    DirectX::SimpleMath::Matrix rotationMatrix;
                    // 왼쪽
                    rotationMatrix = DirectX::SimpleMath::Matrix::CreateRotationY(-angleOffset);
                    engine::Vector3 leftDir = engine::Vector3::TransformNormal(direction, rotationMatrix);
                    leftDir.Normalize();
                    m_bulletFactory->LinearFireMonster(firePosition, leftDir, m_bulletParams);
                    // 오른쪽
                    rotationMatrix = DirectX::SimpleMath::Matrix::CreateRotationY(angleOffset);
                    engine::Vector3 rightDir = engine::Vector3::TransformNormal(direction, rotationMatrix);
                    rightDir.Normalize();
                    m_bulletFactory->LinearFireMonster(firePosition, rightDir, m_bulletParams);
                    break;
				}
                // ─────────────────────────────────────────────
                // 뾰족 빨강
                // 
                // 공격 볌위에 들어오면 플레이어 주변에게 투사체를 난사
                // ─────────────────────────────────────────────
                case MonsterTier::Red:
                {
                    int projectileCount = 8;
                    for (int i = 0; i < projectileCount; ++i)
                    {
                        float angle = DirectX::XM_2PI * (static_cast<float>(i) / static_cast<float>(projectileCount));
                        engine::Vector3 dir = engine::Vector3(cosf(angle), 0.0f, sinf(angle));
                        m_bulletFactory->LinearFireMonster(firePosition, dir, m_bulletParams);
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

                // 공격 애니메이션 재생
                if (m_skeletalAnimator && !m_animName_EngageAttack.empty())
                {
                    m_skeletalAnimator->Play(m_animName_EngageAttack, false, 0, 1.0f);
                }

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
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void MonsterPointedType::OnGui()
    {
        ImGui::Indent();
        
        // ─────────────────────────────────────────────
        // 컴포넌트 검증 (에디터 화면에서도 체크)
        // ─────────────────────────────────────────────
        ImGui::Separator();
        ImGui::Text("=== Component Validation ===");
        
        // 에디터 모드를 위한 실시간 컴포넌트 검색 (같은 GameObject 내에서만 검색)
        engine::Rigidbody* rigidbody = m_rigidbody ? m_rigidbody : (GetGameObject() ? GetGameObject()->GetComponent<engine::Rigidbody>() : nullptr);
        engine::SkeletalAnimator* skeletalAnimator = m_skeletalAnimator ? m_skeletalAnimator : (GetGameObject() ? GetGameObject()->GetComponent<engine::SkeletalAnimator>() : nullptr);
        engine::AnimFSM* animFSM = m_animFSM ? m_animFSM : (GetGameObject() ? GetGameObject()->GetComponent<engine::AnimFSM>() : nullptr);
        engine::LogicFSM* logicFSM = m_logicFSM ? m_logicFSM : (GetGameObject() ? GetGameObject()->GetComponent<engine::LogicFSM>() : nullptr);
        BulletFactory* bulletFactory = m_bulletFactory ? m_bulletFactory : (GetGameObject() ? GetGameObject()->GetComponent<BulletFactory>() : nullptr);
        engine::PathfindingAgent* pathfindingAgent = m_pathfindingAgent ? m_pathfindingAgent : (GetGameObject() ? GetGameObject()->GetComponent<engine::PathfindingAgent>() : nullptr);
        
        // 전체 유효성 검사 (PathfindingAgent는 이동 몬스터 필수)
        bool allValid = rigidbody && skeletalAnimator && bulletFactory && animFSM && logicFSM && pathfindingAgent;
        
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
        
        ImGui::Text("SkeletalAnimator:  %s", skeletalAnimator ? "[OK]" : "[MISSING]");
        if (!skeletalAnimator) ImGui::SameLine(); if (!skeletalAnimator) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");
        
        ImGui::Text("BulletFactory:     %s", bulletFactory ? "[OK]" : "[MISSING]");
        if (!bulletFactory) ImGui::SameLine(); if (!bulletFactory) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");
        
        ImGui::Text("AnimFSM:           %s", animFSM ? "[OK]" : "[MISSING]");
        if (!animFSM) ImGui::SameLine(); if (!animFSM) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");
        
        ImGui::Text("LogicFSM:          %s", logicFSM ? "[OK]" : "[MISSING]");
        if (!logicFSM) ImGui::SameLine(); if (!logicFSM) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");
        
        ImGui::Text("PathfindingAgent:  %s", pathfindingAgent ? "[OK]" : "[MISSING]");
        if (!pathfindingAgent) ImGui::SameLine(); if (!pathfindingAgent) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");
        ImGui::Unindent();

        // 공격 타입
        ImGui::Separator();
        ImGui::Text("Type:");
        if (ImGui::BeginCombo("Attack Type", GetAttackTypeStr(m_attackType)))
        {
            for (int i = 0; i < (int)AttackType::Max; ++i)
            {
                AttackType currentType = (AttackType)i;
                bool isSelected = (m_attackType == currentType);

                if (ImGui::Selectable(GetAttackTypeStr(currentType), isSelected))
                {
                    m_attackType = currentType;
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

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
        ImGui::DragFloat("Attack Anim Duration", &m_attackAnimationDuration, 0.1f, 0.1f, 5.0f);

        // PointedGreen 고유 설정
        ImGui::Separator();
        ImGui::Text("Animation Names:");
        ImGui::InputText("Idle", &m_animName_Idle);
        ImGui::InputText("EngageMove", &m_animName_EngageMove);
        ImGui::InputText("EngageStop", &m_animName_EngageStop);
        ImGui::InputText("EngageAttack", &m_animName_EngageAttack);       
        ImGui::InputText("Fragile", &m_animName_Fragile);
        ImGui::InputText("Dead", &m_animName_Dead);

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
        j["AnimName_Idle"] = m_animName_Idle;
        j["AnimName_EngageMove"] = m_animName_EngageMove;
        j["AnimName_EngageStop"] = m_animName_EngageStop;
        j["AnimName_EngageAttack"] = m_animName_EngageAttack;        
        j["AnimName_Fragile"] = m_animName_Fragile;
        j["AnimName_Dead"] = m_animName_Dead;
    }

    void MonsterPointedType::Load(const engine::json& j)
    {
        MonsterScript::Load(j);
        
        m_detectionRange = j.value("DetectionRange", 15.0f);
        m_attackAnimationDuration = j.value("AttackAnimationDuration", 1.0f);
        m_animName_Idle = j.value("AnimName_Idle", "Idle");
        m_animName_EngageMove = j.value("AnimName_EngageMove", "EngageMove");
        m_animName_EngageStop = j.value("AnimName_EngageStop", "EngageStop");
        m_animName_EngageAttack = j.value("AnimName_EngageAttack", "EngageAttack");        
        m_animName_Fragile = j.value("AnimName_Fragile", "Fragile");
        m_animName_Dead = j.value("AnimName_Dead", "Dead");
    }
}
