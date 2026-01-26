#pragma once

#include "Script/CharacterScript/Common/BaseControllerScript.h"
#include "Script/CharacterScript/Common/BulletParams.h"

namespace engine
{
    class Rigidbody;
    class SkeletalAnimator;
    class PathfindingSystem;
    struct PathResult;
}

namespace game
{
    class BulletFactory;
    class PlayerControllerScript;

    // ═══════════════════════════════════════════════════════════════
    // MonsterScript - 몬스터 공통 기반 클래스
    // 
    // 아키텍처:
    //   - BaseControllerScript 상속 (LogicFSM + AnimFSM 활용)
    //   - 공통 로직: 플레이어 추적, 거리 계산, 회전, 발사
    //   - 몬스터마다 공격 범위, 체력, 속도 등 차별화
    // 
    // 공통 기능:
    //   - 플레이어 감지 및 추적
    //   - 플레이어 방향으로 회전 (Y축만)
    //   - 쿨다운 기반 자동 발사
    //   - 체력 관리 (Dead 상태 전환)
    // ═══════════════════════════════════════════════════════════════
    class MonsterScript : public BaseControllerScript
    {
        REGISTER_SCRIPT(MonsterScript, BaseControllerScript)

    protected:
        // ─────────────────────────────────────────────
        // 컴포넌트 참조
        // ─────────────────────────────────────────────
        engine::Rigidbody* m_rigidbody = nullptr;
        engine::SkeletalAnimator* m_skeletalAnimator = nullptr;
        BulletFactory* m_bulletFactory = nullptr;
        engine::PathfindingSystem* m_pathfindingSystem = nullptr;  // 경로 찾기 시스템 (향후 이동 몬스터용)

        // ─────────────────────────────────────────────
        // 플레이어 추적
        // ─────────────────────────────────────────────
        PlayerControllerScript* m_targetPlayer = nullptr;
        std::string m_targetPlayerObjectName = "Player";

        // ─────────────────────────────────────────────
        // 몬스터 스탯 (자식 클래스에서 설정)
        // ─────────────────────────────────────────────
        float m_Hp = 100.0f;
        float m_AttackRange = 10.0f;

        // ─────────────────────────────────────────────
        // 이동/회전/발사 설정
        // ─────────────────────────────────────────────
        float m_moveSpeed = 0.0f;            // 이동 속도 (DullGray는 0)
        float m_rotationSpeed = 2.0f;        // 회전 속도 (rad/sec)
        float m_fireRate = 3.0f;             // 발사 간격 (초)
        float m_bulletSpeed = 1.0f;          // 총알 속도
        float m_bulletLifetime = 3.0f;       // 총알 수명 (초)

        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        float m_fireTimer = 0.0f;
        bool m_fsmInitialized = false;
        float m_currentRotationAngle = 0.0f;
        
        // ─────────────────────────────────────────────
        // 애니메이션 이름 (자식 클래스에서 설정)
        // ─────────────────────────────────────────────
        std::string m_animName_Attack = "Attack";  // 공격 애니메이션 이름

        // ─────────────────────────────────────────────
        // 회전 완료 판정
        // ─────────────────────────────────────────────
        static constexpr float ROTATION_THRESHOLD = 2.0f * 3.14159f / 180.0f;  // 2도 (라디안)

    public:
        virtual void Awake() override;
        virtual void Start() override;

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

        // ─────────────────────────────────────────────
        // 초기화 (자식에서 오버라이드 필수)
        // ─────────────────────────────────────────────
        virtual void InitializeFSM() {}          // 자식에서 구현
        virtual void InitializeAnimFSM() {}      // 자식에서 구현
        virtual void InitializeAnimations() {}   // 자식에서 구현
        virtual void InitializeBullet() {}       // 자식에서 구현 (총알 설정)

        // ─────────────────────────────────────────────
        // 플레이어 추적
        // ─────────────────────────────────────────────
        void FindPlayer();
        
        // 거리 계산
        float GetDistanceToPlayer() const;              // 직선 거리 (빠름)
        float GetPathDistanceToPlayer() const;          // 경로 거리 (느림, PathfindingSystem 필요)
        bool IsPlayerInRange() const;
        bool m_isPlayerInRange = false;
        
        // 방향 계산
        engine::Vector3 CalculateDirectionToPlayer() const;  // 플레이어 방향 벡터 계산

        // ─────────────────────────────────────────────
        // 회전 및 발사
        // ─────────────────────────────────────────────
        void RotateTowardsPlayer(float deltaTime);
        void RotateTowards(const engine::Vector3& targetDirection, float deltaTime);  // 특정 방향으로 회전
        bool IsRotatedTowardsPlayer() const;
        void HandleShooting(float deltaTime);
        
        // ─────────────────────────────────────────────
        // 이동 (PathfindingSystem 활용, 향후 이동 몬스터용)
        // ─────────────────────────────────────────────
        virtual void MoveTowardsPlayer(float deltaTime);     // 플레이어를 향해 이동 (PathfindingSystem 사용)

        // ─────────────────────────────────────────────
        // 체력 관리
        // ─────────────────────────────────────────────
        virtual void OnDeath();
        bool CheckDeath();
        bool m_isDead = false;

        // ─────────────────────────────────────────────
        // 에디터 검증
        // ─────────────────────────────────────────────
        virtual bool ValidateComponents() const;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
