#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class EditorCameraController :
        public engine::Script<EditorCameraController>
    {
        REGISTER_COMPONENT(EditorCameraController)
    public:
        void Update() override;

        void Save(engine::json& j) const override {}
        void Load(const engine::json& j) override {}
        std::string GetType() const override;;
    };
}