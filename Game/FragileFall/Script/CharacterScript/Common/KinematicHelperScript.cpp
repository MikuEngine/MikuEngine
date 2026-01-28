#include "GamePCH.h"
#include "KinematicHelperScript.h"

#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/Object/GameObject/GameObject.h>
#include <Engine/Core/System/MyTime.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void KinematicHelperScript::Awake()
    {
        if (!GetGameObject()) return;
        
        m_rigidbody = GetGameObject()->GetComponent<engine::Rigidbody>();
    }

    void KinematicHelperScript::Start()
    {
        // 초기 회전 각도 동기화
        if (GetTransform())
        {
            engine::Quaternion currentRot = GetTransform()->GetWorldRotation();
            engine::Vector3 euler = currentRot.ToEuler();
            m_currentRotationAngle = euler.y;
        }

        // Kinematic 확인
        if (m_rigidbody && !m_rigidbody->IsKinematic())
        {
            LOG_PRINT("[KinematicHelperScript] WARNING: Rigidbody is not Kinematic! This script is designed for Kinematic bodies.");
        }
    }

    void KinematicHelperScript::FixedUpdate()
    {
        if (!m_rigidbody || !m_rigidbody->IsKinematic())
        {
            return;
        }

        float fixedDeltaTime = engine::Time::FixedDeltaTime();
        
        ProcessMovement(fixedDeltaTime);
        ProcessRotation(fixedDeltaTime);
    }

    // ═══════════════════════════════════════════════════════════════
    // 이동 API
    // ═══════════════════════════════════════════════════════════════
    void KinematicHelperScript::SetMoveDirection(const engine::Vector3& direction)
    {
        if (direction.LengthSquared() < 0.0001f)
        {
            StopMovement();
            return;
        }

        m_moveDirection = direction;
        m_moveDirection.y = 0.0f;
        m_moveDirection.Normalize();
        m_isMoving = true;
    }

    void KinematicHelperScript::StopMovement()
    {
        m_moveDirection = engine::Vector3::Zero;
        m_isMoving = false;
    }

    // ═══════════════════════════════════════════════════════════════
    // 회전 API
    // ═══════════════════════════════════════════════════════════════
    void KinematicHelperScript::SetLookAtDirection(const engine::Vector3& direction)
    {
        if (direction.LengthSquared() < 0.0001f)
        {
            return;
        }

        m_targetLookDirection = direction;
        m_targetLookDirection.y = 0.0f;
        m_targetLookDirection.Normalize();
        m_isRotating = true;
    }

    void KinematicHelperScript::SetLookAtPosition(const engine::Vector3& targetPosition)
    {
        if (!GetTransform()) return;

        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        engine::Vector3 direction = targetPosition - myPos;
        
        SetLookAtDirection(direction);
    }

    void KinematicHelperScript::StopRotation()
    {
        m_isRotating = false;
    }

    bool KinematicHelperScript::IsLookingAtDirection(const engine::Vector3& direction) const
    {
        if (!GetTransform() || direction.LengthSquared() < 0.0001f)
        {
            return true;
        }

        engine::Vector3 targetDir = direction;
        targetDir.y = 0.0f;
        targetDir.Normalize();

        // 현재 방향
        engine::Vector3 currentDir(
            sinf(m_currentRotationAngle),
            0.0f,
            cosf(m_currentRotationAngle)
        );

        float dot = currentDir.Dot(targetDir);
        
        // 15도 이내면 목표를 보고 있는 것으로 판단
        const float THRESHOLD = cosf(15.0f * 3.14159f / 180.0f);
        return dot >= THRESHOLD;
    }

    // ═══════════════════════════════════════════════════════════════
    // 유틸리티
    // ═══════════════════════════════════════════════════════════════
    void KinematicHelperScript::StopAll()
    {
        StopMovement();
        StopRotation();
    }

    bool KinematicHelperScript::IsKinematic() const
    {
        return m_rigidbody && m_rigidbody->IsKinematic();
    }

    // ═══════════════════════════════════════════════════════════════
    // 내부 처리
    // ═══════════════════════════════════════════════════════════════
    void KinematicHelperScript::ProcessMovement(float fixedDeltaTime)
    {
        if (!m_isMoving || !m_rigidbody || !GetTransform())
        {
            return;
        }

        engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
        engine::Vector3 newPos = currentPos + m_moveDirection * m_moveSpeed * fixedDeltaTime;
        
        m_rigidbody->MovePosition(newPos);
    }

    void KinematicHelperScript::ProcessRotation(float fixedDeltaTime)
    {
        if (!m_isRotating || !m_rigidbody || !GetTransform())
        {
            return;
        }

        // 현재 방향 벡터
        engine::Vector3 currentDir(
            sinf(m_currentRotationAngle),
            0.0f,
            cosf(m_currentRotationAngle)
        );

        // 목표 도달 확인
        float dotToTarget = currentDir.Dot(m_targetLookDirection);
        
        if (dotToTarget > 0.9999f)
        {
            m_isRotating = false;
            return;
        }

        // 회전 방향 결정 (외적의 Y 성분)
        float crossY = currentDir.z * m_targetLookDirection.x - currentDir.x * m_targetLookDirection.z;
        float rotationSign = (crossY > 0.0f) ? 1.0f : -1.0f;

        // 회전 적용
        float rotationAmount = m_rotationSpeed * fixedDeltaTime;
        m_currentRotationAngle += rotationSign * rotationAmount;

        engine::Quaternion newRot = engine::Quaternion::CreateFromAxisAngle(
            engine::Vector3::UnitY, m_currentRotationAngle);
        
        m_rigidbody->MoveRotation(newRot);
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void KinematicHelperScript::OnGui()
    {
        ImGui::Indent();
        
        ImGui::Text("KinematicHelperScript:");

        // 상태 표시
        ImGui::Separator();
        ImGui::Text("Status:");
        
        bool isKinematic = IsKinematic();
        if (isKinematic)
        {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "[OK] Rigidbody is Kinematic");
        }
        else if (m_rigidbody)
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "[WARNING] Rigidbody is NOT Kinematic!");
        }
        else
        {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "[ERROR] No Rigidbody found!");
        }

        // 이동 설정
        ImGui::Separator();
        ImGui::Text("Movement:");
        ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.1f, 0.0f, 100.0f);
        ImGui::Text("Is Moving: %s", m_isMoving ? "Yes" : "No");
        if (m_isMoving)
        {
            ImGui::Text("Direction: (%.2f, %.2f, %.2f)", 
                m_moveDirection.x, m_moveDirection.y, m_moveDirection.z);
        }

        // 회전 설정
        ImGui::Separator();
        ImGui::Text("Rotation:");
        ImGui::DragFloat("Rotation Speed", &m_rotationSpeed, 0.1f, 0.0f, 20.0f);
        ImGui::Text("Is Rotating: %s", m_isRotating ? "Yes" : "No");
        ImGui::Text("Current Angle: %.2f rad (%.1f deg)", 
            m_currentRotationAngle, 
            m_currentRotationAngle * 180.0f / 3.14159f);

        ImGui::Unindent();

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Spacing();
    }

    void KinematicHelperScript::Save(engine::json& j) const
    {
        j["MoveSpeed"] = m_moveSpeed;
        j["RotationSpeed"] = m_rotationSpeed;
    }

    void KinematicHelperScript::Load(const engine::json& j)
    {
        m_moveSpeed = j.value("MoveSpeed", 5.0f);
        m_rotationSpeed = j.value("RotationSpeed", 10.0f);
    }
}
