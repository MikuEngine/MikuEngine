#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine { class AfterimageRenderer; }

namespace game
{
    class TrailDashTest :
        public engine::Script<TrailDashTest>
    {
        REGISTER_SCRIPT(TrailDashTest, Script)

    public:
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        engine::AfterimageRenderer* m_afterimage = nullptr;
        bool m_isDashing = false;
        float m_dashTimer = 0.0f;
        float m_dashDuration = 0.2f;
        float m_dashSpeed = 12.0f;
        engine::Vector3 m_dashDirection = engine::Vector3(0.0f, 0.0f, 1.0f);
    };
}