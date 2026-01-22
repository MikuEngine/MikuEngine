#pragma once

#include "Framework/System/System.h"
#include "Framework/Object/Component/Animator/Animator.h"

namespace engine
{
    class AnimatorSystem :
        public System<Animator>
    {
    public:
        void Update();
    };
}