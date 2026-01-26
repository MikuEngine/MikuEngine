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

        // DullGray 고유 스탯 설정
        m_Hp = 10;
        m_AttackRange = 15.0f;
        m_moveSpeed = 0.0f;  // 이동 불가
        m_rotationSpeed = 3.0f * 3.14159f;  // 360도/초 (라디안)
        m_fireRate = 3.0f;
        m_bulletSpeed = 1.2f;
        m_bulletLifetime = 3.0f;
    }

    void MonsterDullGray::Start()
    {
        MonsterScript::Start();

        // ─────────────────────────────────────────────
        // DullGray 고유 Rigidbody Constraints 설정
        // X, Y, Z 이동 모두 불가, X, Z 회전 불가 (Y축 회전만 허용)
        // ─────────────────────────────────────────────
        if (m_rigidbody)
        {
            using namespace engine;
            RigidbodyConstraints constraints = 
                RigidbodyConstraints::FreezePositionX |
                RigidbodyConstraints::FreezePositionY |
                RigidbodyConstraints::FreezePositionZ |
                RigidbodyConstraints::FreezeRotationX |
                RigidbodyConstraints::FreezeRotationZ;
            
            m_rigidbody->SetConstraints(constraints);
            
            LOG_PRINT("[MonsterDullGray] Rigidbody constraints set: All position frozen, Y-rotation only");
        }

        // 초기 Idle 애니메이션 재생
        if (m_skeletalAnimator && !m_animName_Idle.empty())
        {
            m_skeletalAnimator->Play(m_animName_Idle, true, 0, 1.0f);
        }
        
        // ─────────────────────────────────────────────
        // 초기화 검증 로그
        // ─────────────────────────────────────────────
        LOG_PRINT("[MonsterDullGray] Initialization Complete:");
        LOG_PRINT("  - Rigidbody: {}", m_rigidbody ? "OK" : "MISSING");
        LOG_PRINT("  - SkeletalAnimator: {}", m_skeletalAnimator ? "OK" : "MISSING");
        LOG_PRINT("  - BulletFactory: {}", m_bulletFactory ? "OK" : "MISSING");
        LOG_PRINT("  - AnimFSM: {}", m_animFSM ? "OK" : "MISSING");
        LOG_PRINT("  - LogicFSM: {}", m_logicFSM ? "OK" : "MISSING");
        LOG_PRINT("  - Target Player: {}", m_targetPlayer ? "Found" : "NOT FOUND");
        LOG_PRINT("  - Current State: {}", GetCurrentState());
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
        AddFSMState("Idle", true);   // 기본 상태
        AddFSMState("Engage", false);
        AddFSMState("Dead", false);

        // ─────────────────────────────────────────────
        // StateMap 업데이트 (Transition 추가 전에 필수!)
        // ─────────────────────────────────────────────
        m_logicFSM->Initialize();  // UpdateStateMap() 호출

        // ─────────────────────────────────────────────
        // 파라미터 정의
        // ─────────────────────────────────────────────
        m_logicFSM->SetParameter("PlayerInRange", m_isPlayerInRange);
        m_logicFSM->SetParameter("Die", m_isDead);

        // ─────────────────────────────────────────────
        // 전이 정의 (StateMap이 업데이트된 후에 추가)
        // ─────────────────────────────────────────────
        // Idle ↔ Engage (플레이어 사거리 진입/이탈)
        AddFSMTransition("Idle", "Engage", "PlayerInRange", BoolTrue());
        AddFSMTransition("Engage", "Idle", "PlayerInRange", BoolFalse());

        // Any → Dead (Die 트리거)
        AddFSMTransition("Idle", "Dead", "Die", Trigger());
        AddFSMTransition("Engage", "Dead", "Die", Trigger());

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
        m_animFSM->AddSplitState("Idle",   m_animName_Idle, true,  "", false, 0.0f, 0.1f);
        m_animFSM->AddSplitState("Engage", m_animName_Idle, true,  "", false, 0.0f, 0.1f);  // Engage는 Idle 재생
        m_animFSM->AddSplitState("Dead",   m_animName_Dead, false, "", false, 0.0f, 0.1f);  // Dead는 루프 안 함
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
        // 총알 파라미터는 Attack()에서 직접 설정
    }

    // ═══════════════════════════════════════════════════════════════
    // 공격
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullGray::Attack(float deltaTime)
    {
        // 쿨다운 타이머 감소
        if (m_fireTimer > 0.0f)
        {
            m_fireTimer -= deltaTime;
            return;  // 쿨다운 중에는 발사하지 않음
        }

        // 발사 조건: 쿨다운 완료 (회전 완료 대기 안 함, 플레이어 방향으로 발사)
        if (m_bulletFactory && m_targetPlayer && m_targetPlayer->GetGameObject())
        {
            engine::Vector3 monsterPos = GetTransform()->GetWorldPosition();
            engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();
            
            // ─────────────────────────────────────────────
            // 플레이어 방향 계산 (직접 계산)
            // 매 프레임 플레이어를 향해 회전 중이므로 얼추 맞음
            // ─────────────────────────────────────────────
            engine::Vector3 direction = playerPos - monsterPos;
            direction.y = 0.0f;  // Y축 무시 (수평 방향만)
            
            if (direction.LengthSquared() > 0.001f)
            {
                direction.Normalize();

                // 리니어 총알 발사
                BulletParams params;
                params.type = BulletType::Linear;
                params.speed = m_bulletSpeed;      // 1.0
                params.lifetime = m_bulletLifetime; // 3.0
                params.damage = 10;

                LOG_PRINT("[MonsterDullGray] Firing bullet towards player ({:.2f}, {:.2f}, {:.2f})", 
                          direction.x, direction.y, direction.z);

                m_bulletFactory->LinearFireMonster(monsterPos, direction, params);

                // 발사 애니메이션 재생 (루프 없음)
                if (m_skeletalAnimator && !m_animName_Attack.empty())
                {
                    m_skeletalAnimator->Play(m_animName_Attack, false, 0, 1.0f);
                }

                // 쿨다운 재설정 (3초)
                m_fireTimer = m_fireRate;  // 3.0초
            }
        }
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
            ImGui::Text("Rotated to Player: %s", IsRotatedTowardsPlayer() ? "Yes" : "No");
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
        j["AnimName_Dead"] = m_animName_Dead;
    }

    void MonsterDullGray::Load(const engine::json& j)
    {
        MonsterScript::Load(j);
        
        m_animName_Idle = j.value("AnimName_Idle", "Idle");
        m_animName_Attack = j.value("AnimName_Attack", "Attack");
        m_animName_Dead = j.value("AnimName_Dead", "Dead");
    }
}
