#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class TestScript :
        public engine::Script<TestScript>
    {
        REGISTER_COMPONENT(TestScript)
    private:
        float m_speed = 10.0f;

    public:
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        std::string GetType() const override;
    };
}