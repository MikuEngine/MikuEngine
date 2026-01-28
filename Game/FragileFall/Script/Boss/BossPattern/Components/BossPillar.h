#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class BossScript;

    // ═══════════════════════════════════════════════════════════════
    // BossPillar - 기둥 오브젝트
    // 
    // 목적:
    //   - 기둥 쉴드 패턴에서 생성되는 기둥
    //   - 체력 관리 및 파괴 시 보스에 알림
    // ═══════════════════════════════════════════════════════════════
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
