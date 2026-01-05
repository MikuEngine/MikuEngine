#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class AnimationPlayer :
        public engine::Script<AnimationPlayer>
    {
        REGISTER_COMPONENT(AnimationPlayer)

    public:
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        std::string GetType() const override;
    };
}