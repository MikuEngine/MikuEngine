#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class ParticleAttachment :
        public engine::Script<ParticleAttachment>
    {
        REGISTER_SCRIPT(ParticleAttachment, Script)

    private:
        engine::Ptr<engine::GameObject> m_target = nullptr;
        bool m_isTargetDestroyed = false;

    public:
        void LateUpdate() override;
        void SetTarget(engine::GameObject* target);

    public:
        // void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    };
}