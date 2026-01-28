#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class Rigidbody;
    class Transform;
}

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // KinematicHelperScript - Kinematic Rigidbody용 이동/회전 헬퍼
    // 
    // 용도:
    //   - Kinematic Rigidbody를 사용하는 오브젝트의 이동/회전 처리
    //   - Dynamic Rigidbody가 아닌 경우에 사용
    //   - 재사용 가능한 유틸리티 스크립트
    // 
    // 사용법:
    //   1. GameObject에 이 스크립트 추가
    //   2. SetMoveDirection() / SetTargetPosition()으로 이동 목표 설정
    //   3. SetTargetRotation() / SetLookAtDirection()으로 회전 목표 설정
    //   4. FixedUpdate에서 자동으로 MovePosition/MoveRotation 처리
    // 
    // 주의:
    //   - 반드시 Kinematic Rigidbody가 필요합니다
    //   - Dynamic Rigidbody에는 사용하지 마세요
    // ═══════════════════════════════════════════════════════════════
    class KinematicHelperScript : public engine::Script<KinematicHelperScript>
    {
        //REGISTER_SCRIPT(KinematicHelperScript, Script)
        DEFINE_COMPONENT_TYPE(KinematicHelperScript, Script)

    private:
        // ─────────────────────────────────────────────
        // 컴포넌트 참조
        // ─────────────────────────────────────────────
        engine::Rigidbody* m_rigidbody = nullptr;

        // ─────────────────────────────────────────────
        // 이동 설정
        // ─────────────────────────────────────────────
        float m_moveSpeed = 5.0f;
        engine::Vector3 m_moveDirection = engine::Vector3::Zero;
        bool m_isMoving = false;

        // ─────────────────────────────────────────────
        // 회전 설정
        // ─────────────────────────────────────────────
        float m_rotationSpeed = 10.0f;  // rad/sec
        engine::Vector3 m_targetLookDirection = engine::Vector3::UnitZ;
        bool m_isRotating = false;

        // ─────────────────────────────────────────────
        // 상태
        // ─────────────────────────────────────────────
        float m_currentRotationAngle = 0.0f;

    public:
        // ─────────────────────────────────────────────
        // 생명주기
        // ─────────────────────────────────────────────
        void Awake() override;
        void Start() override;
        void FixedUpdate() override;

        // ─────────────────────────────────────────────
        // 이동 API
        // ─────────────────────────────────────────────
        
        // 이동 방향 설정 (정규화된 방향 벡터)
        void SetMoveDirection(const engine::Vector3& direction);
        
        // 이동 속도 설정
        void SetMoveSpeed(float speed) { m_moveSpeed = speed; }
        float GetMoveSpeed() const { return m_moveSpeed; }
        
        // 이동 정지
        void StopMovement();
        
        // 이동 중인지 확인
        bool IsMoving() const { return m_isMoving; }

        // ─────────────────────────────────────────────
        // 회전 API
        // ─────────────────────────────────────────────
        
        // 특정 방향을 바라보도록 설정
        void SetLookAtDirection(const engine::Vector3& direction);
        
        // 특정 위치를 바라보도록 설정
        void SetLookAtPosition(const engine::Vector3& targetPosition);
        
        // 회전 속도 설정
        void SetRotationSpeed(float speed) { m_rotationSpeed = speed; }
        float GetRotationSpeed() const { return m_rotationSpeed; }
        
        // 회전 정지
        void StopRotation();
        
        // 회전 중인지 확인
        bool IsRotating() const { return m_isRotating; }
        
        // 목표 방향을 보고 있는지 확인
        bool IsLookingAtDirection(const engine::Vector3& direction) const;

        // ─────────────────────────────────────────────
        // 유틸리티
        // ─────────────────────────────────────────────
        
        // 모든 움직임 정지
        void StopAll();
        
        // Kinematic인지 확인
        bool IsKinematic() const;

    private:
        // ─────────────────────────────────────────────
        // 내부 처리
        // ─────────────────────────────────────────────
        void ProcessMovement(float fixedDeltaTime);
        void ProcessRotation(float fixedDeltaTime);

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
