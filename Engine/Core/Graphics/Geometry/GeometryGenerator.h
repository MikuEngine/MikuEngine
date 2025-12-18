#pragma once

#include "Core/Graphics/Resource/Mesh.h"

namespace engine
{
    class GeometryGenerator
    {
    public:
        static MeshData CreateQuad(float w, float h);
        static MeshData CreateFullscreenQuad();
    };
}
