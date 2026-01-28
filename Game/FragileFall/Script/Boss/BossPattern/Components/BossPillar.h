#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class BossScript;

    class BossPillar :
        public engine::Script<BossPillar>
    {
        REGISTER_SCRIPT(BossPillar, Script)

    private:
        int m_hp = 100;
        int m_maxHp = 100;
        engine::Ptr<BossScript> m_boss;
        bool m_isDestroyed = false;

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

        void TakeDamage(int damage);
        void OnDestroyed();
        bool IsDestroyed() const { return m_isDestroyed; }

        void SetBoss(engine::Ptr<BossScript> boss) { m_boss = boss; }
        void SetHP(int hp) { m_hp = hp; m_maxHp = hp; }
        int GetHP() const { return m_hp; }
        int GetMaxHP() const { return m_maxHp; }
    };
}
