#include "GamePCH.h"
#include "MonsterScript.h"

#include "BulletFactory.h"

#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Engine/Core/System/Input.h>
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

        // LogicFSM 초기화 (한 번만)
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

        // 초기 회전 설정 (현재 Transform 기준)
        if (GetTransform())
        {
            engine::Vector3 forward = GetTransform()->GetForward();
            m_currentRotationAngle = atan2f(forward.x, forward.z);
        }
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

        // BulletFactory: 씬에서 검색
        auto* scene = engine::SceneManager::Get().GetScene();
        if (scene)
        {
            if (auto* factoryGO = scene->FindGameObject("BulletFactory"))
            {
                m_bulletFactory = factoryGO->GetComponent<BulletFactory>();
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 입력 처리 (몬스터는 AI이므로 기본 구현은 비어있음)
    // ═══════════════════════════════════════════════════════════════
    
    void MonsterScript::ProcessInput()
    {
        // 몬스터는 AI로 동작하므로 입력 처리 없음
        // 자식 클래스에서 AI 로직으로 FSM 파라미터 설정
    }

    // ═══════════════════════════════════════════════════════════════
    // 게임 로직 (자식에서 오버라이드 권장)
    // ═══════════════════════════════════════════════════════════════
    
    void MonsterScript::UpdateGameLogic()
    {
        // 기본 구현: 애니메이션만 업데이트
        UpdateAnimation();
    }

    // ═══════════════════════════════════════════════════════════════
    // 행동 제한 (기본 구현)
    // ═══════════════════════════════════════════════════════════════
    
    bool MonsterScript::CanMove() const
    {
        if (!m_logicFSM) return true;

        std::string state = m_logicFSM->GetCurrentState();

        // 기본적으로 Dead, Stun 상태에서는 이동 불가
        if (state == "Dead" || state == "Stun" || state == "Hit")
        {
            return false;
        }

        return true;
    }

    bool MonsterScript::CanAttack() const
    {
        if (!m_logicFSM) return true;

        std::string state = m_logicFSM->GetCurrentState();

        // 기본적으로 Dead, Stun 상태에서는 공격 불가
        if (state == "Dead" || state == "Stun" || state == "Hit")
        {
            return false;
        }

        return true;
    }

    // ═══════════════════════════════════════════════════════════════
    // FSM 초기화 (자식에서 오버라이드)
    // ═══════════════════════════════════════════════════════════════
    
    void MonsterScript::InitializeFSM()
    {
        if (!m_logicFSM) return;

        // 기본 상태만 추가 (자식에서 확장)
        AddFSMState("Idle", true);
    }

    void MonsterScript::InitializeAnimFSM()
    {
        if (!m_animFSM) return;

        // 단일 레이어 애니메이션 상태 (자식에서 확장)
        // 상체 웨이트 0으로 설정하여 하체만 사용
        m_animFSM->AddSplitState("Idle", "Idle", true, "", false, 0.0f, 0.1f);
    }

    void MonsterScript::UpdateAnimation()
    {
        if (!m_animFSM || !m_logicFSM) return;

        // 기본 구현: LogicFSM 상태를 그대로 AnimFSM에 적용
        std::string logicState = m_logicFSM->GetCurrentState();
        m_animFSM->SetAnimState(logicState);
    }

    // ═══════════════════════════════════════════════════════════════
    // 헬퍼 함수 - 거리/방향 계산
    // ═══════════════════════════════════════════════════════════════
    
    float MonsterScript::GetDistanceFromTarget(engine::GameObject* target) const
    {
        if (!target || !GetTransform()) return 0.0f;

        engine::Transform* targetTransform = target->GetTransform();
        if (!targetTransform) return 0.0f;

        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        engine::Vector3 targetPos = targetTransform->GetWorldPosition();

        // XZ 평면 기준 거리 (Y 무시)
        float dx = targetPos.x - myPos.x;
        float dz = targetPos.z - myPos.z;

        return sqrtf(dx * dx + dz * dz);
    }

    engine::Vector3 MonsterScript::GetTargetDirection(engine::GameObject* target) const
    {
        if (!target || !GetTransform()) return engine::Vector3::Zero;

        engine::Transform* targetTransform = target->GetTransform();
        if (!targetTransform) return engine::Vector3::Zero;

        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        engine::Vector3 targetPos = targetTransform->GetWorldPosition();

        engine::Vector3 direction = targetPos - myPos;
        direction.y = 0.0f;  // XZ 평면으로 제한

        float length = direction.Length();
        if (length < 0.0001f) return engine::Vector3::Zero;

        return direction / length;  // 정규화
    }

    // ═══════════════════════════════════════════════════════════════
    // 헬퍼 함수 - 벡터 기반 회전
    // ═══════════════════════════════════════════════════════════════
    
    void MonsterScript::RotateToTargetByVector(engine::GameObject* target, float deltaTime)
    {
        engine::Vector3 direction = GetTargetDirection(target);
        if (direction.LengthSquared() < 0.0001f) return;

        RotateToDirByVector(direction, deltaTime);
    }

    void MonsterScript::RotateToDirByVector(const engine::Vector3& direction, float deltaTime)
    {
        if (!GetTransform()) return;

        // 방향 벡터 정규화
        engine::Vector3 targetDir = direction;
        targetDir.y = 0.0f;
        
        float length = targetDir.Length();
        if (length < 0.0001f) return;
        targetDir /= length;

        // ─────────────────────────────────────────────
        // 현재 방향 벡터 (각도에서 계산)
        // ─────────────────────────────────────────────
        engine::Vector3 currentDir(
            sinf(m_currentRotationAngle),
            0.0f,
            cosf(m_currentRotationAngle)
        );

        // ─────────────────────────────────────────────
        // 목표 도달 확인 (내적으로)
        // ─────────────────────────────────────────────
        float dotToTarget = currentDir.x * targetDir.x + currentDir.z * targetDir.z;
        
        // 거의 같은 방향이면 회전 불필요
        if (dotToTarget > 0.9999f) return;

        // ─────────────────────────────────────────────
        // 외적으로 회전 방향 결정
        // crossY = current.z * target.x - current.x * target.z
        // 양수: 시계 방향 (CW)
        // 음수: 반시계 방향 (CCW)
        // ─────────────────────────────────────────────
        float crossY = currentDir.z * targetDir.x - currentDir.x * targetDir.z;
        float rotationSign = (crossY >= 0.0f) ? 1.0f : -1.0f;

        // ─────────────────────────────────────────────
        // 회전 적용
        // ─────────────────────────────────────────────
        float rotationAmount = m_rotationSpeed * deltaTime;
        m_currentRotationAngle += rotationSign * rotationAmount;

        // 쿼터니언 적용
        engine::Quaternion newRot = engine::Quaternion::CreateFromAxisAngle(
            engine::Vector3::UnitY, m_currentRotationAngle);
        GetTransform()->SetLocalRotation(newRot);
    }

    void MonsterScript::LookAtDirection(const engine::Vector3& direction)
    {
        if (!GetTransform()) return;

        engine::Vector3 dir = direction;
        dir.y = 0.0f;
        
        float length = dir.Length();
        if (length < 0.0001f) return;
        dir /= length;

        // 각도 계산 및 즉시 적용
        m_currentRotationAngle = atan2f(dir.x, dir.z);

        engine::Quaternion newRot = engine::Quaternion::CreateFromAxisAngle(
            engine::Vector3::UnitY, m_currentRotationAngle);
        GetTransform()->SetLocalRotation(newRot);
    }

    void MonsterScript::LookAtTarget(engine::GameObject* target)
    {
        engine::Vector3 direction = GetTargetDirection(target);
        if (direction.LengthSquared() > 0.0001f)
        {
            LookAtDirection(direction);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    
    void MonsterScript::OnGui()
    {
        BaseControllerScript::OnGui();

        if (ImGui::CollapsingHeader("Monster Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("=== Movement ===");
            ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.1f, 0.0f, 50.0f);
            ImGui::DragFloat("Rotation Speed", &m_rotationSpeed, 0.1f, 0.0f, 50.0f);

            ImGui::Separator();
            ImGui::Text("=== Combat ===");
            ImGui::DragFloat("Fire Rate", &m_fireRate, 0.01f, 0.01f, 5.0f);
            ImGui::DragFloat("Bullet Speed", &m_bulletSpeed, 0.1f, 0.1f, 100.0f);
            ImGui::DragFloat("Bullet Lifetime", &m_bulletLifetime, 0.1f, 0.1f, 10.0f);

            ImGui::Separator();
            ImGui::Text("=== Runtime ===");
            ImGui::Text("Fire Timer: %.2f", m_fireTimer);
            ImGui::Text("Rotation Angle: %.1f deg", engine::ToDegree(m_currentRotationAngle));
        }
    }

    void MonsterScript::Save(engine::json& j) const
    {
        BaseControllerScript::Save(j);

        j["MoveSpeed"] = m_moveSpeed;
        j["RotationSpeed"] = m_rotationSpeed;
        j["FireRate"] = m_fireRate;
        j["BulletSpeed"] = m_bulletSpeed;
        j["BulletLifetime"] = m_bulletLifetime;
    }

    void MonsterScript::Load(const engine::json& j)
    {
        BaseControllerScript::Load(j);

        if (j.contains("MoveSpeed"))
            m_moveSpeed = j["MoveSpeed"].get<float>();
        if (j.contains("RotationSpeed"))
            m_rotationSpeed = j["RotationSpeed"].get<float>();
        if (j.contains("FireRate"))
            m_fireRate = j["FireRate"].get<float>();
        if (j.contains("BulletSpeed"))
            m_bulletSpeed = j["BulletSpeed"].get<float>();
        if (j.contains("BulletLifetime"))
            m_bulletLifetime = j["BulletLifetime"].get<float>();
    }
}
