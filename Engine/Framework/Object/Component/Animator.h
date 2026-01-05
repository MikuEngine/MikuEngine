#pragma once

#include "Framework/Object/Component/Component.h"

namespace engine
{
    class Animator :
        public Component
    {
    public:
        ~Animator();

    public:
        void Initialize() override;
        virtual void Update() = 0;
    };
}