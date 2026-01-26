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

        // 현재 회전 각도 초기화
        if (GetTransform())
        {
            engine::Quaternion currentRotation = GetTransform()->GetLocalRotation();
            engine::Vector3 euler = currentRotation.ToEuler();
            m_currentRotationAngle = euler.y;  // Y축 회전값 (라디안)
            LOG_PRINT("[MonsterScript] Initial rotation: Y={:.2f}° ({:.2f} rad)", 
                euler.y * 180.0f / 3.14159f, euler.y);
        }

        // Rigidbody 제약 설정 (몬스터는 Y축 회전만 허용)
        // Start()에서 설정하여 m_moveSpeed가 자식 클래스에서 설정된 후 적용
        if (m_rigidbody)
        {
            // X, Z축 회전 고정 (Y축만 스크립트로 제어)
            using namespace engine;
            RigidbodyConstraints constraints = 
                RigidbodyConstraints::FreezeRotationX | 
                RigidbodyConstraints::FreezeRotationZ;
            
            //// 이동하지 않는 몬스터는 Y축 위치도 고정
            //if (m_moveSpeed <= 0.0f)
            //{
            //    constraints = constraints | RigidbodyConstraints::FreezePositionY;
            //}
            
            m_rigidbody->SetConstraints(constraints);
            
            LOG_PRINT("[MonsterScript] Rigidbody constraints set: FreezeRotationX|Z, FreezePositionY={}", 
                m_moveSpeed <= 0.0f);
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
        
        // 디버깅: 정규화된 방향 벡터 출력 (처음 몇 번만)
        static int dirLogCount = 0;
        if (dirLogCount < 3)
        {
            LOG_PRINT("[MonsterScript] Direction: monster({:.2f}, {:.2f}, {:.2f}), player({:.2f}, {:.2f}, {:.2f}), dir_normalized({:.2f}, {:.2f}, {:.2f})",
                monsterPos.x, monsterPos.y, monsterPos.z,
                playerPos.x, playerPos.y, playerPos.z,
                direction.x, direction.y, direction.z);
            dirLogCount++;
        }
        
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
        if (targetDirection.LengthSquared() < 0.001f)
        {
            LOG_PRINT("[MonsterScript] RotateTowards: Target direction is too small, skipping rotation");
            return;
        }

        // 목표 각도 계산 (라디안)
        float targetAngle = atan2f(targetDirection.x, targetDirection.z);

        // 현재 각도와의 차이 계산
        float angleDiff = targetAngle - m_currentRotationAngle;

        // 각도 정규화 (-π ~ π)
        while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
        while (angleDiff < -3.14159f) angleDiff += 6.28318f;

        // 회전 속도만큼 회전
        float maxRotation = m_rotationSpeed * deltaTime;
        
        // 디버깅 로그 (처음 몇 프레임만)
        static int logCount = 0;
        if (logCount < 5)
        {
            LOG_PRINT("[MonsterScript] Rotate: current={:.2f}°, target={:.2f}°, diff={:.2f}°, maxRot={:.2f}°", 
                m_currentRotationAngle * 180.0f / 3.14159f,
                targetAngle * 180.0f / 3.14159f,
                angleDiff * 180.0f / 3.14159f,
                maxRotation * 180.0f / 3.14159f);
            logCount++;
        }
        
        if (std::abs(angleDiff) < maxRotation)
        {
            m_currentRotationAngle = targetAngle;
        }
        else
        {
            m_currentRotationAngle += (angleDiff > 0 ? maxRotation : -maxRotation);
        }

        // Transform 회전 적용
        engine::Quaternion rotation = engine::Quaternion::CreateFromAxisAngle(
            engine::Vector3::UnitY,
            m_currentRotationAngle
        );
        
        GetTransform()->SetLocalRotation(rotation);
        
        // 디버깅: 회전 적용 확인 (처음 몇 번만)
        static int rotLogCount = 0;
        if (rotLogCount < 3)
        {
            engine::Quaternion appliedRot = GetTransform()->GetLocalRotation();
            engine::Vector3 appliedEuler = appliedRot.ToEuler();
            
            LOG_PRINT("[MonsterScript] Rotation applied: Y={:.2f}° (target={:.2f}°)",
                appliedEuler.y * 180.0f / 3.14159f,
                m_currentRotationAngle * 180.0f / 3.14159f);
            rotLogCount++;
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

        // 목표 각도 계산
        float targetAngle = atan2f(direction.x, direction.z);

        // 각도 차이 계산
        float angleDiff = targetAngle - m_currentRotationAngle;

        // 각도 정규화
        while (angleDiff > 3.14159f) angleDiff -= 6.28318f;
        while (angleDiff < -3.14159f) angleDiff += 6.28318f;

        // 임계값 이하인지 확인
        return std::abs(angleDiff) <= ROTATION_THRESHOLD;
    }

    void MonsterScript::HandleShooting(float deltaTime)
    {
        engine::Vector3 monsterPos = GetTransform()->GetWorldPosition();
        engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();
        engine::Vector3 direction = CalculateDirectionToPlayer();
        LOG_PRINT("[MonsterScript] Direction: monster({:.2f}, {:.2f}, {:.2f}), player({:.2f}, {:.2f}, {:.2f}), dir_normalized({:.2f}, {:.2f}, {:.2f})",
            monsterPos.x, monsterPos.y, monsterPos.z,
            playerPos.x, playerPos.y, playerPos.z,
            direction.x, direction.y, direction.z);

        // 쿨다운 타이머 감소
        if (m_fireTimer > 0.0f)
        {
            m_fireTimer -= deltaTime;
        }

        // 발사 조건: 회전 완료 + 쿨다운 완료
        if (IsRotatedTowardsPlayer() && m_fireTimer <= 0.0f)
        {
            // 발사!
            if (m_bulletFactory && m_targetPlayer && m_targetPlayer->GetGameObject())
            {
                engine::Vector3 monsterPos = GetTransform()->GetWorldPosition();
                engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();

                // 방향 벡터 계산
                engine::Vector3 direction = playerPos - monsterPos;
                direction.y = 0.0f;
                
                if (direction.LengthSquared() > 0.001f)
                {
                    direction.Normalize();

                    // BulletParams 설정 (자식 클래스에서 InitializeBullet으로 설정 가능)
                    BulletParams params;
                    params.type = BulletType::Linear;
                    params.speed = m_bulletSpeed;
                    params.lifetime = m_bulletLifetime;
                    params.damage = 10;

                    m_bulletFactory->FireMonster(monsterPos, direction, params);

                    // 발사 애니메이션 재생 (루프 없음)
                    // SkeletalAnimator로 직접 제어하여 Attack 애니메이션을 한 번만 재생
                    if (m_skeletalAnimator && !m_animName_Attack.empty())
                    {
                        m_skeletalAnimator->Play(m_animName_Attack, false, 0, 1.0f);
                    }

                    // 쿨다운 재설정
                    m_fireTimer = m_fireRate;
                }
            }
        }
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
        }

        // Engage 상태: 플레이어 방향 회전 + 발사
        if (currentState == "Engage")
        {
            RotateTowardsPlayer(deltaTime);
            HandleShooting(deltaTime);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태 진입 콜백
    // ═══════════════════════════════════════════════════════════════
    void MonsterScript::OnStateEntered(const std::string& state)
    {
        if (state == "Dead")
        {
            OnDeath();
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
        
        m_Hp = j.value("Hp", 100.0f);
        m_AttackRange = j.value("AttackRange", 10.0f);
        m_moveSpeed = j.value("MoveSpeed", 0.0f);
        m_rotationSpeed = j.value("RotationSpeed", 2.0f);
        m_fireRate = j.value("FireRate", 3.0f);
        m_bulletSpeed = j.value("BulletSpeed", 1.0f);
        m_bulletLifetime = j.value("BulletLifetime", 3.0f);
    }
}
