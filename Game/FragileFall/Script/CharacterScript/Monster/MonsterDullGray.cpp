#include "GamePCH.h"
#include "MonsterDullGray.h"

#include "Script/CharacterScript/Common/BulletFactory.h"

#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Physics/PhysicsLayer.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullGray::Awake()
    {
        MonsterScript::Awake();
        // 스탯은 씬 파일에서 로드됨 (Load 함수 참조)
    }

    void MonsterDullGray::Start()
    {
        MonsterScript::Start();

        // 초기 Idle 애니메이션 재생
        if (m_skeletalAnimator && !m_animName_Idle.empty())
        {
            m_skeletalAnimator->Play(m_animName_Idle, true, 0, 1.0f);
        } 
    }

    // ═══════════════════════════════════════════════════════════════
    // FSM 초기화
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullGray::InitializeFSM()
    {
        if (!m_logicFSM) return;

        // ─────────────────────────────────────────────
        // 상태 정의
        // ─────────────────────────────────────────────
        AddFSMState("Idle", true);    // 기본 상태
        AddFSMState("Engage", false);
        AddFSMState("Fragile", false);
        AddFSMState("Dead", false);

        // ─────────────────────────────────────────────
        // StateMap 업데이트 (Transition 추가 전에 필수!)
        // ─────────────────────────────────────────────
        m_logicFSM->Initialize();  // UpdateStateMap() 호출

        // ─────────────────────────────────────────────
        // 파라미터 정의
        // ─────────────────────────────────────────────
        m_logicFSM->SetParameter("PlayerInRange", m_isPlayerInRange);
        m_logicFSM->SetParameter("Fragile", m_isFragile);
        m_logicFSM->SetParameter("Die", m_isDead);

        // ─────────────────────────────────────────────
        // 전이 정의 (StateMap이 업데이트된 후에 추가)
        // ─────────────────────────────────────────────
        // Idle ↔ Engage (플레이어 사거리 진입/이탈)
        AddFSMTransition("Idle", "Engage", "PlayerInRange", BoolTrue());
        AddFSMTransition("Engage", "Idle", "PlayerInRange", BoolFalse());

        // Any → Fragile (HP 0, Fragile 트리거)
        AddFSMTransition("Idle", "Fragile", "Fragile", Trigger());
        AddFSMTransition("Engage", "Fragile", "Fragile", Trigger());

        // Fragile → Dead (Execution, Die 트리거)
        AddFSMTransition("Fragile", "Dead", "Die", Trigger());

        // ─────────────────────────────────────────────
        // 초기 상태 설정
        // ─────────────────────────────────────────────
        m_logicFSM->InitializeCurrentState();  // 기본 상태로 설정
    }

    void MonsterDullGray::InitializeAnimFSM()
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
        m_animFSM->AddSplitState("Idle",    m_animName_Idle, true,  "", false, 0.0f, 0.1f);
        m_animFSM->AddSplitState("Engage",  m_animName_Idle, true,  "", false, 0.0f, 0.1f);  // Engage는 Idle 재생
        m_animFSM->AddSplitState("Fragile", m_animName_Idle, true,  "", false, 0.0f, 0.1f);  // Fragile은 Idle 재생 (전용 애니메이션 설정 가능)
        m_animFSM->AddSplitState("Dead",    m_animName_Dead, false, "", false, 0.0f, 0.1f);  // Dead는 루프 안 함
    }

    void MonsterDullGray::InitializeAnimations()
    {
        if (!m_skeletalAnimator) return;

        // SkeletalAnimator에 애니메이션 등록
        // 실제 .fbx 파일 경로는 에디터에서 설정하거나
        // 씬 파일에서 로드됨
        // 여기서는 이름만 연결
    }

    void MonsterDullGray::InitializeBullet()
    {
        m_bulletParams.type = BulletType::Linear;
        m_bulletParams.speed = m_bulletSpeed;
        m_bulletParams.lifetime = m_bulletLifetime;
        m_bulletParams.damage = 10;
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태별 행동 - 비물리 (Update에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullGray::UpdateStateBasedBehavior(const std::string& state, float deltaTime)
    {
        if (state == "Engage")
        {
            ExecuteEngageBehaviorNonPhysics(deltaTime);
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

    void MonsterDullGray::ExecuteEngageBehaviorNonPhysics(float deltaTime)
    {
        // 공격 타이머 및 발사 (비물리)
        Attack(deltaTime);
    }

    void MonsterDullGray::ExecuteIdleBehaviorNonPhysics()
    {
        // 비물리 Idle 처리
    }

    void MonsterDullGray::ExecuteFragileBehaviorNonPhysics()
    {
        // Fragile 상태: 아무 행동도 하지 않음 (Execution 대기)
    }

    void MonsterDullGray::ExecuteDeadBehaviorNonPhysics()
    {
        // 부모 클래스의 Dead 타이머 처리 (2초 후 Destroy)
        MonsterScript::ExecuteDeadBehaviorNonPhysics();
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태별 행동 - 물리 (FixedUpdate에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullGray::UpdatePhysicsStateBasedBehavior(const std::string& state)
    {
        if (state == "Engage")
        {
            ExecuteEngageBehaviorPhysics();
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

    void MonsterDullGray::ExecuteEngageBehaviorPhysics()
    {
        // 회전 (물리)
        RotateTowardsPlayer();
    }

    void MonsterDullGray::ExecuteIdleBehaviorPhysics()
    {
        StopRotation();
    }

    void MonsterDullGray::ExecuteFragileBehaviorPhysics()
    {
        // Fragile 상태: 물리적으로 정지
    }

    void MonsterDullGray::ExecuteDeadBehaviorPhysics()
    {
        StopAllMovement();
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태 진입 콜백
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullGray::OnStateEntered(const std::string& state)
    {
        if (state == "Fragile")
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
            StopRotation();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 공격
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullGray::Attack(float deltaTime)
    {
        if (m_fireTimer > 0.0f)
        {
            m_fireTimer -= deltaTime;
            return;
        }

        if (!m_bulletFactory || !m_targetPlayer || !m_targetPlayer->GetGameObject())
        {
            return;
        }

        // 현재 보고 있는 방향으로 발사. 현재 쓰고있는 메쉬는 앞뒤가 반대라 * -1.0f
        //engine::Vector3 direction = GetForwardDirection();
        //direction *= -1.0f;

        // 그냥 플레이어를 향해 발사.
        engine::Vector3 direction = CalculateDirectionToPlayer();

        engine::Vector3 firePosition = GetTransform()->GetWorldPosition();
        m_bulletFactory->LinearFireMonster(firePosition, direction, m_bulletParams);

        if (m_skeletalAnimator && !m_animName_Attack.empty())
        {
            m_skeletalAnimator->Play(m_animName_Attack, false, 0, 1.0f);
        }

        m_fireTimer = m_fireRate;
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullGray::OnGui()
    {
        ImGui::Indent();
        
        ImGui::Text("MonsterDullGray:");

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
        
        // 전체 유효성 검사
        bool allValid = rigidbody && skeletalAnimator && bulletFactory && animFSM && logicFSM;
        
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
        ImGui::Unindent();

        // 스탯
        ImGui::Separator();
        ImGui::Text("Stats:");
        ImGui::DragInt("HP", &m_Hp, 1, 1, 10);
        ImGui::DragFloat("Attack Range", &m_AttackRange, 0.1f, 0.0f, 15.0f);

        // 설정
        ImGui::Separator();
        ImGui::Text("Settings:");
        ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Rotation Speed", &m_rotationSpeed, 0.1f, 0.0f, 10.0f);
        ImGui::DragFloat("Fire Rate (sec)", &m_fireRate, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Bullet Speed", &m_bulletSpeed, 0.1f, 0.1f, 100.0f);
        ImGui::DragFloat("Bullet Lifetime", &m_bulletLifetime, 0.1f, 0.5f, 10.0f);

        // DullGray 고유 설정
        ImGui::Separator();
        ImGui::Text("Animation Names:");
        ImGui::InputText("Idle", &m_animName_Idle);
        ImGui::InputText("Attack", &m_animName_Attack);
        ImGui::InputText("Fragile", &m_animName_Fragile);
        ImGui::InputText("Dead", &m_animName_Dead);

        // 런타임 정보
        ImGui::Separator();
        ImGui::Text("Runtime Info:");
        ImGui::Text("Current State: %s", GetCurrentState().c_str());
        ImGui::Text("Player Found: %s", m_targetPlayer ? "Yes" : "No");
        if (m_targetPlayer)
        {
            ImGui::Text("Distance to Player: %.2f", GetDistanceToPlayer());
            ImGui::Text("Player In Range: %s", IsPlayerInRange() ? "Yes" : "No");
            ImGui::Text("Looking at Player: %s", IsLookingAtPlayer() ? "Yes" : "No");
            
            // 회전 디버그 정보
            ImGui::Separator();
            ImGui::Text("=== Rotation Debug ===");
            engine::Vector3 dirToPlayer = CalculateDirectionToPlayer();
            // 엔진의 +Z Forward 방향 사용
            engine::Vector3 forward = GetForwardDirectionReverse();
            
            float dot = forward.Dot(dirToPlayer);
            float angleDeg = acosf(std::clamp(dot, -1.0f, 1.0f)) * 180.0f / 3.14159f;
            
            ImGui::Text("Forward (GetForward): (%.2f, %.2f, %.2f)", forward.x, forward.y, forward.z);
            ImGui::Text("To Player: (%.2f, %.2f, %.2f)", dirToPlayer.x, dirToPlayer.y, dirToPlayer.z);
            ImGui::Text("Dot Product: %.3f", dot);
            ImGui::Text("Angle to Player: %.1f deg", angleDeg);
            ImGui::Text("Threshold: 15.0 deg (stop), 25.0 deg (near-stop)");
            
            // 좌표계 정보
            ImGui::Separator();
            engine::Vector3 worldPos = GetTransform()->GetWorldPosition();
            engine::Vector3 playerPos = m_targetPlayer->GetGameObject()->GetTransform()->GetWorldPosition();
            ImGui::Text("Monster Pos: (%.1f, %.1f, %.1f)", worldPos.x, worldPos.y, worldPos.z);
            ImGui::Text("Player Pos: (%.1f, %.1f, %.1f)", playerPos.x, playerPos.y, playerPos.z);
            
            if (m_rigidbody)
            {
                ImGui::Separator();
                engine::Vector3 angVel = m_rigidbody->GetAngularVelocity();
                ImGui::Text("Angular Velocity: (%.3f, %.3f, %.3f)", angVel.x, angVel.y, angVel.z);
                
                // Cross product로 계산한 회전 방향 표시
                engine::Vector3 cross = forward.Cross(dirToPlayer);
                float rotationSign = (cross.y >= 0.0f) ? 1.0f : -1.0f;
                ImGui::Text("Cross Product Y: %.3f", cross.y);
                ImGui::Text("Rotation Sign: %.1f (%s)", rotationSign, 
                    rotationSign > 0 ? "CCW (Left)" : "CW (Right)");
                
                // 회전 상태 표시
                if (angleDeg <= 15.0f)
                {
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: STOPPED (Within 15deg threshold)");
                }
                else if (angleDeg <= 25.0f)
                {
                    if (std::abs(angVel.y) < 0.5f)
                    {
                        ImGui::TextColored(ImVec4(0, 1, 1, 1), "Status: NEAR-STOP (Slowing down)");
                    }
                    else
                    {
                        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Status: Rotating (Angle: %.1f deg)", angleDeg);
                    }
                }
                else
                {
                    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Status: ROTATING (Angle: %.1f deg)", angleDeg);
                }
                
                // 경고: 회전 방향이 잘못되었을 가능성
                if (std::abs(angVel.y) > 1.0f && angleDeg > 90.0f)
                {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "WARNING: High velocity but large angle!");
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "Check: Rotation direction or player movement");
                }
            }
        }
        ImGui::Text("Fire Timer: %.2f", m_fireTimer);
        
        ImGui::Unindent();
        
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Spacing();
    }

    void MonsterDullGray::Save(engine::json& j) const
    {
        MonsterScript::Save(j);
        
        j["AnimName_Idle"] = m_animName_Idle;
        j["AnimName_Attack"] = m_animName_Attack;
        j["AnimName_Fragile"] = m_animName_Fragile;
        j["AnimName_Dead"] = m_animName_Dead;
    }

    void MonsterDullGray::Load(const engine::json& j)
    {
        MonsterScript::Load(j);
        
        m_animName_Idle = j.value("AnimName_Idle", "Idle");
        m_animName_Attack = j.value("AnimName_Attack", "Attack");
        m_animName_Fragile = j.value("AnimName_Fragile", "Fragile");
        m_animName_Dead = j.value("AnimName_Dead", "Dead");
    }
}
