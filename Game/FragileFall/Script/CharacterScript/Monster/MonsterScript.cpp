#include "GamePCH.h"
#include "MonsterScript.h"

#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Framework/System/SystemManager.h"

#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/System/PathfindingSystem.h>
#include <Engine/Core/System/MyTime.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void MonsterScript::Awake()
    {
        BaseControllerScript::Awake();
    }

    void MonsterScript::Start()
    {
        BaseControllerScript::Start();

        // 애니메이션 초기화
        InitializeAnimations();

        // 총알 설정 초기화
        InitializeBullet();

        // LogicFSM 초기화
        if (!m_fsmInitialized && m_logicFSM)
        {
            InitializeFSM();
            m_fsmInitialized = true;
        }

        // AnimFSM 초기화
        if (m_animFSM)
        {
            InitializeAnimFSM();
        }

        // Rigidbody 필수 확인
        if (!m_rigidbody)
        {
            LOG_PRINT("[MonsterScript] ERROR: Monster requires Dynamic Rigidbody!");
            return;
        }

        // ─────────────────────────────────────────────
        // Mass 강제 설정 (플레이어와의 충돌 시 밀림 방지)
        // Scene 파일의 값보다 우선 적용됨
        // - 이동하지 않는 몬스터 (moveSpeed == 0): 1500
        // - 이동하는 몬스터: 100
        // ─────────────────────────────────────────────
        float mass = (m_moveSpeed <= 0.0f) ? 15000.0f : 500.0f;
        m_rigidbody->SetMass(mass);
        m_rigidbody->SetAngularDamping(55.0f);
        m_rigidbody->SetLinearDamping(10.0f);

        // Rigidbody 제약 설정 (몬스터는 Y축 회전만 허용)
        using namespace engine;
        RigidbodyConstraints constraints = 
            RigidbodyConstraints::FreezeRotationX | 
            RigidbodyConstraints::FreezeRotationZ;
        
        // 이동하지 않는 몬스터는 Y축 위치도 고정
        if (m_moveSpeed <= 0.0f)
        {
            constraints = constraints | RigidbodyConstraints::FreezePositionY;
        }
        
        m_rigidbody->SetConstraints(constraints);
        
        // Dynamic Rigidbody 설정 확인
        if (!m_rigidbody->IsDynamic())
        {
            LOG_PRINT("[MonsterScript] WARNING: Monster Rigidbody should be Dynamic!");
        }

        // 플레이어 찾기
        FindPlayer();
    }

    // ═══════════════════════════════════════════════════════════════
    // 컴포넌트 캐싱
    // ═══════════════════════════════════════════════════════════════
    void MonsterScript::CacheComponents()
    {
        BaseControllerScript::CacheComponents();

        if (!GetGameObject()) return;

        m_rigidbody = GetGameObject()->GetComponent<engine::Rigidbody>();
        m_skeletalAnimator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();

        // BulletFactory는 자신의 GameObject에서만 검색
        m_bulletFactory = GetGameObject()->GetComponent<BulletFactory>();
        
        // PathfindingSystem 검색 (Scene의 시스템)
        auto* scene = engine::SceneManager::Get().GetScene();
        if (scene)
        {
            m_pathfindingSystem = &engine::SystemManager::Get().GetPathfindingSystem();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 플레이어 추적
    // ═══════════════════════════════════════════════════════════════
    void MonsterScript::FindPlayer()
    {
        auto* scene = engine::SceneManager::Get().GetScene();
        if (!scene) return;

        // 1. "Player" 이름으로 검색
        if (auto* playerGO = scene->FindGameObject(m_targetPlayerObjectName))
        {
            m_targetPlayer = playerGO->GetComponent<PlayerControllerScript>();
            if (m_targetPlayer) return;
        }

        // 2. PlayerControllerScript 컴포넌트로 검색
        if (!m_targetPlayer)
        {
            for (const auto& go : scene->GetGameObjects())
            {
                if (auto* player = go->GetComponent<PlayerControllerScript>())
                {
                    m_targetPlayer = player;
                    return;
                }
            }
        }
    }

    float MonsterScript::GetDistanceToPlayer() const
    {
        if (!m_targetPlayer || !m_targetPlayer->GetGameObject()) return FLT_MAX;

        engine::Vector3 monsterPos = GetTransform()->GetWorldPosition();
        engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();
        
        return (playerPos - monsterPos).Length();
    }

    float MonsterScript::GetPathDistanceToPlayer() const
    {
        // PathfindingSystem을 사용한 실제 경로 거리 계산
        // 향후 이동하는 몬스터에서 장애물을 고려한 실제 거리가 필요할 때 사용
        if (!m_pathfindingSystem || !m_targetPlayer || !m_targetPlayer->GetGameObject())
        {
            return FLT_MAX;
        }

        engine::Vector3 monsterPos = GetTransform()->GetWorldPosition();
        engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();
        
        // PathfindingSystem으로 경로 찾기
        engine::PathResult pathResult = m_pathfindingSystem->FindPath(monsterPos, playerPos);
        
        if (pathResult.success)
        {
            return pathResult.totalDistance;
        }
        
        // 경로를 찾지 못하면 직선 거리 반환
        return GetDistanceToPlayer();
    }

    bool MonsterScript::IsPlayerInRange() const
    {
        // 현재는 직선 거리만 사용
        // 향후 이동 몬스터는 GetPathDistanceToPlayer()를 사용할 수 있음
        return GetDistanceToPlayer() <= m_AttackRange;
    }

    engine::Vector3 MonsterScript::CalculateDirectionToPlayer() const
    {
        if (!m_targetPlayer || !m_targetPlayer->GetGameObject())
        {
            return engine::Vector3::Zero;
        }

        engine::Vector3 monsterPos = GetTransform()->GetWorldPosition();
        engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();

        // 방향 벡터 계산 (Y축 무시)
        engine::Vector3 direction = playerPos - monsterPos;
        direction.y = 0.0f;
        
        if (direction.LengthSquared() < 0.001f)
        {
            return engine::Vector3::Zero;
        }
        
        direction.Normalize();
        return direction;
    }

    // ═══════════════════════════════════════════════════════════════
    // 회전 및 발사
    // ═══════════════════════════════════════════════════════════════
    void MonsterScript::RotateTowardsPlayer(float deltaTime)
    {
        // CalculateDirectionToPlayer()를 사용하여 방향 계산
        engine::Vector3 direction = CalculateDirectionToPlayer();
        
        if (direction.LengthSquared() < 0.001f) return;
        
        RotateTowards(direction, deltaTime);
    }

    void MonsterScript::RotateTowards(const engine::Vector3& targetDirection, float deltaTime)
    {
        if (!m_rigidbody)
        {
            LOG_PRINT("[MonsterScript] ERROR: Rigidbody is required for monster rotation!");
            return;
        }

        if (targetDirection.LengthSquared() < 0.001f)
        {
            // 목표 방향 없음 - 회전 멈춤
            m_rigidbody->SetAngularVelocity(engine::Vector3::Zero);
            return;
        }

        // ─────────────────────────────────────────────
        // 1. 현재 각도 (Transform에서 읽음)
        // ─────────────────────────────────────────────
        engine::Quaternion currentRotation = GetTransform()->GetWorldRotation();
        engine::Vector3 currentEuler = currentRotation.ToEuler();
        float currentAngle = currentEuler.y;

        // ─────────────────────────────────────────────
        // 2. 목표 각도 계산 (앞뒤 반대 수정: +180도)
        // ─────────────────────────────────────────────
        float targetAngle = atan2f(targetDirection.x, targetDirection.z) + 3.14159f;  // +180도 (앞뒤 반전)
        
        // 각도 정규화 (-π ~ π)
        while (targetAngle > 3.14159f) targetAngle -= 6.28318f;
        while (targetAngle < -3.14159f) targetAngle += 6.28318f;

        // ─────────────────────────────────────────────
        // 3. 각도 차이 계산 (정규화: -π ~ π)
        // ─────────────────────────────────────────────
        float angleDiff = targetAngle - currentAngle;
        while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
        while (angleDiff < -3.14159f) angleDiff += 6.28318f;

        // ─────────────────────────────────────────────
        // 4. PD 컨트롤러로 각속도 계산 (진동 방지)
        // ─────────────────────────────────────────────
        
        // 목표에 거의 도달하면 회전 멈춤
        if (std::abs(angleDiff) < ROTATION_THRESHOLD)
        {
            m_rigidbody->SetAngularVelocity(engine::Vector3::Zero);
            return;
        }

        // Proportional: 각도 차이에 비례
        float proportional = angleDiff * m_rotationSpeed;
        
        // Derivative: 현재 각속도 (damping 역할)
        engine::Vector3 currentAngVel = m_rigidbody->GetAngularVelocity();
        float derivative = -currentAngVel.y * 0.5f;  // Damping factor
        
        // PD 제어
        float angularVelocity = proportional + derivative;
        
        // 최대 각속도 제한
        const float maxAngularSpeed = 5.0f;  // rad/sec (진동 방지를 위해 감소)
        if (std::abs(angularVelocity) > maxAngularSpeed)
        {
            angularVelocity = (angularVelocity > 0) ? maxAngularSpeed : -maxAngularSpeed;
        }

        // Y축 각속도 설정
        m_rigidbody->SetAngularVelocity(engine::Vector3(0.0f, angularVelocity, 0.0f));

        // 디버깅 로그
        static int logCounter = 0;
        if (logCounter++ % 60 == 0)  // 60프레임마다 1번
        {
            LOG_PRINT("[MonsterScript] Rotate: current={:.1f}°, target={:.1f}°, diff={:.1f}°, angVel={:.2f}",
                currentAngle * 180.0f / 3.14159f,
                targetAngle * 180.0f / 3.14159f,
                angleDiff * 180.0f / 3.14159f,
                angularVelocity);
        }
    }

    bool MonsterScript::IsRotatedTowardsPlayer() const
    {
        if (!m_targetPlayer || !m_targetPlayer->GetGameObject()) return false;

        engine::Vector3 monsterPos = GetTransform()->GetWorldPosition();
        engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();

        // 방향 벡터 계산 (Y축 무시)
        engine::Vector3 direction = playerPos - monsterPos;
        direction.y = 0.0f;
        
        if (direction.LengthSquared() < 0.001f) return true;
        
        direction.Normalize();

        // ─────────────────────────────────────────────
        // 현재 회전값 (Transform에서 직접 읽음)
        // ─────────────────────────────────────────────
        engine::Quaternion currentRotation = GetTransform()->GetWorldRotation();
        engine::Vector3 currentEuler = currentRotation.ToEuler();
        float currentAngle = currentEuler.y;

        // 목표 각도 계산 (앞뒤 반전: +180도)
        float targetAngle = atan2f(direction.x, direction.z) + 3.14159f;
        
        // 각도 정규화 (-π ~ π)
        while (targetAngle > 3.14159f) targetAngle -= 6.28318f;
        while (targetAngle < -3.14159f) targetAngle += 6.28318f;

        // 각도 차이 계산
        float angleDiff = targetAngle - currentAngle;

        // 각도 정규화
        while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
        while (angleDiff < -3.14159f) angleDiff += 6.28318f;

        // 임계값 이하인지 확인
        return std::abs(angleDiff) <= ROTATION_THRESHOLD;
    }

    void MonsterScript::Attack(float deltaTime)
    {
        // 기본 구현은 비어있음
        // 자손 클래스에서 오버라이드하여 구현
    }

    // ═══════════════════════════════════════════════════════════════
    // 이동 (PathfindingSystem 활용, 향후 이동 몬스터용)
    // ═══════════════════════════════════════════════════════════════
    void MonsterScript::MoveTowardsPlayer(float deltaTime)
    {
        // PathfindingSystem을 사용하여 플레이어를 향해 이동
        // 현재 DullGray는 이동하지 않지만, 향후 이동하는 몬스터에서 사용
        
        if (!m_pathfindingSystem || !m_rigidbody || !m_targetPlayer)
        {
            return;
        }

        if (m_moveSpeed <= 0.0f)
        {
            return;  // 이동 속도가 0이면 이동하지 않음 (DullGray)
        }

        engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
        engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();

        // PathfindingSystem으로 경로 찾기
        engine::PathResult pathResult = m_pathfindingSystem->FindPath(currentPos, playerPos);

        if (!pathResult.success || pathResult.path.empty())
        {
            return;  // 경로를 찾지 못함
        }

        // 다음 웨이포인트로 이동
        // path[0]는 현재 위치이므로, path[1]이 다음 목표 지점
        size_t nextWaypointIndex = pathResult.path.size() > 1 ? 1 : 0;
        engine::Vector3 nextWaypoint = pathResult.path[nextWaypointIndex];

        // 다음 웨이포인트 방향 계산
        engine::Vector3 direction = nextWaypoint - currentPos;
        direction.y = 0.0f;  // Y축 무시

        if (direction.LengthSquared() < 0.001f)
        {
            return;  // 이미 웨이포인트에 도착
        }

        direction.Normalize();

        // 방향으로 회전
        RotateTowards(direction, deltaTime);

        // Rigidbody를 통한 이동 (물리 시뮬레이션 활용)
        engine::Vector3 velocity = direction * m_moveSpeed;
        m_rigidbody->SetLinearVelocity(velocity);
    }

    // ═══════════════════════════════════════════════════════════════
    // 입력 처리 (FSM 파라미터 설정)
    // ═══════════════════════════════════════════════════════════════
    void MonsterScript::ProcessInput()
    {
        if (!m_logicFSM) return;

        // 플레이어를 찾지 못했으면 재탐색
        if (!m_targetPlayer)
        {
            FindPlayer();
        }

        // 플레이어와의 거리 체크
        m_isPlayerInRange = IsPlayerInRange();

        // FSM 파라미터 업데이트
        m_logicFSM->SetParameter("PlayerInRange", m_isPlayerInRange);
    }

    // ═══════════════════════════════════════════════════════════════
    // 게임 로직 업데이트
    // ═══════════════════════════════════════════════════════════════
    void MonsterScript::UpdateGameLogic()
    {
        float deltaTime = engine::Time::DeltaTime();

        // 체력 체크
        m_isDead = CheckDeath();

        // 현재 상태 확인
        std::string currentState = GetCurrentState();

        // 디버깅: 상태 변경 로그
        static std::string lastState = "";
        if (currentState != lastState)
        {
            LOG_PRINT("[MonsterScript] State changed: {} -> {}", lastState, currentState);
            lastState = currentState;
            
            // Engage 상태가 아니면 회전 멈춤
            if (currentState != "Engage" && m_rigidbody)
            {
                m_rigidbody->SetAngularVelocity(engine::Vector3::Zero);
            }
        }

        // Engage 상태: 플레이어 방향 회전 + 공격
        if (currentState == "Engage")
        {
            RotateTowardsPlayer(deltaTime);
            Attack(deltaTime);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태 진입 콜백
    // ═══════════════════════════════════════════════════════════════
    void MonsterScript::OnStateEntered(const std::string& state)
    {
        if (state == "Dead")
        {
            // 죽음: 모든 물리 동작 정지
            if (m_rigidbody)
            {
                m_rigidbody->SetAngularVelocity(engine::Vector3::Zero);
                m_rigidbody->SetLinearVelocity(engine::Vector3::Zero);
            }
            OnDeath();
        }
        else if (state == "Idle")
        {
            // Idle: 회전 정지
            if (m_rigidbody)
            {
                m_rigidbody->SetAngularVelocity(engine::Vector3::Zero);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 행동 제한
    // ═══════════════════════════════════════════════════════════════
    bool MonsterScript::CanMove() const
    {
        std::string state = GetCurrentState();
        return state != "Dead";
    }

    bool MonsterScript::CanAttack() const
    {
        std::string state = GetCurrentState();
        return state == "Engage" && state != "Dead";
    }

    // ═══════════════════════════════════════════════════════════════
    // 체력 관리
    // ═══════════════════════════════════════════════════════════════
    bool MonsterScript::CheckDeath()
    {
        if (m_Hp <= 0.0f && GetCurrentState() != "Dead")
        {
            if (m_logicFSM)
            {
                m_logicFSM->SetTrigger("Die");
                return true;
            }
        }

        return false;
    }

    void MonsterScript::OnDeath()
    {
        // 속도 정지
        if (m_rigidbody)
        {
            m_rigidbody->SetLinearVelocity(engine::Vector3::Zero);
        }

        // TODO: Dead 애니메이션 재생 후 일정 시간 후 파괴
        // float deathAnimDuration = 2.0f;
        // GetGameObject()->Destroy(deathAnimDuration);
    }

    // ═══════════════════════════════════════════════════════════════
    // 컴포넌트 검증
    // ═══════════════════════════════════════════════════════════════
    bool MonsterScript::ValidateComponents() const
    {
        bool isValid = true;

        if (!m_rigidbody) isValid = false;
        if (!m_skeletalAnimator) isValid = false;
        if (!m_bulletFactory) isValid = false;
        if (!m_animFSM) isValid = false;
        if (!m_logicFSM) isValid = false;

        return isValid;
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void MonsterScript::OnGui()
    {
        ImGui::Indent();
        
        ImGui::Text("MonsterScript:");

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
        ImGui::DragFloat("Attack Range", &m_AttackRange, 0.1f, 0.1f, 15.0f);

        // 설정
        ImGui::Separator();
        ImGui::Text("Settings:");
        ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Rotation Speed", &m_rotationSpeed, 0.1f, 0.0f, 10.0f);
        ImGui::DragFloat("Fire Rate (sec)", &m_fireRate, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Bullet Speed", &m_bulletSpeed, 0.1f, 0.1f, 100.0f);
        ImGui::DragFloat("Bullet Lifetime", &m_bulletLifetime, 0.1f, 0.5f, 10.0f);

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

    void MonsterScript::Save(engine::json& j) const
    {
        BaseControllerScript::Save(j);
        
        j["Hp"] = m_Hp;
        j["AttackRange"] = m_AttackRange;
        j["MoveSpeed"] = m_moveSpeed;
        j["RotationSpeed"] = m_rotationSpeed;
        j["FireRate"] = m_fireRate;
        j["BulletSpeed"] = m_bulletSpeed;
        j["BulletLifetime"] = m_bulletLifetime;
    }

    void MonsterScript::Load(const engine::json& j)
    {
        BaseControllerScript::Load(j);
        
        m_Hp = j.value("Hp", 100);
        m_AttackRange = j.value("AttackRange", 10.0f);
        m_moveSpeed = j.value("MoveSpeed", 0.0f);
        m_rotationSpeed = j.value("RotationSpeed", 2.0f);
        m_fireRate = j.value("FireRate", 3.0f);
        m_bulletSpeed = j.value("BulletSpeed", 1.0f);
        m_bulletLifetime = j.value("BulletLifetime", 3.0f);
    }
}
