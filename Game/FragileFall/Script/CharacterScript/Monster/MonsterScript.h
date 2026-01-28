#pragma once

#include "Script/CharacterScript/Common/BaseControllerScript.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/CharacterScript/Common/BulletParams.h"

namespace engine
{
    class Rigidbody;
    class SkeletalAnimator;
    class PathfindingAgent;
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
        engine::PathfindingAgent* m_pathfindingAgent = nullptr;  // 경로 찾기 에이전트

        // ─────────────────────────────────────────────
        // 플레이어 추적
        // ─────────────────────────────────────────────
        PlayerControllerScript* m_targetPlayer = nullptr;
        std::string m_targetPlayerObjectName = "Player";

        // ─────────────────────────────────────────────
        // 몬스터 스탯 (자식 클래스에서 설정)
        // ─────────────────────────────────────────────
        int m_Hp = 10;
        float m_AttackRange = 5.0f;

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
        
        // ─────────────────────────────────────────────
        // 애니메이션 이름 (자식 클래스에서 설정)
        // ─────────────────────────────────────────────
        //std::string m_animName_Attack = "Attack";  // 공격 애니메이션 이름

        // ─────────────────────────────────────────────
        // 회전 완료 판정 (물리 기반 회전에서는 더 큰 임계값 필요)
        // ─────────────────────────────────────────────
        static constexpr float ROTATION_THRESHOLD = 10.0f * 3.14159f / 180.0f;  // 10도 (라디안)

    public:
        virtual void Awake() override;
        virtual void Start() override;

    protected:
        // ─────────────────────────────────────────────
        // BaseControllerScript 오버라이드
        // ─────────────────────────────────────────────
        void CacheComponents() override;
        void ProcessInput() override;
        void UpdateGameLogic() override;         // 비물리 로직 (타이머, 체력 등)
        void UpdatePhysicsLogic() override;      // 물리 로직 (이동, 회전)
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
        // 플레이어 추적 (Player 특화 wrapper 함수)
        // ─────────────────────────────────────────────
        void FindPlayer();
        
        float GetDistanceToPlayer() const;
        engine::Vector3 CalculateDirectionToPlayer() const;
        bool IsPlayerInRange() const;
        void RotateTowardsPlayer();  // FixedUpdate에서 호출
        bool IsLookingAtPlayer() const;
        
        // 호환성 유지
        bool IsRotatedTowardsPlayer() const { return IsLookingAtPlayer(); }
        
        bool m_isPlayerInRange = false;
        
        // ─────────────────────────────────────────────
        // 경로 찾기 (PathfindingAgent 활용)
        // ─────────────────────────────────────────────
        float GetPathDistanceToPlayer() const;
        
        void StopAllMovement();
        
        // ─────────────────────────────────────────────
        // 공격 (자손 클래스에서 오버라이드)
        // ─────────────────────────────────────────────
        virtual void Attack(float deltaTime);
        
        // ─────────────────────────────────────────────
        // 상태별 행동 - 비물리 (Update에서 호출, DeltaTime 기반)
        // ─────────────────────────────────────────────
        virtual void UpdateStateBasedBehavior(const std::string& state, float deltaTime);
        virtual void ExecuteEngageBehaviorNonPhysics(float deltaTime);  // 공격 타이머 등
        virtual void ExecuteIdleBehaviorNonPhysics();
        virtual void ExecuteFragileBehaviorNonPhysics();
        virtual void ExecuteDeadBehaviorNonPhysics();

        // ─────────────────────────────────────────────
        // 상태별 행동 - 물리 (FixedUpdate에서 호출)
        // ─────────────────────────────────────────────
        virtual void UpdatePhysicsStateBasedBehavior(const std::string& state);
        virtual void ExecuteEngageBehaviorPhysics();    // 이동, 회전
        virtual void ExecuteIdleBehaviorPhysics();
        virtual void ExecuteFragileBehaviorPhysics();
        virtual void ExecuteDeadBehaviorPhysics();
        
        // ─────────────────────────────────────────────
        // 이동 (PathfindingAgent 활용, 이동 몬스터용)
        // ─────────────────────────────────────────────
        virtual void MoveTowardsPlayer();  // FixedUpdate에서 호출

        // ─────────────────────────────────────────────
        // 체력 관리
        // ─────────────────────────────────────────────
        void CheckHealth();
        void TriggerFragile();
        void TriggerDeath();  // Execution에서 호출
        virtual void OnFragile();
        virtual void OnDeath();
        
        bool m_isFragile = false;
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
