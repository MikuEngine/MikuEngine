#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class MonsterGenerateTest :
        public engine::Script<MonsterGenerateTest>
    {
        REGISTER_SCRIPT(MonsterGenerateTest, Script)

    public:
        //void Awake() override;
        void Start() override;
        //void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}