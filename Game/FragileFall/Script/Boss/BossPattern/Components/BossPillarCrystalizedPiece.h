#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class BossPillarCrystalizedPiece :
        public engine::Script<BossPillarCrystalizedPiece>
    {
        REGISTER_SCRIPT(BossPillarCrystalizedPiece, Script)

    private:
        float m_progress = 0.0f;
        float m_startYScale = 0.0f;
        float m_targetYScale = 2.0f;

    public:
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}