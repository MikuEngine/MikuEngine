#pragma once

#include "Framework/Object/Component/Component.h"

namespace engine
{
    enum class LightType
    {
        Global,
        Point,
        Spot
    };

    class Light :
        public Component
    {
        //REGISTER_COMPONENT(Light)

    private:
        LightType m_type = LightType::Global;
        Vector3 m_color{ 1.0f, 1.0f, 1.0f };
        float m_intensity = 1.0f;
        float m_range = 10.0f;
        float m_spotAngle = 45.0f;
    };
}