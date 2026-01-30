#pragma once

#include "MonsterScript.h"
#include "Script/CharacterScript/Common/BulletParams.h"

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterSharpType - 뾰족 공격 몬스터 구현
    // 
    // 특징:
    //   - 감지 거리 내에 플레이어 진입 시 추적 시작
    //   - 공격 사거리 내 진입 시 정지하고 공격
    //   - 공격 모션 중 이동 불가
    // ═══════════════════════════════════════════════════════════════

    class MonsterSharpType : public MonsterScript
    {
        REGISTER_SCRIPT(MonsterSharpType, MonsterScript)

    private:
        // ─────────────────────────────────────────────
        // 총알 설정
        // ─────────────────────────────────────────────
        BulletParams m_bulletParams;

        // ─────────────────────────────────────────────
        // 애니메이션 이름 (Initialize에서 설정)
        // m_animName_Attack은 부모 클래스 MonsterScript에 정의됨
        // ─────────────────────────────────────────────
        std::string m_animName_Idle = "Idle";
        std::string m_animName_EngageMove = "EngageMove";
        std::string m_animName_EngageStop = "EngageStop";
        std::string m_animName_EngageAttack = "EngageAttack";
        std::string m_animName_Fragile = "Fragile";
        std::string m_animName_Dead = "Dead";

        // ─────────────────────────────────────────────
        // PointedGreen 고유 변수
        // ─────────────────────────────────────────────
        float m_detectionRange = 15.0f;           // 감지 거리 (공격 사거리 * 1.5)
        float m_attackAnimationDuration = 1.1f;   // 공격 애니메이션 재생 시간 (이동 불가 시간)
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
        
        // 상태별 행동 - 비물리 (Update에서 호출)
        void UpdateStateBasedBehavior(const std::string& state, float deltaTime) override;
        void ExecuteEngageMoveBehaviorNonPhysics(float deltaTime);
        void ExecuteEngageStopBehaviorNonPhysics(float deltaTime);
        void ExecuteEngageAttackBehaviorNonPhysics(float deltaTime);
        void ExecuteIdleBehaviorNonPhysics() override;
        void ExecuteFragileBehaviorNonPhysics() override;
        void ExecuteDeadBehaviorNonPhysics() override;
        
        // 상태별 행동 - 물리 (FixedUpdate에서 호출)
        void UpdatePhysicsStateBasedBehavior(const std::string& state) override;
        void ExecuteEngageMoveBehaviorPhysics();
        void ExecuteEngageStopBehaviorPhysics();
        void ExecuteEngageAttackBehaviorPhysics();
        void ExecuteIdleBehaviorPhysics() override;
        void ExecuteFragileBehaviorPhysics() override;
        void ExecuteDeadBehaviorPhysics() override;
        
        // 상태 진입 콜백
        void OnStateEntered(const std::string& state) override;
        
        // 공격
        void Attack(float deltaTime) override;
        
        // 행동 제한
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
