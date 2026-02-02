#pragma once

#include "Script/CharacterScript/Monster/MonsterScript.h"
#include "Script/CharacterScript/Common/BulletParams.h"

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterRoundType - 동글 공격 몬스터 공통 프레임워크
    // 
    // 특징:
    //   - MonsterScript 상속 (공통 스탯 6개 + Fragile 부활)
    //   - 5개 색상별 클래스의 기반 클래스
    //   - MonsterPointedType 기반 FSM (Idle, EngageMove, EngageStop, EngageAttack, Fragile, Dead)
    //   - PathfindingAgent 기본 사용 (색상별로 오버라이드 가능)
    //   - AttackType::Round 고정
    // 
    // 색상별 클래스:
    //   - MonsterRoundGray, MonsterRoundGreen, MonsterRoundBlue,
    //   - MonsterRoundRed, MonsterRoundPurple
    // ═══════════════════════════════════════════════════════════════

    class MonsterRoundType : public MonsterScript
    {
        REGISTER_SCRIPT(MonsterRoundType, MonsterScript)

    protected:
        // ─────────────────────────────────────────────
        // 총알 설정
        // ─────────────────────────────────────────────
        BulletParams m_bulletParams;

        // ─────────────────────────────────────────────
        // 애니메이션 이름 (자식에서 오버라이드 가능)
        // ─────────────────────────────────────────────
        std::string m_animName_Idle = "Idle";
        std::string m_animName_EngageMove = "EngageMove";
        std::string m_animName_EngageStop = "EngageStop";
        std::string m_animName_EngageAttack = "EngageAttack";
        std::string m_animName_Fragile = "Fragile";
        std::string m_animName_Dead = "Dead";

        // ─────────────────────────────────────────────
        // 공격 관련
        // ─────────────────────────────────────────────
        float m_attackAnimationDuration = 1.0f;   // 공격 애니메이션 재생 시간
        float m_attackAnimationTimer = 0.0f;      // 공격 애니메이션 타이머
        
        bool m_isPlayerInDetectionRange = false;  // 플레이어가 감지 거리 안에 있는지
        bool m_canFire = false;                   // 발사 가능한지 (쿨타임 체크)

    public:
        void Awake() override;
        void Start() override;

    protected:
        // ─────────────────────────────────────────────
        // MonsterScript 오버라이드
        // ─────────────────────────────────────────────
        void InitializeFSM() override;
        void InitializeAnimFSM() override;
        void InitializeAnimations() override;
        void InitializeBullet() override;
        
        // 입력 처리 (FSM 파라미터 업데이트)
        void ProcessInput() override;
        
        // ─────────────────────────────────────────────
        // 상태별 행동 - 비물리 (Update에서 호출)
        // ─────────────────────────────────────────────
        void UpdateStateBasedBehavior(const std::string& state, float deltaTime) override;
        virtual void ExecuteEngageMoveBehaviorNonPhysics(float deltaTime);
        virtual void ExecuteEngageStopBehaviorNonPhysics(float deltaTime);
        virtual void ExecuteEngageAttackBehaviorNonPhysics(float deltaTime);
        void ExecuteIdleBehaviorNonPhysics() override;
        void ExecuteFragileBehaviorNonPhysics() override;
        void ExecuteDeadBehaviorNonPhysics() override;
        
        // ─────────────────────────────────────────────
        // 상태별 행동 - 물리 (FixedUpdate에서 호출)
        // ─────────────────────────────────────────────
        void UpdatePhysicsStateBasedBehavior(const std::string& state) override;
        virtual void ExecuteEngageMoveBehaviorPhysics();
        virtual void ExecuteEngageStopBehaviorPhysics();
        virtual void ExecuteEngageAttackBehaviorPhysics();
        void ExecuteIdleBehaviorPhysics() override;
        void ExecuteFragileBehaviorPhysics() override;
        void ExecuteDeadBehaviorPhysics() override;
        
        // ─────────────────────────────────────────────
        // 상태 진입 콜백
        // ─────────────────────────────────────────────
        void OnStateEntered(const std::string& state) override;
        
        // ─────────────────────────────────────────────
        // 공격 (자식에서 오버라이드)
        // ─────────────────────────────────────────────
        void Attack(float deltaTime) override;
        
        // ─────────────────────────────────────────────
        // 행동 제한
        // ─────────────────────────────────────────────
        bool CanMove() const override;
        bool CanAttack() const override;
        
        // ─────────────────────────────────────────────
        // 헬퍼 함수
        // ─────────────────────────────────────────────
        bool IsPlayerInDetectionRange() const;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}