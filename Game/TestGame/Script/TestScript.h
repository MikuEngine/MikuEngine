#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class TestScript :
        public engine::Script<TestScript>
    {
    public:
        void Start() override;
        void Update() override;
    };
}