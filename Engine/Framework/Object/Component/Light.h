#pragma once

#include "Framework/Object/Component/Component.h"

namespace engine
{
    enum class LightType
    {
        Directional,
        Point,
        Spot
    };

    class Light :
        public Component
    {
        //REGISTER_COMPONENT(Light)
    };
}