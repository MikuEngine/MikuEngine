#include "GamePCH.h"
#include "MonsterDullType.h"

#include "Script/CharacterScript/Common/BulletFactory.h"

#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>

// 둔탁 타입은 StaticMesh를 사용하므로 SkeletalAnimator/SkeletalMeshRenderer include 불필요

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullType::Awake()
    {
        MonsterScript::Awake();
        
        // Dull 타입 고정
        m_attackType = AttackType::Dull;
        
        // 스탯은 씬 파일에서 로드됨 (Load 함수 참조)
    }

    void MonsterDullType::Start()
    {
        MonsterScript::Start();
        
        // 둔탁 타입은 StaticMesh를 사용하므로 애니메이션 관련 초기화 불필요
    }

    // ═══════════════════════════════════════════════════════════════
    // FSM 초기화
    // ═══════════════════════════════════════════════════════════════
    void MonsterDullType::InitializeFSM()
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
        // 둔탁 타입은 StaticMesh를 사용하므로 AnimFSM 초기화 불필요
    }

    void MonsterDullType::InitializeAnimations()
    {
        // 둔탁 타입은 StaticMesh를 사용하므로 애니메이션 초기화 불필요
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
            // ─────────────────────────────────────────────
            // 포물선 전용 파라미터 (동글녹색과 동일한 방식)
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
        case MonsterTier::Red:
            m_bulletParams.type = BulletType::Curve;
            m_bulletParams.speed = m_bulletSpeed;           // 나선형에서는 미사용
            m_bulletParams.lifetime = m_bulletLifetime;
            m_bulletParams.angularSpeed = m_curvedAngularSpeed;      // 에디터 설정값
            m_bulletParams.radiusGrowthRate = m_curvedRadiusGrowth;  // 에디터 설정값
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
            break;
        }
        // ─────────────────────────────────────────────
        // 둔탁 초록 (동글녹색과 동일한 포물선 발사 방식)
        // 
        // 고정된 위치에서 플레이어가 인식범위 접근 시 초록색 곡사포 투사체를 날림
        // - 에디터 설정: m_parabolicSpeed (속력), m_ownGravity (중력)
        // - 자동 계산: launchAngle (플레이어 거리 기반, m_useHighArc로 선택)
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
            
            // 거리 계산 (디버그용)
            float dx = targetPos.x - bulletStartPos.x;
            float dz = targetPos.z - bulletStartPos.z;
            float distance = std::sqrt(dx * dx + dz * dz);
            
            // 발사각 자동 계산 (m_useHighArc에 따라 높은/낮은 선택)
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
        // 둔탁 파랑
        // 
        // 고정된 위치에서 플레이어에게 3방향 투사체 발사
        // - m_spreadAngle: 좌우 퍼짐 각도 (인스펙터에서 설정 가능)
        // ─────────────────────────────────────────────
        case MonsterTier::Blue:
        {
            m_bulletFactory->ThreewayFireMonster(firePosition, direction, m_spreadAngle, m_bulletParams);
            break;
        }
        // ─────────────────────────────────────────────
        // 둔탁 빨강
        // 
        // 고정된 위치에서 4방향 나선형 탄환 발사
        // - 팩토리가 +X, -X, +Z, -Z 방향으로 4발 생성
        // - 각 탄환은 CurvedMovement로 나선 궤도 이동
        // - m_curvedAngularSpeed, m_curvedRadiusGrowth: 실시간 반영
        // ─────────────────────────────────────────────
        case MonsterTier::Red:
        {
            engine::Vector3 curvedFirePos = firePosition;
            curvedFirePos.y = 2.2f;  // Y 오프셋
            
            m_bulletFactory->CurvedFireMonster(curvedFirePos, m_curvedAngularSpeed, m_curvedRadiusGrowth, m_bulletParams);
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
            m_bulletFactory->LinearFireMonster(firePosition, direction, m_bulletParams);
            break;
        }

        // 둔탁 타입은 StaticMesh를 사용하므로 공격 애니메이션 재생 불필요

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
        // 둔탁 타입은 StaticMesh 사용 - SkeletalAnimator/AnimFSM 불필요
        // ─────────────────────────────────────────────
        ImGui::Separator();
        ImGui::Text("=== Component Validation ===");
        
        // 에디터 모드를 위한 실시간 컴포넌트 검색 (같은 GameObject 내에서만 검색)
        engine::Rigidbody* rigidbody = m_rigidbody ? m_rigidbody : (GetGameObject() ? GetGameObject()->GetComponent<engine::Rigidbody>() : nullptr);
        engine::LogicFSM* logicFSM = m_logicFSM ? m_logicFSM : (GetGameObject() ? GetGameObject()->GetComponent<engine::LogicFSM>() : nullptr);
        BulletFactory* bulletFactory = m_bulletFactory ? m_bulletFactory : (GetGameObject() ? GetGameObject()->GetComponent<BulletFactory>() : nullptr);
        
        // 전체 유효성 검사 (둔탁: Rigidbody, LogicFSM, BulletFactory만 필수)
        bool allValid = rigidbody && bulletFactory && logicFSM;
        
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
        ImGui::Unindent();

        // 공격 타입 (읽기 전용 - Dull 고정)
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
        ImGui::DragFloat("Attack Range", &m_AttackRange, 0.1f, 0.0f, 15.0f);

        // 설정 (둔탁은 이동하지 않으므로 Move Speed 제외)
        ImGui::Separator();
        ImGui::Text("Settings:");
        ImGui::DragFloat("Rotation Speed", &m_rotationSpeed, 0.1f, 0.0f, 10.0f);
        ImGui::DragFloat("Fire Rate (sec)", &m_fireRate, 0.1f, 0.1f, 10.0f);
        
        // Green이 아닐 때만 일반 Bullet Speed 표시
        if (m_monsterTier != MonsterTier::Green)
        {
            ImGui::DragFloat("Bullet Speed", &m_bulletSpeed, 0.1f, 0.1f, 100.0f);
        }
        ImGui::DragFloat("Bullet Lifetime", &m_bulletLifetime, 0.1f, 0.5f, 10.0f);

        // ─────────────────────────────────────────────
        // 포물선 설정 (Green일 때만 표시 - 동글녹색과 동일)
        // - 편집 가능: 속력, 자체 중력
        // - 읽기 전용: 발사각 (플레이어 거리 기반 자동 계산)
        // ─────────────────────────────────────────────
        if (m_monsterTier == MonsterTier::Green)
        {
            ImGui::Separator();
            ImGui::Text("=== Parabolic Bullet Settings ===");
            
            // 편집 가능한 설정
            ImGui::DragFloat("Bullet Speed", &m_parabolicSpeed, 0.5f, 1.0f, 50.0f, "%.1f m/s");
            ImGui::DragFloat("Own Gravity", &m_ownGravity, 0.1f, 1.0f, 30.0f, "%.1f m/s^2");
            
            // 최대 사거리 표시 (v² / g)
            float maxRange = (m_parabolicSpeed * m_parabolicSpeed) / m_ownGravity;
            ImGui::Text("Max Range (at 45 deg): %.1f m", maxRange);
            
            // 사거리 검증
            if (maxRange < m_AttackRange)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), 
                    "WARNING: Max Range < Attack Range!");
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), 
                    "Increase Speed or decrease Gravity/AttackRange");
            }
            else
            {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), 
                    "OK: Max Range >= Attack Range");
            }
            
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Runtime values (read-only):");
            
            // 읽기 전용: 마지막으로 계산된 값 표시
            ImGui::Text("  Launch Angle: %.1f deg", m_bulletParams.launchAngle);
            ImGui::Text("  Speed: %.1f m/s", m_bulletParams.speed);
            
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
                "(Angle calculated at fire time based on player distance)");
        }

        // ─────────────────────────────────────────────
        // 3방향 발사 설정 (Blue일 때만 표시)
        // ─────────────────────────────────────────────
        if (m_monsterTier == MonsterTier::Blue)
        {
            ImGui::Separator();
            ImGui::Text("=== Threeway Bullet Settings ===");
            
            ImGui::DragFloat("Spread Angle", &m_spreadAngle, 0.01f, 0.0f, 1.5f, "%.2f rad");
            
            // 각도(degree) 변환 표시
            float spreadDeg = m_spreadAngle * 180.0f / 3.14159265f;
            ImGui::Text("  = %.1f degrees", spreadDeg);
            
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
                "(Left: -%.2f rad, Center: 0, Right: +%.2f rad)", m_spreadAngle, m_spreadAngle);
        }

        // ─────────────────────────────────────────────
        // 나선형 발사 설정 (Red일 때만 표시)
        // ─────────────────────────────────────────────
        if (m_monsterTier == MonsterTier::Red)
        {
            ImGui::Separator();
            ImGui::Text("=== Curved (Spiral) Bullet Settings ===");
            
            ImGui::DragFloat("Angular Speed", &m_curvedAngularSpeed, 0.1f, 0.1f, 10.0f, "%.1f rad/s");
            ImGui::DragFloat("Radius Growth", &m_curvedRadiusGrowth, 0.1f, 0.1f, 20.0f, "%.1f m/s");
            
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Preview:");
            ImGui::Text("  Angular Speed: Higher = tighter spiral");
            ImGui::Text("  Radius Growth: Higher = faster expansion");
            
            // 1초 후 반지름 표시
            ImGui::Text("  Radius at 1s: %.1f m", m_curvedRadiusGrowth * 1.0f);
            ImGui::Text("  Radius at 2s: %.1f m", m_curvedRadiusGrowth * 2.0f);
        }

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
        // 둔탁 타입은 StaticMesh 사용 - 애니메이션 관련 직렬화 불필요
    }

    void MonsterDullType::Load(const engine::json& j)
    {
        MonsterScript::Load(j);
        // 둔탁 타입은 StaticMesh 사용 - 애니메이션 관련 직렬화 불필요
    }
}
