#include "GamePCH.h"
#include "MonsterDullType.h"

#include "Script/CharacterScript/Common/BulletFactory.h"

#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Object/Component/Renderer/SkeletalMeshRenderer.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Physics/PhysicsLayer.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullType::Awake()
    {
        MonsterScript::Awake();
        // 스탯은 씬 파일에서 로드됨 (Load 함수 참조)
    }

    void MonsterDullType::Start()
    {
        MonsterScript::Start();

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
    void MonsterDullType::InitializeFSM()
    {
        if (!m_logicFSM) return;

        LOG_PRINT("Initializing FSM. Current Tier: {}", (int)m_monsterTier);

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
		m_logicFSM->SetParameter("HasMonsters", true);      // 둔탁 보라 전용

        // ─────────────────────────────────────────────
        // 전이 정의 (StateMap이 업데이트된 후에 추가)
        // ─────────────────────────────────────────────
        // Idle ↔ Engage (플레이어 사거리 진입/이탈)
        AddFSMTransition("Idle", "Engage", "PlayerInRange", BoolTrue());
        AddFSMTransition("Engage", "Idle", "PlayerInRange", BoolFalse());

		// 둔탁 보라 전용 로직
        if (m_monsterTier == MonsterTier::Purple)
        {
            AddFSMTransition("Idle", "Dead", "Die", Trigger());
            AddFSMTransition("Engage", "Dead", "Die", Trigger());
        }
        else
        {
            // Any → Fragile (HP 0, Fragile 트리거)
            AddFSMTransition("Idle", "Fragile", "Fragile", Trigger());
            AddFSMTransition("Engage", "Fragile", "Fragile", Trigger());

            // Fragile → Dead (Execution, Die 트리거)
            AddFSMTransition("Fragile", "Dead", "Die", Trigger());
        }

        // ─────────────────────────────────────────────
        // 초기 상태 설정
        // ─────────────────────────────────────────────
        m_logicFSM->InitializeCurrentState();  // 기본 상태로 설정
    }

    void MonsterDullType::InitializeAnimFSM()
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
        m_animFSM->AddSplitState("Engage",  m_animName_Idle, true,  "", false, 0.0f, 0.1f);     // Engage는 Idle 재생
        m_animFSM->AddSplitState("Fragile", m_animName_Idle, true,  "", false, 0.0f, 0.1f);     // Fragile은 Idle 재생 (전용 애니메이션 설정 가능)
        m_animFSM->AddSplitState("Dead",    m_animName_Dead, false, "", false, 0.0f, 0.1f);     // Dead는 루프 안 함
 
    }

    void MonsterDullType::InitializeAnimations()
    {
        if (!m_skeletalAnimator) return;

        // SkeletalAnimator에 애니메이션 등록
        // 실제 .fbx 파일 경로는 에디터에서 설정하거나
        // 씬 파일에서 로드됨
        // 여기서는 이름만 연결
    }

    void MonsterDullType::InitializeBullet()
    {
        switch (m_monsterTier)
        {
        case MonsterTier::Gray:
		case MonsterTier::Blue:
		case MonsterTier::Purple:
            m_bulletParams.type = BulletType::Linear;
            m_bulletParams.speed = m_bulletSpeed;
            m_bulletParams.lifetime = m_bulletLifetime;
            m_bulletParams.damage = 10;
            break;
        case MonsterTier::Green:
            m_bulletParams.type = BulletType::Parabolic;
            m_bulletParams.gravity = 9.81f;
            m_bulletParams.lifetime = m_bulletLifetime;
            m_bulletParams.damage = 15;
            break;
        case MonsterTier::Red:
            m_bulletParams.type = BulletType::Curve;
            m_bulletParams.speed = m_bulletSpeed;
            m_bulletParams.lifetime = m_bulletLifetime;
            m_bulletParams.curveSpeed = 5.0f;
            m_bulletParams.damage = 20;
            m_rotationSpeed = 15.0f;
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
    // BaseControllerScript 오버라이드
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullType::UpdateGameLogic()
    {
        if (m_monsterTier == MonsterTier::Purple)
        {
            if (!m_isDead)
            {
                m_hasOtherMonstersAlive = CheckMonstersPurpleType();
                m_logicFSM->SetParameter("HasMonsters", m_hasOtherMonstersAlive);

                if (!m_hasOtherMonstersAlive)
                {
                    m_Hp = 0;
                    m_isDead = true;
                    m_logicFSM->SetTrigger("Die");
                }
            }
        }

        MonsterScript::UpdateGameLogic();
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태별 행동 - 비물리 (Update에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullType::UpdateStateBasedBehavior(const std::string& state, float deltaTime)
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

    void MonsterDullType::ExecuteEngageBehaviorNonPhysics(float deltaTime)
    {
        // 공격 타이머 및 발사 (비물리)
        Attack(deltaTime);
    }

    void MonsterDullType::ExecuteIdleBehaviorNonPhysics()
    {
        // 비물리 Idle 처리
    }

    void MonsterDullType::ExecuteFragileBehaviorNonPhysics()
    {
        // Fragile 상태: 아무 행동도 하지 않음 (Execution 대기)
    }

    void MonsterDullType::ExecuteDeadBehaviorNonPhysics()
    {
        // 부모 클래스의 Dead 타이머 처리 (2초 후 Destroy)
        MonsterScript::ExecuteDeadBehaviorNonPhysics();
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태별 행동 - 물리 (FixedUpdate에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullType::UpdatePhysicsStateBasedBehavior(const std::string& state)
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

    void MonsterDullType::ExecuteEngageBehaviorPhysics()
    {
        // 회전 (물리)
        RotateTowardsPlayer();
    }

    void MonsterDullType::ExecuteIdleBehaviorPhysics()
    {
        StopRotation();
    }

    void MonsterDullType::ExecuteFragileBehaviorPhysics()
    {
        // Fragile 상태: 물리적으로 정지
    }

    void MonsterDullType::ExecuteDeadBehaviorPhysics()
    {
        StopAllMovement();
    }

    bool MonsterDullType::CheckMonstersPurpleType()
    {
        const auto& gameObjects = engine::SceneManager::Get().GetScene()->GetGameObjects();

        for (const auto& go : gameObjects)
        {
            if (!go || go.get() == GetGameObject())
                continue;

            auto* monster = go->GetComponent<MonsterScript>();

            if (monster && !monster->m_isDead)
            {
                return true;
            }
        }

        return false; 
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태 진입 콜백
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullType::OnStateEntered(const std::string& state)
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

    void MonsterDullType::TakeDamage(float damage)
    {
        if (m_isDead)
            return;

        if (m_monsterTier == MonsterTier::Purple && m_hasOtherMonstersAlive)
            return;

        MonsterScript::TakeDamage(damage);
    }

    // ═══════════════════════════════════════════════════════════════
    // 공격
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullType::Attack(float deltaTime)
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

        engine::Vector3 direction = CalculateDirectionToPlayer();
        engine::Vector3 firePosition = GetTransform()->GetWorldPosition();

        switch (m_monsterTier)
        {
        // ─────────────────────────────────────────────
        // 둔탁 회색
        // 
        // 고정된 위치에서 플레이어가 인식범위 접근 시 투사체를 발사한다
        // ─────────────────────────────────────────────
        case MonsterTier::Gray:
        {
            m_bulletFactory->LinearFireMonster(firePosition, direction, m_bulletParams);
            m_fireTimer = m_fireRate;
            break;
        }
        // ─────────────────────────────────────────────
        // 둔탁 초록
        // 
        // 고정된 위치에서 플레이어가 인식범위 접근 시 초록색 곡사포 투사체를 날림
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
            float verticalSpeed = (dy + 0.5f * m_bulletParams.gravity * travelTime * travelTime) / travelTime;

            engine::Vector3 finalVelocity = (horizontalDir * horizontalSpeed) + (engine::Vector3::Up * verticalSpeed);
            float finalSpeed = finalVelocity.Length();
            finalVelocity.Normalize();

            m_bulletParams.speed = finalSpeed;
            m_bulletFactory->ParabolicFireMonster(startPos, finalVelocity, m_bulletParams);

            break;
        }
        // ─────────────────────────────────────────────
        // 둔탁 파랑
        // 
        // 고정된 위치에서 플레이어에게 투사체 3개를 날림
        // ─────────────────────────────────────────────
        case MonsterTier::Blue:
        {
			engine::Vector3 RotattedDirection1 = engine::Vector3::Transform(direction, engine::Matrix::CreateRotationY(0.2f));
			engine::Vector3 RotattedDirection2 = engine::Vector3::Transform(direction, engine::Matrix::CreateRotationY(-0.2f));

            m_bulletFactory->LinearFireMonster(firePosition, direction, m_bulletParams);
            m_bulletFactory->LinearFireMonster(firePosition, RotattedDirection1, m_bulletParams);
            m_bulletFactory->LinearFireMonster(firePosition, RotattedDirection2, m_bulletParams);

            break;
        }
        // ─────────────────────────────────────────────
        // 둔탁 빨강
        // 
        // 고정된 위치에서 4방향의 회전하는 회오리 공격을 날림
        // ─────────────────────────────────────────────
        case MonsterTier::Red:
        {
            for (int i = 0; i < 4; ++i)
            {
                float fireAngle = m_currentRotation + (DirectX::XM_PIDIV2 * i);

                engine::Vector3 fireDir;
                fireDir.x = cosf(fireAngle);
                fireDir.y = 0.0f;
                fireDir.z = sinf(fireAngle);
                fireDir.Normalize();

                m_bulletFactory->LinearFireMonster(firePosition, fireDir, m_bulletParams);
            }

            float rotationAmount = DirectX::XM_PI * 0.7f;
            m_targetRotation = m_currentRotation + rotationAmount;
            m_isRotating = true;
            break;
		}
        // ─────────────────────────────────────────────
        // 둔탁 보라
        // 
        // 다른 몬스터 모두 처치하기 전까지 무적으로 다른 몬스터 처치시 자동으로 사망하며 그전 까지는 둔탁 회색패턴으로 계속해서 공격
        // ─────────────────────────────────────────────
        case MonsterTier::Purple:
        {
            m_bulletFactory->LinearFireMonster(firePosition, direction, m_bulletParams);
            break;
		}
        default:
            m_fireTimer = m_fireRate;
            break;
        }

        if (m_skeletalAnimator && !m_animName_Attack.empty())
        {
            m_skeletalAnimator->Play(m_animName_Attack, false, 0, 1.0f);
        }

        m_fireTimer = m_fireRate;
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullType::OnGui()
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

    void MonsterDullType::Save(engine::json& j) const
    {
        MonsterScript::Save(j);
        
        j["AnimName_Idle"] = m_animName_Idle;
        j["AnimName_Attack"] = m_animName_Attack;
        j["AnimName_Fragile"] = m_animName_Fragile;
        j["AnimName_Dead"] = m_animName_Dead;
    }

    void MonsterDullType::Load(const engine::json& j)
    {
        MonsterScript::Load(j);
        
        m_animName_Idle = j.value("AnimName_Idle", "Idle");
        m_animName_Attack = j.value("AnimName_Attack", "Attack");
        m_animName_Fragile = j.value("AnimName_Fragile", "Fragile");
        m_animName_Dead = j.value("AnimName_Dead", "Dead");
    }
}
