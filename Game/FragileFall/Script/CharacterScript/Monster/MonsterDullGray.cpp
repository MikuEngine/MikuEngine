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
        m_Hp = 100.0f;
        m_AttackRange = 15.0f;
        m_moveSpeed = 0.0f;  // 이동 불가
        m_rotationSpeed = 2.0f * 3.14159f;  // 360도/초 (라디안)
        m_fireRate = 3.0f;
        m_bulletSpeed = 1.0f;
        m_bulletLifetime = 3.0f;
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
        BulletParams params;
        params.type = BulletType::Linear;
        params.speed = m_bulletSpeed;
        params.lifetime = m_bulletLifetime;
        params.damage = 10;

        // BulletMonster를 생성하도록 BulletFactory 설정
        // BulletFactory는 Fire() 호출 시 적절한 총알을 생성
        
        // 현재 BulletFactory는 BulletPlayer만 생성하므로
        // BulletMonster를 생성하도록 수정이 필요하거나,
        // 별도의 BulletFactoryMonster를 사용해야 함
        
        // TODO: BulletFactory 확장 또는 별도 팩토리 사용
        // 현재는 MonsterScript::HandleShooting()에서 처리
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
        
        // 에디터 모드를 위한 실시간 컴포넌트 검색
        engine::Rigidbody* rigidbody = m_rigidbody ? m_rigidbody : (GetGameObject() ? GetGameObject()->GetComponent<engine::Rigidbody>() : nullptr);
        engine::SkeletalAnimator* skeletalAnimator = m_skeletalAnimator ? m_skeletalAnimator : (GetGameObject() ? GetGameObject()->GetComponent<engine::SkeletalAnimator>() : nullptr);
        engine::AnimFSM* animFSM = m_animFSM ? m_animFSM : (GetGameObject() ? GetGameObject()->GetComponent<engine::AnimFSM>() : nullptr);
        engine::LogicFSM* logicFSM = m_logicFSM ? m_logicFSM : (GetGameObject() ? GetGameObject()->GetComponent<engine::LogicFSM>() : nullptr);
        
        // BulletFactory는 자신의 GameObject에서만 검색
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
        ImGui::DragFloat("HP", &m_Hp, 1.0f, 0.0f, 1000.0f);
        ImGui::DragFloat("Attack Range", &m_AttackRange, 0.1f, 0.0f, 100.0f);

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
