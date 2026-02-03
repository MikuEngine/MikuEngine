#include "GamePCH.h"
#include "MonsterRoundType.h"

#include "Script/CharacterScript/Common/BulletFactory.h"

#include <Framework/Object/Component/Pathfinding/PathfindingAgent.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Object/Component/Renderer/SkeletalMeshRenderer.h>
#include <Framework/Object/Component/Renderer/StaticMeshRenderer.h>
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
        
        // 메쉬 타입 감지
        DetectMeshType();

        // PathfindingAgent 설정 (이동 몬스터용)
        if (m_pathfindingAgent)
        {
            m_pathfindingAgent->SetPathUpdateInterval(0.5f);
            m_pathfindingAgent->SetWaypointReachDistance(1.0f);
            m_pathfindingAgent->SetTargetMoveThreshold(2.0f);
        }

        // 초기 Idle 애니메이션 재생 (SkeletalMesh 사용 시에만)
        if (HasAnimation() && !m_animName_Idle.empty())
        {
            m_skeletalAnimator->Play(m_animName_Idle, true, 0, 1.0f);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // [필수 오버라이드] InitializeFSM
    // 
    // 자식 클래스에서 색상별 전용 FSM을 정의해야 합니다.
    // 아래는 샘플 구현이며, 실제 사용 시 반드시 오버라이드하세요.
    // 
    // 참고: Gray는 공격 상태 없이 4개 상태, Green은 5개 상태 사용
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundType::InitializeFSM()
    {
        // [샘플 구현] - 자식에서 오버라이드 필수!
        // 실제 게임에서는 각 색상별로 필요한 상태만 정의하세요.
        
        if (!m_logicFSM) return;

        // 샘플 8상태 FSM (모든 상태 포함)
        AddFSMState("Idle", true);
        AddFSMState("IdleMove", false);
        AddFSMState("EngageMove", false);
        AddFSMState("EngageStop", false);
        AddFSMState("EngageAttack", false);
        AddFSMState("Repositioning", false);
        AddFSMState("Fragile", false);
        AddFSMState("Dead", false);

        m_logicFSM->Initialize();

        // 샘플 파라미터
        m_logicFSM->SetParameter("IdleTimerComplete", false);
        m_logicFSM->SetParameter("PlayerDetected", false);
        m_logicFSM->SetParameter("PlayerInDetectionRange", m_isPlayerInDetectionRange);
        m_logicFSM->SetParameter("PlayerInRange", m_isPlayerInRange);
        m_logicFSM->SetParameter("CanFire", m_canFire);
        m_logicFSM->SetParameter("AttackComplete", false);
        m_logicFSM->SetParameter("NeedRepositioning", false);
        m_logicFSM->SetParameter("RepositioningComplete", false);
        m_logicFSM->SetParameter("ReturnToIdleMove", false);
        m_logicFSM->SetParameter("Fragile", m_isFragile);
        m_logicFSM->SetParameter("Die", m_isDead);

        // 샘플 전이 (Idle → IdleMove만 정의, 나머지는 자식에서)
        AddFSMTransition("Idle", "IdleMove", "IdleTimerComplete", BoolTrue());
        
        // Fragile 전이
        AddFSMTransition("Idle", "Fragile", "Fragile", Trigger());
        AddFSMTransition("IdleMove", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageMove", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageStop", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageAttack", "Fragile", "Fragile", Trigger());
        AddFSMTransition("Repositioning", "Fragile", "Fragile", Trigger());
        AddFSMTransition("Fragile", "Dead", "Die", Trigger());
        AddFSMTransition("Fragile", "Idle", "Revive", Trigger());

        m_logicFSM->InitializeCurrentState();
    }

    void MonsterRoundType::InitializeAnimFSM()
    {
        if (!m_animFSM) return;

        m_animFSM->ClearStates();

        // LogicFSM 상태 → 애니메이션 매핑
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

    // ═══════════════════════════════════════════════════════════════
    // [선택적 오버라이드] InitializeBullet
    // 
    // 기본 구현: Linear 타입 총알
    // 다른 발사 패턴 사용 시 오버라이드하세요.
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundType::InitializeBullet()
    {
        m_bulletParams.type = BulletType::Linear;
        m_bulletParams.speed = m_bulletSpeed;
        m_bulletParams.lifetime = m_bulletLifetime;
        m_bulletParams.damage = static_cast<int>(m_attackDamage);
    }

    // ═══════════════════════════════════════════════════════════════
    // [필수 오버라이드] ProcessInput
    // 
    // 자식 클래스에서 색상별 감지/파라미터 로직을 구현해야 합니다.
    // 아래는 샘플 구현입니다.
    // 
    // 예시:
    //   - Gray: 레이캐스트 감지, 공격 파라미터 없음
    //   - Green: 거리 기반 감지, 공격 쿨타임 체크
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundType::ProcessInput()
    {
        // [샘플 구현] - 자식에서 오버라이드 필수!
        
        if (!m_logicFSM) return;

        if (!m_targetPlayer)
        {
            FindPlayer();
        }

        m_isPlayerInDetectionRange = IsPlayerInDetectionRange();
        m_isPlayerInRange = IsPlayerInRange();
        m_canFire = (m_fireTimer <= 0.0f);

        m_logicFSM->SetParameter("PlayerInDetectionRange", m_isPlayerInDetectionRange);
        m_logicFSM->SetParameter("PlayerInRange", m_isPlayerInRange);
        m_logicFSM->SetParameter("CanFire", m_canFire);
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태별 행동 - 비물리 (Update에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundType::UpdateStateBasedBehavior(const std::string& state, float deltaTime)
    {
        // 발사 쿨타임 감소 (모든 상태에서)
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

    // ─────────────────────────────────────────────
    // [선택적 오버라이드] 상태별 비물리 행동
    // 기본 구현: 빈 구현 (자식에서 필요한 상태만 오버라이드)
    // ─────────────────────────────────────────────
    void MonsterRoundType::ExecuteIdleMoveBehaviorNonPhysics(float deltaTime)
    {
        // 자식에서 오버라이드 (예: Gray의 랜덤 방향 전환 타이머)
    }

    void MonsterRoundType::ExecuteEngageMoveBehaviorNonPhysics(float deltaTime)
    {
        // 자식에서 오버라이드 (이동은 보통 물리에서 처리)
    }

    void MonsterRoundType::ExecuteEngageStopBehaviorNonPhysics(float deltaTime)
    {
        // 자식에서 오버라이드
    }

    void MonsterRoundType::ExecuteEngageAttackBehaviorNonPhysics(float deltaTime)
    {
        // 기본: Attack 호출 (자식에서 오버라이드 가능)
        Attack(deltaTime);
    }

    void MonsterRoundType::ExecuteRepositioningBehaviorNonPhysics(float deltaTime)
    {
        // 자식에서 오버라이드
    }

    // ─────────────────────────────────────────────
    // [공통 사용] Idle, Fragile, Dead 비물리 행동
    // ─────────────────────────────────────────────
    void MonsterRoundType::ExecuteIdleBehaviorNonPhysics()
    {
        // Idle 타이머 → IdleTimerComplete 파라미터 설정
        m_idleTimer += engine::Time::DeltaTime();
        
        if (m_idleTimer >= m_idleWaitTime)
        {
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("IdleTimerComplete", true);
            }
        }
    }

    void MonsterRoundType::ExecuteFragileBehaviorNonPhysics()
    {
        MonsterScript::ExecuteFragileBehaviorNonPhysics();
    }

    void MonsterRoundType::ExecuteDeadBehaviorNonPhysics()
    {
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

    // ─────────────────────────────────────────────
    // [선택적 오버라이드] 상태별 물리 행동
    // 기본 구현: 빈 구현 또는 기본 이동/정지
    // ─────────────────────────────────────────────
    void MonsterRoundType::ExecuteIdleMoveBehaviorPhysics()
    {
        // 자식에서 오버라이드 (예: Gray의 4방향 이동)
    }

    void MonsterRoundType::ExecuteEngageMoveBehaviorPhysics()
    {
        // 기본: 플레이어를 향해 이동 (자식에서 오버라이드 가능)
        MoveTowardsPlayer();
    }

    void MonsterRoundType::ExecuteEngageStopBehaviorPhysics()
    {
        // 기본: 정지 + 플레이어 방향 회전
        StopAllMovement();
        RotateTowardsPlayer();
    }

    void MonsterRoundType::ExecuteEngageAttackBehaviorPhysics()
    {
        // 기본: 정지 + 플레이어 방향 회전 (자식에서 오버라이드 가능)
        StopAllMovement();
        RotateTowardsPlayer();
    }

    void MonsterRoundType::ExecuteRepositioningBehaviorPhysics()
    {
        // 자식에서 오버라이드
        StopAllMovement();
    }

    // ─────────────────────────────────────────────
    // [공통 사용] Idle, Fragile, Dead 물리 행동
    // ─────────────────────────────────────────────
    void MonsterRoundType::ExecuteIdleBehaviorPhysics()
    {
        StopAllMovement();
        StopRotation();
    }

    void MonsterRoundType::ExecuteFragileBehaviorPhysics()
    {
        StopAllMovement();
    }

    void MonsterRoundType::ExecuteDeadBehaviorPhysics()
    {
        StopAllMovement();
    }

    // ═══════════════════════════════════════════════════════════════
    // [선택적 오버라이드] OnStateEntered
    // 
    // 상태 진입 시 추가 처리가 필요하면 오버라이드하세요.
    // 부모 호출을 권장합니다: MonsterRoundType::OnStateEntered(state);
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundType::OnStateEntered(const std::string& state)
    {
        if (state == "Idle")
        {
            StopAllMovement();
            StopRotation();
            m_idleTimer = 0.0f;
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("IdleTimerComplete", false);
            }
        }
        else if (state == "IdleMove")
        {
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
    // [선택적 오버라이드] Attack
    // 
    // 기본 구현: 플레이어 방향 Linear 발사 + 애니메이션 타이머
    // 다른 발사 패턴/즉시 복귀 등이 필요하면 오버라이드하세요.
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundType::Attack(float deltaTime)
    {
        // 공격 애니메이션 타이머 업데이트
        m_attackAnimationTimer += deltaTime;

        // 발사 (쿨타임 완료 시)
        if (m_fireTimer <= 0.0f)
        {
            if (m_bulletFactory && m_targetPlayer && m_targetPlayer->GetGameObject())
            {
                engine::Vector3 direction = CalculateDirectionToPlayer();
                engine::Vector3 firePosition = GetTransform()->GetWorldPosition();
                
                m_bulletFactory->LinearFireMonster(firePosition, direction, m_bulletParams);

                if (HasAnimation() && !m_animName_EngageAttack.empty())
                {
                    m_skeletalAnimator->Play(m_animName_EngageAttack, false, 0, 1.0f);
                }

                m_fireTimer = m_fireRate;
            }
        }

        // 공격 완료 (애니메이션 타이머 기반)
        if (m_attackAnimationTimer >= m_attackAnimationDuration)
        {
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("AttackComplete", true);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // [필수 오버라이드] CanMove, CanAttack
    // 
    // 자식 클래스에서 색상별 상태 조건을 정의해야 합니다.
    // 아래는 샘플 구현입니다.
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundType::CanMove() const
    {
        // [샘플 구현] - 자식에서 오버라이드 필수!
        std::string state = GetCurrentState();
        return state == "IdleMove" || state == "EngageMove";
    }

    bool MonsterRoundType::CanAttack() const
    {
        // [샘플 구현] - 자식에서 오버라이드 필수!
        std::string state = GetCurrentState();
        return state == "EngageAttack" && !m_isFragile && !m_isDead;
    }

    // ═══════════════════════════════════════════════════════════════
    // 헬퍼 함수
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundType::IsPlayerInDetectionRange() const
    {
        return IsTargetInRange(m_targetPlayer ? m_targetPlayer->GetGameObject() : nullptr, m_detectionRange);
    }

    // ═══════════════════════════════════════════════════════════════
    // 메쉬 타입 감지
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundType::DetectMeshType()
    {
        auto* skeletalRenderer = GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>();
        if (skeletalRenderer)
        {
            m_meshType = RoundMeshType::Skeletal;
            return;
        }

        m_staticMeshRenderer = GetGameObject()->GetComponent<engine::StaticMeshRenderer>();
        if (m_staticMeshRenderer)
        {
            m_meshType = RoundMeshType::Static;
            return;
        }

        m_meshType = RoundMeshType::None;
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundType::OnGui()
    {
        ImGui::Indent();
        
        ImGui::Text("MonsterRoundType (Base):");

        MonsterScript::OnGui();

        // 메쉬 타입 정보
        ImGui::Separator();
        ImGui::Text("=== Mesh Type ===");
        
        const char* meshTypeStr = "None";
        ImVec4 meshTypeColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
        
        switch (m_meshType)
        {
        case RoundMeshType::Static:
            meshTypeStr = "StaticMeshRenderer";
            meshTypeColor = ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
            break;
        case RoundMeshType::Skeletal:
            meshTypeStr = "SkeletalMeshRenderer";
            meshTypeColor = ImVec4(0.3f, 0.6f, 1.0f, 1.0f);
            break;
        default:
            break;
        }
        
        ImGui::TextColored(meshTypeColor, "Detected: %s", meshTypeStr);
        
        // RoundType 설정
        ImGui::Separator();
        ImGui::Text("=== RoundType Settings ===");
        ImGui::DragFloat("Idle Wait Time", &m_idleWaitTime, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Attack Anim Duration", &m_attackAnimationDuration, 0.1f, 0.1f, 5.0f);

        // 애니메이션 이름
        ImGui::Separator();
        ImGui::Text("Animation Names:");
        
        if (HasAnimation())
        {
            ImGui::InputText("Idle##Round", &m_animName_Idle);
            ImGui::InputText("IdleMove##Round", &m_animName_IdleMove);
            ImGui::InputText("EngageMove##Round", &m_animName_EngageMove);
            ImGui::InputText("EngageStop##Round", &m_animName_EngageStop);
            ImGui::InputText("EngageAttack##Round", &m_animName_EngageAttack);
            ImGui::InputText("Repositioning##Round", &m_animName_Repositioning);
            ImGui::InputText("Fragile##Round", &m_animName_Fragile);
            ImGui::InputText("Dead##Round", &m_animName_Dead);
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "[Animation Not Used - Read Only]");
            
            ImGui::BeginDisabled(true);
            ImGui::InputText("Idle##Round", &m_animName_Idle);
            ImGui::InputText("IdleMove##Round", &m_animName_IdleMove);
            ImGui::InputText("EngageMove##Round", &m_animName_EngageMove);
            ImGui::InputText("EngageStop##Round", &m_animName_EngageStop);
            ImGui::InputText("EngageAttack##Round", &m_animName_EngageAttack);
            ImGui::InputText("Repositioning##Round", &m_animName_Repositioning);
            ImGui::InputText("Fragile##Round", &m_animName_Fragile);
            ImGui::InputText("Dead##Round", &m_animName_Dead);
            ImGui::EndDisabled();
        }

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
