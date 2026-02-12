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
        // 둔탁 타입은 StaticMesh를 사용하므로 애니메이션 이름 변수 불필요


        // ─────────────────────────────────────────────
        // 둔탁 보라
        // ─────────────────────────────────────────────
        bool m_hasOtherMonstersAlive = false;
        float m_purpleAliveCheckInterval = 1.0f;
        float m_purpleAliveCheckTimer = 0.0f;

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

        // BaseControllerScript 오버라이드
		void UpdateGameLogic() override;

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
        
        // 둔탁 보라
        bool CheckMonstersPurpleType();

        // 상태 진입 콜백
        void OnStateEntered(const std::string& state) override;

        // 외부 데미지 처리 (총알 등에서 호출)
        void TakeDamage(float damage, bool isShieldPierce = false) override;
        
        // 공격 (3초마다 리니어 총알 발사)
        void Attack(float deltaTime) override;

    public:
        // ─────────────────────────────────────────────
        // Parabolic 타입 여부 (Green = true)
        // ─────────────────────────────────────────────
        bool IsParabolicBullet() const override { return m_monsterTier == MonsterTier::Green; }
        
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
