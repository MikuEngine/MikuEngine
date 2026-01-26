#pragma once

#include "Script/CharacterScript/Common/BaseControllerScript.h"

namespace engine
{
    class Rigidbody;
    class SkeletalAnimator;
    class GameObject;
}

namespace game
{
    class BulletFactory;

    // ═══════════════════════════════════════════════════════════════
    // MonsterScript - 몬스터 기본 컨트롤러
    // 
    // 모든 몬스터 타입의 기반 클래스입니다.
    // 단일 레이어 애니메이션을 사용하며, 공통 헬퍼 함수를 제공합니다.
    // 
    // 사용법:
    //   1. 이 클래스를 상속받아서 몬스터 종류별 클래스 생성
    //   2. InitializeFSM()에서 해당 몬스터의 상태와 전이 정의
    //   3. UpdateGameLogic()에서 AI 로직 구현
    // ═══════════════════════════════════════════════════════════════
    class MonsterScript :
        public BaseControllerScript
    {
        REGISTER_SCRIPT(MonsterScript, BaseControllerScript)

    protected:
        // ─────────────────────────────────────────────
        // 컴포넌트 참조
        // ─────────────────────────────────────────────
        engine::Rigidbody* m_rigidbody = nullptr;
        engine::SkeletalAnimator* m_skeletalAnimator = nullptr;
        BulletFactory* m_bulletFactory = nullptr;

        // ─────────────────────────────────────────────
        // 이동 설정
        // ─────────────────────────────────────────────
        float m_moveSpeed = 5.0f;

        // ─────────────────────────────────────────────
        // 발사 설정
        // ─────────────────────────────────────────────
        float m_fireRate = 0.2f;         // 발사 간격 (초)
        float m_bulletSpeed = 1.0f;      // 총알 속도
        float m_bulletLifetime = 3.0f;   // 총알 수명 (초)

        // ─────────────────────────────────────────────
        // 회전 설정
        // ─────────────────────────────────────────────
        float m_rotationSpeed = 10.0f;   // 회전 속도 (rad/sec * factor)

        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        float m_fireTimer = 0.0f;
        bool m_fsmInitialized = false;
        
        // 현재 회전 각도 (라디안, 벡터 기반 회전용)
        float m_currentRotationAngle = 0.0f;

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

        // ─────────────────────────────────────────────
        // 행동 제한 (하이브리드 패턴)
        // ─────────────────────────────────────────────
        bool CanMove() const override;
        bool CanAttack() const override;

        // ─────────────────────────────────────────────
        // 초기화 (자식에서 오버라이드)
        // ─────────────────────────────────────────────
        virtual void InitializeFSM();
        virtual void InitializeAnimFSM();

        // ─────────────────────────────────────────────
        // 애니메이션 (자식에서 오버라이드 가능)
        // ─────────────────────────────────────────────
        virtual void UpdateAnimation();

        // ═══════════════════════════════════════════════════════════════
        // 헬퍼 함수 - 거리/방향 계산
        // ═══════════════════════════════════════════════════════════════
        
        /// @brief 대상 오브젝트와의 거리를 반환
        /// @param target 대상 게임오브젝트
        /// @return XZ 평면 기준 거리
        float GetDistanceFromTarget(engine::GameObject* target) const;

        /// @brief 대상 오브젝트를 향한 방향 벡터를 반환
        /// @param target 대상 게임오브젝트
        /// @return 정규화된 방향 벡터 (XZ 평면, Y=0)
        engine::Vector3 GetTargetDirection(engine::GameObject* target) const;

        // ═══════════════════════════════════════════════════════════════
        // 헬퍼 함수 - 벡터 기반 회전
        // ═══════════════════════════════════════════════════════════════
        
        /// @brief 대상 오브젝트를 향해 벡터 기반 회전
        /// @param target 대상 게임오브젝트
        /// @param deltaTime 프레임 시간
        void RotateToTargetByVector(engine::GameObject* target, float deltaTime);

        /// @brief 입력받은 방향 벡터를 향해 벡터 기반 회전
        /// @param direction 목표 방향 벡터 (정규화 안 해도 됨)
        /// @param deltaTime 프레임 시간
        void RotateToDirByVector(const engine::Vector3& direction, float deltaTime);

        /// @brief 즉시 해당 방향을 바라보도록 회전 (보간 없음)
        /// @param direction 목표 방향 벡터
        void LookAtDirection(const engine::Vector3& direction);

        /// @brief 즉시 대상을 바라보도록 회전 (보간 없음)
        /// @param target 대상 게임오브젝트
        void LookAtTarget(engine::GameObject* target);

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
