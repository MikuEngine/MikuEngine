#pragma once

#include "Vertex.h"

namespace engine
{
    struct ParticleStructuredData
    {
        Vector3 position;
        float size;

        Vector4 color;

        Vector3 emissiveColor;
        float emissiveIntensity;

        Vector2 uvOffset;
        Vector2 uvScale;

        float rotation;
        Vector3 __pad;
    };
}