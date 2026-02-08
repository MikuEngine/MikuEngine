#pragma once

#include <Framework/Object/Component/Script.h>
#include "Script/Interface/IDamageable.h"

namespace game
{
    class BossScript;

    class BossPillar :
        public engine::Script<BossPillar>,
        public IDamageable
    {
        REGISTER_SCRIPT(BossPillar, Script)

    private:
        float m_hp = 30.0f;
        float m_maxHp = 30.0f;
        engine::Ptr<BossScript> m_boss;
        std::vector<engine::Ptr<engine::GameObject>> m_pillarCrystalizedPieces;

        bool m_isCrystalized = false;

    public:
        void Update() override;

        void TakeDamage(float damage, bool isShieldPierce = false) override;
        void Execute();

        bool IsCrystalized();

        void SetBoss(engine::Ptr<BossScript> boss) { m_boss = boss; }
        void SetHP(float hp) { m_hp = hp; m_maxHp = hp; }
        float GetHP() const { return m_hp; }
        float GetMaxHP() const { return m_maxHp; }

    private:
        void OnCrystalized();
    };
}
