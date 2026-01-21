#include "GamePCH.h"
#include "TestLogicFSM.h"
#include "TestAnimationFSM.h"
#include "InputBinding.h"

#include <Framework/Object/GameObject/GameObject.h>

namespace game
{
    void TestLogicFSM::Awake()
    {
        CharacterLogicFSM::Awake();
    }

    void TestLogicFSM::Start()
    {
        CharacterLogicFSM::Start();
        LOG_PRINT("[TestLogicFSM] Started");
    }

    void TestLogicFSM::Update()
    {
        CharacterLogicFSM::Update();
        
        // 회전 처리
        HandleRotation();
    }

    void TestLogicFSM::UpdateWalk()
    {
        // 정지
        if (!IsMoving())
        {
            ChangeState(CharacterState::Idle);
            return;
        }

        // 공격 (이동 중에도 공격 가능)
        if (IsAttackPressed())
        {
            ChangeState(CharacterState::Attack);
            return;
        }

        // 이동 적용
        engine::Vector3 currentPos = GetTransform()->GetLocalPosition();
        currentPos += m_context.moveDirection * m_moveSpeed * engine::Time::DeltaTime();
        GetTransform()->SetLocalPosition(currentPos);
    }

    void TestLogicFSM::OnEnterAttack()
    {
        CharacterLogicFSM::OnEnterAttack();
        LOG_PRINT("[TestLogicFSM] Attack!");
    }

    void TestLogicFSM::UpdateAttack()
    {
        // 공격 상태는 애니메이션 종료까지 유지
        // OnAnimationFinished()에서 Idle로 복귀
        
        // 이동 입력이 있으면 이동 방향으로 회전만 함 (위치는 이동 안함)
    }

    void TestLogicFSM::HandleRotation()
    {
        if (!m_rotateToMoveDirection)
        {
            return;
        }

        // 이동 방향이 있을 때만 회전
        if (m_context.moveDirection.LengthSquared() < 0.001f)
        {
            return;
        }

        // 목표 회전 각도 계산 (Y축 기준)
        float targetAngle = std::atan2(m_context.moveDirection.x, m_context.moveDirection.z);
        targetAngle = DirectX::XMConvertToDegrees(targetAngle);

        // 현재 회전 각도
        engine::Quaternion currentRot = GetTransform()->GetLocalRotation();
        engine::Vector3 euler = currentRot.ToEuler();
        float currentAngle = DirectX::XMConvertToDegrees(euler.y);

        // 부드러운 회전
        float angleDiff = targetAngle - currentAngle;
        
        // -180 ~ 180 범위로 정규화
        while (angleDiff > 180.0f) angleDiff -= 360.0f;
        while (angleDiff < -180.0f) angleDiff += 360.0f;

        float maxRotation = m_rotationSpeed * engine::Time::DeltaTime();
        float rotation = std::clamp(angleDiff, -maxRotation, maxRotation);

        float newAngle = currentAngle + rotation;
        engine::Quaternion newRot = engine::Quaternion::CreateFromYawPitchRoll(
            DirectX::XMConvertToRadians(newAngle), 0.0f, 0.0f);
        GetTransform()->SetLocalRotation(newRot);
    }

    void TestLogicFSM::OnGui()
    {
        CharacterLogicFSM::OnGui();

        ImGui::Separator();
        ImGui::Text("TestLogicFSM Settings");
        
        ImGui::Checkbox("Rotate To Move Direction", &m_rotateToMoveDirection);
        ImGui::DragFloat("Rotation Speed", &m_rotationSpeed, 1.0f, 0.0f, 720.0f);
    }

    void TestLogicFSM::Save(engine::json& j) const
    {
        CharacterLogicFSM::Save(j);
        
        j["RotationSpeed"] = m_rotationSpeed;
        j["RotateToMoveDirection"] = m_rotateToMoveDirection;
    }

    void TestLogicFSM::Load(const engine::json& j)
    {
        CharacterLogicFSM::Load(j);
        
        engine::JsonGet(j, "RotationSpeed", m_rotationSpeed);
        engine::JsonGet(j, "RotateToMoveDirection", m_rotateToMoveDirection);
    }

    std::string TestLogicFSM::GetType() const
    {
        return "TestLogicFSM";
    }
}
