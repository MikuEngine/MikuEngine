#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class Rigidbody;
}

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // CheckRotateScript
    // 
    // 외적을 이용한 최단 회전 방향 판단을 시각적으로 보여주기 위한 테스트 스크립트
    // - P1, P2를 원점 기준으로 Q1, Q2 방향으로 회전
    // - Dynamic Rigidbody의 AddForce를 이용한 물리 기반 원형 운동
    // ═══════════════════════════════════════════════════════════════

    class CheckRotateScript :
        public engine::Script<CheckRotateScript>
    {
        REGISTER_COMPONENT(CheckRotateScript, Script)

    private:
        // 회전 상태 (enum을 먼저 정의)
        enum class RotateState { Waiting, Rotating, Completed };

        // 게임오브젝트 참조
        engine::GameObject* m_bulletP1 = nullptr;
        engine::GameObject* m_bulletQ1 = nullptr;
        engine::GameObject* m_bulletP2 = nullptr;
        engine::GameObject* m_bulletQ2 = nullptr;

        // Rigidbody 참조
        engine::Rigidbody* m_rbP1 = nullptr;
        engine::Rigidbody* m_rbP2 = nullptr;

        // Cube (원점에서 Y축 자전)
        engine::GameObject* m_cube = nullptr;
        engine::Rigidbody* m_rbCube = nullptr;
        RotateState m_stateCube = RotateState::Waiting;

        // 초기 방향 벡터 (정규화, Y=0)
        engine::Vector3 m_initialDirP1;
        engine::Vector3 m_initialDirP2;
        engine::Vector3 m_targetDirQ1;
        engine::Vector3 m_targetDirQ2;

        // 초기 반지름 (원점에서의 거리)
        float m_radiusP1 = 0.0f;
        float m_radiusP2 = 0.0f;

        // 회전 상태
        RotateState m_stateP1 = RotateState::Waiting;
        RotateState m_stateP2 = RotateState::Waiting;

        // 회전 방향 (1: 양의 방향 CW, -1: 음의 방향 CCW)
        float m_rotationDirP1 = 0.0f;
        float m_rotationDirP2 = 0.0f;

        // 누적 회전 각도 (도)
        float m_rotatedAngleP1 = 0.0f;
        float m_rotatedAngleP2 = 0.0f;

        // 목표까지의 총 회전 각도 (도)
        float m_totalAngleP1 = 0.0f;
        float m_totalAngleP2 = 0.0f;

        // 타이머
        float m_waitTimer = 0.0f;
        float m_waitDuration = 2.0f;  // 2초 대기

        // 회전 설정
        float m_rotationSpeed = 5.0f;           // 도/초
        float m_maxLinearSpeed = 10.0f;         // 최대 선형 속도 (속도 캡)
        float m_accelerationForce = 500.0f;     // 가속 힘 (크게 설정하여 빠르게 목표 속도 도달)

        // 완료 판정 임계값
        float m_completionThreshold = 0.9999f;  // 내적 값이 이 이상이면 완료

    public:
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        // 오브젝트 초기화
        bool InitializeObjects();

        // 회전 방향 계산 (외적 이용)
        float CalculateRotationDirection(const engine::Vector3& from, const engine::Vector3& to);

        // 두 방향 벡터 사이의 각도 계산 (도)
        float CalculateAngleBetween(const engine::Vector3& from, const engine::Vector3& to);

        // 물리 기반 원형 운동 업데이트
        void UpdateCircularMotion(
            engine::Rigidbody* rb,
            float rotationDir,
            float radius,
            RotateState& state,
            float& rotatedAngle,
            float totalAngle,
            const engine::Vector3& targetDir
        );

        // 접선 방향 계산 (Y축 기준)
        engine::Vector3 GetTangentDirection(const engine::Vector3& position, float rotationDir);

        // 상태를 문자열로 변환
        const char* StateToString(RotateState state);
    };
}
