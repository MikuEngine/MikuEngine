#include "GamePCH.h"
#include "MonsterRoundType.h"

#include "Script/CharacterScript/Common/BulletFactory.h"

#include <Framework/Object/Component/Pathfinding/PathfindingAgent.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Physics/PhysicsLayer.h>
#include <Engine/Core/System/MyTime.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundType::Awake()
    {
        MonsterScript::Awake();
        
        // Round 타입 고정
        m_attackType = AttackType::Round;
    }

    void MonsterRoundType::Start()
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
    }

    // ═══════════════════════════════════════════════════════════════
    // FSM 초기화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundType::InitializeFSM()
    {
        if (!m_logicFSM) return;

        // ─────────────────────────────────────────────
        // 상태 정의 (동글몬 전용)
        // ─────────────────────────────────────────────
        AddFSMState("Idle", true);           // 기본 상태 (대기)
        AddFSMState("IdleMove", false);      // 맵 배회
        AddFSMState("EngageMove", false);    // 추적 이동
        AddFSMState("EngageStop", false);    // 정지 (공격 사거리 안, 쿨타임 중)
        AddFSMState("EngageAttack", false);  // 공격 중 정지
        AddFSMState("Repositioning", false); // 위치 보정
        AddFSMState("Fragile", false);
        AddFSMState("Dead", false);

        // ─────────────────────────────────────────────
        // StateMap 업데이트 (Transition 추가 전에 필수!)
        // ─────────────────────────────────────────────
        m_logicFSM->Initialize();  // UpdateStateMap() 호출

        // ─────────────────────────────────────────────
        // 파라미터 정의
        // ─────────────────────────────────────────────
        m_logicFSM->SetParameter("IdleTimerComplete", false);     // Idle 대기 시간 완료
        m_logicFSM->SetParameter("PlayerDetected", false);        // 4방향 레이캐스트 감지
        m_logicFSM->SetParameter("PlayerInDetectionRange", m_isPlayerInDetectionRange);
        m_logicFSM->SetParameter("PlayerInRange", m_isPlayerInRange);
        m_logicFSM->SetParameter("CanFire", m_canFire);
        m_logicFSM->SetParameter("AttackComplete", false);
        m_logicFSM->SetParameter("NeedRepositioning", false);     // 위치 보정 필요
        m_logicFSM->SetParameter("RepositioningComplete", false); // 위치 보정 완료
        m_logicFSM->SetParameter("ReturnToIdleMove", false);      // EngageMove 충돌 후 IdleMove 복귀
        m_logicFSM->SetParameter("Fragile", m_isFragile);
        m_logicFSM->SetParameter("Die", m_isDead);

        // ─────────────────────────────────────────────
        // 전이 정의
        // ─────────────────────────────────────────────
        
        // Idle → IdleMove (대기 시간 완료)
        AddFSMTransition("Idle", "IdleMove", "IdleTimerComplete", BoolTrue());
        
        // IdleMove → EngageMove (플레이어 레이캐스트 감지)
        AddFSMTransition("IdleMove", "EngageMove", "PlayerDetected", BoolTrue());
        
        // IdleMove → Repositioning (위치 보정 필요)
        AddFSMTransition("IdleMove", "Repositioning", "NeedRepositioning", BoolTrue());
        
        // EngageMove → IdleMove (충돌 후 복귀)
        AddFSMTransition("EngageMove", "IdleMove", "ReturnToIdleMove", BoolTrue());
        
        // EngageMove → Repositioning (위치 보정 필요)
        AddFSMTransition("EngageMove", "Repositioning", "NeedRepositioning", BoolTrue());

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
        
        // Repositioning → IdleMove (위치 보정 완료)
        AddFSMTransition("Repositioning", "IdleMove", "RepositioningComplete", BoolTrue());

        // Any → Fragile (HP 0, Fragile 트리거)
        AddFSMTransition("Idle", "Fragile", "Fragile", Trigger());
        AddFSMTransition("IdleMove", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageMove", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageStop", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageAttack", "Fragile", "Fragile", Trigger());
        AddFSMTransition("Repositioning", "Fragile", "Fragile", Trigger());

        // Fragile → Dead (Execution, Die 트리거)
        AddFSMTransition("Fragile", "Dead", "Die", Trigger());
        
        // Fragile → Idle (부활, Revive 트리거)
        AddFSMTransition("Fragile", "Idle", "Revive", Trigger());

        // ─────────────────────────────────────────────
        // 초기 상태 설정
        // ─────────────────────────────────────────────
        m_logicFSM->InitializeCurrentState();  // 기본 상태로 설정
    }

    void MonsterRoundType::InitializeAnimFSM()
    {
        if (!m_animFSM) return;

        // 기존 상태 클리어
        m_animFSM->ClearStates();

        // ─────────────────────────────────────────────
        // LogicFSM 상태 → 애니메이션 매핑
        // ─────────────────────────────────────────────
        m_animFSM->AddSplitState("Idle",          m_animName_Idle, true,  "", false, 0.0f, 0.1f);
        m_animFSM->AddSplitState("IdleMove",      m_animName_IdleMove, true, "", false, 0.0f, 0.1f);
        m_animFSM->AddSplitState("EngageMove",    m_animName_EngageMove, true, "", false, 0.0f, 0.1f);
        m_animFSM->AddSplitState("EngageStop",    m_animName_EngageStop, true, "", false, 0.0f, 0.1f);
        m_animFSM->AddSplitState("EngageAttack",  m_animName_EngageAttack, false, "", false, 0.0f, 0.1f);
        m_animFSM->AddSplitState("Repositioning", m_animName_Repositioning, true, "", false, 0.0f, 0.1f);
        m_animFSM->AddSplitState("Fragile",       m_animName_Fragile, true, "", false, 0.0f, 0.1f);
        m_animFSM->AddSplitState("Dead",          m_animName_Dead, false, "", false, 0.0f, 0.1f);
    }

    void MonsterRoundType::InitializeAnimations()
    {
        if (!m_skeletalAnimator) return;
        // 애니메이션은 에디터에서 설정하거나 씬 파일에서 로드됨
    }

    void MonsterRoundType::InitializeBullet()
    {
        m_bulletParams.type = BulletType::Linear;
        m_bulletParams.speed = m_bulletSpeed;
        m_bulletParams.lifetime = m_bulletLifetime;
        m_bulletParams.damage = static_cast<int>(m_attackDamage);
    }

    // ═══════════════════════════════════════════════════════════════
    // 입력 처리 (FSM 파라미터 업데이트)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundType::ProcessInput()
    {
        if (!m_logicFSM) return;

        // 플레이어를 찾지 못했으면 재탐색
        if (!m_targetPlayer)
        {
            FindPlayer();
        }

        // 플레이어와의 거리 체크
        m_isPlayerInDetectionRange = IsPlayerInDetectionRange();
        m_isPlayerInRange = IsPlayerInRange();
        
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
    void MonsterRoundType::UpdateStateBasedBehavior(const std::string& state, float deltaTime)
    {
        // 발사 쿨타임 감소 (모든 상태에서, 비물리)
        if (m_fireTimer > 0.0f)
        {
            m_fireTimer -= deltaTime;
        }

        if (state == "Idle")
        {
            ExecuteIdleBehaviorNonPhysics();
        }
        else if (state == "IdleMove")
        {
            ExecuteIdleMoveBehaviorNonPhysics(deltaTime);
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
        else if (state == "Repositioning")
        {
            ExecuteRepositioningBehaviorNonPhysics(deltaTime);
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

    void MonsterRoundType::ExecuteIdleMoveBehaviorNonPhysics(float deltaTime)
    {
        // 기본: 없음 (자식에서 오버라이드)
    }

    void MonsterRoundType::ExecuteEngageMoveBehaviorNonPhysics(float deltaTime)
    {
        // 기본: 없음 (이동은 물리에서 처리)
        // 자식에서 오버라이드 가능
    }

    void MonsterRoundType::ExecuteEngageStopBehaviorNonPhysics(float deltaTime)
    {
        // 기본: 없음 (회전은 물리에서 처리)
        // 자식에서 오버라이드 가능
    }

    void MonsterRoundType::ExecuteEngageAttackBehaviorNonPhysics(float deltaTime)
    {
        // 공격 로직 (비물리)
        Attack(deltaTime);
    }

    void MonsterRoundType::ExecuteRepositioningBehaviorNonPhysics(float deltaTime)
    {
        // 기본: 없음 (자식에서 오버라이드)
    }

    void MonsterRoundType::ExecuteIdleBehaviorNonPhysics()
    {
        // Idle 타이머 업데이트
        m_idleTimer += engine::Time::DeltaTime();
        
        if (m_idleTimer >= m_idleWaitTime)
        {
            // 타이머 완료 → IdleMove로 전이
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("IdleTimerComplete", true);
            }
        }
    }

    void MonsterRoundType::ExecuteFragileBehaviorNonPhysics()
    {
        // 부모 클래스의 Fragile 처리 (부활 타이머)
        MonsterScript::ExecuteFragileBehaviorNonPhysics();
    }

    void MonsterRoundType::ExecuteDeadBehaviorNonPhysics()
    {
        // 부모 클래스의 Dead 처리 (파괴 타이머)
        MonsterScript::ExecuteDeadBehaviorNonPhysics();
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태별 행동 - 물리 (FixedUpdate에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundType::UpdatePhysicsStateBasedBehavior(const std::string& state)
    {
        if (state == "Idle")
        {
            ExecuteIdleBehaviorPhysics();
        }
        else if (state == "IdleMove")
        {
            ExecuteIdleMoveBehaviorPhysics();
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
        else if (state == "Repositioning")
        {
            ExecuteRepositioningBehaviorPhysics();
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

    void MonsterRoundType::ExecuteIdleMoveBehaviorPhysics()
    {
        // 기본: 없음 (자식에서 오버라이드)
    }

    void MonsterRoundType::ExecuteEngageMoveBehaviorPhysics()
    {
        // 플레이어를 향해 이동 (물리)
        MoveTowardsPlayer();
    }

    void MonsterRoundType::ExecuteEngageStopBehaviorPhysics()
    {
        // 공격 사거리 안에 있지만 쿨타임 중 - 정지하고 회전만 (물리)
        StopAllMovement();
        RotateTowardsPlayer();
    }

    void MonsterRoundType::ExecuteEngageAttackBehaviorPhysics()
    {
        // 공격 중에는 이동 불가, 회전만 가능 (물리)
        StopAllMovement();
        RotateTowardsPlayer();
    }

    void MonsterRoundType::ExecuteRepositioningBehaviorPhysics()
    {
        // 기본: 정지 (자식에서 오버라이드)
        StopAllMovement();
    }

    void MonsterRoundType::ExecuteIdleBehaviorPhysics()
    {
        StopAllMovement();
        StopRotation();
    }

    void MonsterRoundType::ExecuteFragileBehaviorPhysics()
    {
        // Fragile 상태: 물리적으로 정지
        StopAllMovement();
    }

    void MonsterRoundType::ExecuteDeadBehaviorPhysics()
    {
        StopAllMovement();
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태 진입 콜백
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundType::OnStateEntered(const std::string& state)
    {
        if (state == "Idle")
        {
            StopAllMovement();
            StopRotation();
            m_idleTimer = 0.0f;  // Idle 타이머 초기화
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("IdleTimerComplete", false);
            }
        }
        else if (state == "IdleMove")
        {
            // IdleMove 진입 시 파라미터 초기화
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("PlayerDetected", false);
                m_logicFSM->SetParameter("NeedRepositioning", false);
            }
        }
        else if (state == "EngageMove")
        {
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("AttackComplete", false);
                m_logicFSM->SetParameter("ReturnToIdleMove", false);
                m_logicFSM->SetParameter("NeedRepositioning", false);
            }
        }
        else if (state == "EngageStop")
        {
            StopAllMovement();
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("AttackComplete", false);
            }
        }
        else if (state == "EngageAttack")
        {
            // 공격 상태 진입 시 이동 멈추고 타이머 초기화
            StopAllMovement();
            m_attackAnimationTimer = 0.0f;
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("AttackComplete", false);
            }
        }
        else if (state == "Repositioning")
        {
            StopAllMovement();
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("RepositioningComplete", false);
                m_logicFSM->SetParameter("NeedRepositioning", false);
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
    }

    // ═══════════════════════════════════════════════════════════════
    // 공격 (기본 구현 - 자식에서 오버라이드)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundType::Attack(float deltaTime)
    {
        // 공격 애니메이션 타이머 업데이트
        m_attackAnimationTimer += deltaTime;

        // EngageAttack 상태 진입 시 즉시 발사 (CanFire가 true였기 때문에 진입)
        if (m_fireTimer <= 0.0f)
        {
            // 발사 가능 상태
            if (m_bulletFactory && m_targetPlayer && m_targetPlayer->GetGameObject())
            {
                // 플레이어를 향해 발사
                engine::Vector3 direction = CalculateDirectionToPlayer();
                engine::Vector3 firePosition = GetTransform()->GetWorldPosition();
                
                // 기본: Linear 발사 (자식에서 오버라이드하여 다른 패턴 구현)
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
            m_logicFSM->SetParameter("AttackComplete", true);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 행동 제한
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundType::CanMove() const
    {
        std::string state = GetCurrentState();
        return state == "IdleMove" || state == "EngageMove";
    }

    bool MonsterRoundType::CanAttack() const
    {
        std::string state = GetCurrentState();
        return state == "EngageAttack" && state != "Fragile" && state != "Dead";
    }

    // ═══════════════════════════════════════════════════════════════
    // 헬퍼 함수
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundType::IsPlayerInDetectionRange() const
    {
        return IsTargetInRange(m_targetPlayer ? m_targetPlayer->GetGameObject() : nullptr, m_detectionRange);
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundType::OnGui()
    {
        ImGui::Indent();
        
        ImGui::Text("MonsterRoundType (Base):");

        // 부모 클래스 OnGui 호출
        MonsterScript::OnGui();

        // ─────────────────────────────────────────────
        // RoundType 고유 설정
        // ─────────────────────────────────────────────
        ImGui::Separator();
        ImGui::Text("=== RoundType Settings ===");
        ImGui::DragFloat("Idle Wait Time", &m_idleWaitTime, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Attack Anim Duration", &m_attackAnimationDuration, 0.1f, 0.1f, 5.0f);

        // 애니메이션 이름
        ImGui::Separator();
        ImGui::Text("Animation Names:");
        ImGui::InputText("Idle##Round", &m_animName_Idle);
        ImGui::InputText("IdleMove##Round", &m_animName_IdleMove);
        ImGui::InputText("EngageMove##Round", &m_animName_EngageMove);
        ImGui::InputText("EngageStop##Round", &m_animName_EngageStop);
        ImGui::InputText("EngageAttack##Round", &m_animName_EngageAttack);
        ImGui::InputText("Repositioning##Round", &m_animName_Repositioning);
        ImGui::InputText("Fragile##Round", &m_animName_Fragile);
        ImGui::InputText("Dead##Round", &m_animName_Dead);

        // 런타임 정보
        ImGui::Separator();
        ImGui::Text("=== RoundType Runtime ===");
        ImGui::Text("Idle Timer: %.2f / %.2f", m_idleTimer, m_idleWaitTime);
        ImGui::Text("Player In Detection Range: %s", m_isPlayerInDetectionRange ? "Yes" : "No");
        ImGui::Text("Can Fire: %s", m_canFire ? "Yes" : "No");
        ImGui::Text("Attack Anim Timer: %.2f / %.2f", m_attackAnimationTimer, m_attackAnimationDuration);
        
        ImGui::Unindent();
    }

    void MonsterRoundType::Save(engine::json& j) const
    {
        MonsterScript::Save(j);
        
        j["IdleWaitTime"] = m_idleWaitTime;
        j["AttackAnimationDuration"] = m_attackAnimationDuration;
        j["AnimName_Idle"] = m_animName_Idle;
        j["AnimName_IdleMove"] = m_animName_IdleMove;
        j["AnimName_EngageMove"] = m_animName_EngageMove;
        j["AnimName_EngageStop"] = m_animName_EngageStop;
        j["AnimName_EngageAttack"] = m_animName_EngageAttack;
        j["AnimName_Repositioning"] = m_animName_Repositioning;
        j["AnimName_Fragile"] = m_animName_Fragile;
        j["AnimName_Dead"] = m_animName_Dead;
    }

    void MonsterRoundType::Load(const engine::json& j)
    {
        MonsterScript::Load(j);
        
        m_idleWaitTime = j.value("IdleWaitTime", 1.0f);
        m_attackAnimationDuration = j.value("AttackAnimationDuration", 1.0f);
        m_animName_Idle = j.value("AnimName_Idle", "Idle");
        m_animName_IdleMove = j.value("AnimName_IdleMove", "IdleMove");
        m_animName_EngageMove = j.value("AnimName_EngageMove", "EngageMove");
        m_animName_EngageStop = j.value("AnimName_EngageStop", "EngageStop");
        m_animName_EngageAttack = j.value("AnimName_EngageAttack", "EngageAttack");
        m_animName_Repositioning = j.value("AnimName_Repositioning", "Repositioning");
        m_animName_Fragile = j.value("AnimName_Fragile", "Fragile");
        m_animName_Dead = j.value("AnimName_Dead", "Dead");
    }
}