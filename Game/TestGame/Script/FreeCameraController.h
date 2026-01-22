#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class FreeCameraController :
        public engine::Script<FreeCameraController>
    {
        REGISTER_COMPONENT(FreeCameraController, Script)
    private:
        float m_moveSpeed = 10.0f;
        float m_rotationSpeed = 0.1f;

    public:
        //void Awake() override;
        //void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}