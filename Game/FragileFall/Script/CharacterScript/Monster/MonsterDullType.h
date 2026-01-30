#pragma once

#include "MonsterScript.h"
#include "Script/CharacterScript/Common/BulletParams.h"

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterDulltype - 둔탁 공격 몬스터 구현 
    // 
    // 특징:
    //   - 제자리에 고정되어 플레이어를 공격
    // ═══════════════════════════════════════════════════════════════

    class MonsterDullType : public MonsterScript
    {
        REGISTER_SCRIPT(MonsterDullType, MonsterScript)

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
        std::string m_animName_Engage = "Engage";
        std::string m_animName_Fragile = "Fragile";
        std::string m_animName_Dead = "Dead";
        std::string m_animName_Attack = "Attack";

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
        
        // 상태별 행동 - 비물리 (Update에서 호출)
        void UpdateStateBasedBehavior(const std::string& state, float deltaTime) override;
        void ExecuteEngageBehaviorNonPhysics(float deltaTime) override;
        void ExecuteIdleBehaviorNonPhysics() override;
        void ExecuteFragileBehaviorNonPhysics() override;
        void ExecuteDeadBehaviorNonPhysics() override;
        
        // 상태별 행동 - 물리 (FixedUpdate에서 호출)
        void UpdatePhysicsStateBasedBehavior(const std::string& state) override;
        void ExecuteEngageBehaviorPhysics() override;
        void ExecuteIdleBehaviorPhysics() override;
        void ExecuteFragileBehaviorPhysics() override;
        void ExecuteDeadBehaviorPhysics() override;
        
        // 상태 진입 콜백
        void OnStateEntered(const std::string& state) override;
        
        // 공격 (3초마다 리니어 총알 발사)
        void Attack(float deltaTime) override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
