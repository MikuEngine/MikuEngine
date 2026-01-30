#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterRoundType - 뾰족 공격 몬스터 구현
    // 
    // 특징:
    //   - 유도탄이나 특수 패턴을 가진 이동형 AI
    // ═══════════════════════════════════════════════════════════════

    class MonsterRoundType :
        public engine::Script<MonsterRoundType>
    {
        REGISTER_SCRIPT(MonsterRoundType, Script)

    public:
        //void Awake() override;
        //void Start() override;
        //void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}