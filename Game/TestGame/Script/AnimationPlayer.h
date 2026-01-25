#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class AnimationPlayer :
        public engine::Script<AnimationPlayer>
    {
        REGISTER_COMPONENT(AnimationPlayer, Script)

    private:
        engine::Quaternion m_upperBodyRotation = engine::Quaternion::Identity;
        float m_currentPitch = 0.0f; // 상하 (X축 회전)
        float m_currentYaw = 0.0f;   // 좌우 (Y축 회전)

    public:
		void Awake() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}