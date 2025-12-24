#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class EditorCameraController :
        public engine::Script<EditorCameraController>
    {
    public:
        void Update() override;
    };
}