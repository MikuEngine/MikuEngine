#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class StaticMeshRenderer;
}

namespace game
{
    class BossShieldEffect :
        public engine::Script<BossShieldEffect>
    {
        REGISTER_SCRIPT(BossShieldEffect, Script)

    private:
        engine::StaticMeshRenderer* m_shieldMesh = nullptr;
        float m_pulseTimer = 0.0f;
        float m_pulseSpeed = 2.0f;  // 펄스 속도

    public:
        void Awake() override;
        void Update() override;
    };
}
