#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class Rotator :
        public engine::Script<Rotator>
    {
        REGISTER_COMPONENT(Rotator)

    private:
        float m_speed = 0.0f;

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        std::string GetType() const override;
    };
}