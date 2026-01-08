#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class SpritePlayer :
        public engine::Script<SpritePlayer>
    {
        REGISTER_COMPONENT(SpritePlayer)

    public:
        //void Awake() override;
        void Start() override;
        //void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        std::string GetType() const override;
    };
}