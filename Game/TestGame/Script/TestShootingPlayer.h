#pragma once

#include "BaseControllerScript.h"
#include "BulletParams.h"

namespace engine
{
    class Rigidbody;
    class SkeletalAnimator;
}

namespace game
{
    class AimPointer;
    class BulletFactory;

    // ═══════════════════════════════════════════════════════════════
    // TestShootingPlayer - 하이브리드 FSM 기반 슈팅 플레이어
    // 
    // 아키텍처:
    //   - LogicFSM: 고수준 상태 (Idle, Walk, IdleShoot, WalkShoot)
    //   - 행동 로직: 함수 기반, CanMove()/CanAttack()으로 제한
    //   - AnimFSM: LogicFSM 상태에 따라 애니메이션 재생
    // 
    // FSM 상태:
    //   - Idle: 정지 상태
    //   - Walk: 이동 상태
    //   - IdleShoot: 정지 + 발사
    //   - WalkShoot: 이동 + 발사
    // ═══════════════════════════════════════════════════════════════
    class TestShootingPlayer : public BaseControllerScript
    {
        REGISTER_COMPONENT(TestShootingPlayer, BaseControllerScript)

    protected:
        // ─────────────────────────────────────────────
        // 컴포넌트 참조
        // ─────────────────────────────────────────────
        engine::Rigidbody* m_rigidbody = nullptr;
        engine::SkeletalAnimator* m_skeletalAnimator = nullptr;
        AimPointer* m_aimPointer = nullptr;
        BulletFactory* m_bulletFactory = nullptr;

        // ─────────────────────────────────────────────
        // 이동 설정
        // ─────────────────────────────────────────────
        float m_moveSpeed = 5.0f;

        // ─────────────────────────────────────────────
        // 발사 설정 (쿨다운/타이밍은 Player가 관리)
        // ─────────────────────────────────────────────
        float m_fireRate = 0.2f;         // 발사 간격 (초)
        float m_bulletSpeed = 1.0f;     // 총알 속도
        float m_bulletLifetime = 3.0f;   // 총알 수명 (초)

        // ─────────────────────────────────────────────
        // 참조 설정
        // ─────────────────────────────────────────────
        std::string m_aimPointerObjectName = "AimPointer";  // 씬에서 찾을 AimPointer 오브젝트 이름

        // ─────────────────────────────────────────────
        // 기타 설정
        // ─────────────────────────────────────────────
        bool m_enableUpperBodyAim = true;

        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        float m_fireTimer = 0.0f;
        bool m_fsmInitialized = false;
        
        // 마지막 이동 방향 (캐릭터가 서있는 방향)
        // 초기값: -Z 방향 (아래쪽)
        engine::Vector3 m_lastMoveDirection = engine::Vector3(0.0f, 0.0f, -1.0f);
        
        // 현재 캐릭터 하체 회전 각도 (라디안)
        float m_currentRotationAngle = 0.0f;
        
        // ─────────────────────────────────────────────
        // 에임 방향 추적 (벡터 기반 - 각도 래핑 문제 회피)
        // ─────────────────────────────────────────────
        engine::Vector3 m_prevAimDirection = engine::Vector3(0.0f, 0.0f, 1.0f);  // 이전 에임 방향 (정규화)
        float m_aimCrossProductSmoothed = 0.0f;       // 스무딩된 외적 값 (양수: 반시계, 음수: 시계)
        bool m_aimTrackingInitialized = false;

    public:
        void Awake() override;
        void Start() override;

    protected:
        // ─────────────────────────────────────────────
        // BaseControllerScript 오버라이드
        // ─────────────────────────────────────────────
        void CacheComponents() override;
        void ProcessInput() override;
        void UpdateGameLogic() override;
        void OnStateEntered(const std::string& state) override;

        // ─────────────────────────────────────────────
        // 행동 제한 (하이브리드 패턴)
        // ─────────────────────────────────────────────
        bool CanMove() const override;
        bool CanAttack() const override;

    private:
        // ─────────────────────────────────────────────
        // 초기화
        // ─────────────────────────────────────────────
        void InitializeFSM();
        void InitializeAnimFSM();  // AnimFSM 상태 매핑 등록

        // ─────────────────────────────────────────────
        // 입력 유틸리티
        // ─────────────────────────────────────────────
        engine::Vector3 GetMoveInputDirection() const;

        // ─────────────────────────────────────────────
        // 액션 함수
        // ─────────────────────────────────────────────
        void HandleMovement(float deltaTime);
        void HandleShooting(float deltaTime);
        void UpdateUpperBodyAim();
        float CalculateAimYaw() const;

        // ─────────────────────────────────────────────
        // 애니메이션 제어
        // ─────────────────────────────────────────────
        void UpdateAnimation();
        void UpdateLowerBodyRotation();
        bool IsMovingBackward() const;
        std::string GetAnimationState() const;
        
        // ─────────────────────────────────────────────
        // 에임 추적 유틸리티
        // ─────────────────────────────────────────────
        void UpdateAimTracking();             // 에임 각속도 업데이트
        float GetAimRotationDirection() const; // 에임 회전 방향 (양수: 시계, 음수: 반시계)

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}