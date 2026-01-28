#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class StaticMeshRenderer;
}

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // BossShieldEffect - 쉴드 이펙트
    // 
    // 목적:
    //   - 기둥이 살아있을 때 보스 주변에 표시되는 쉴드 이펙트
    //   - 쉴드 활성화/비활성화 시각적 피드백
    // ═══════════════════════════════════════════════════════════════
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
