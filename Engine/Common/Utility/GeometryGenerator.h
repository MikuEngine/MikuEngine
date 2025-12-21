#pragma once

#include "Core/Graphics/Data/Vertex.h"

namespace engine
{
    struct MeshData
    {
        std::vector<CommonVertex> vertices;
        std::vector<DWORD> indices;
    };

    class GeometryGenerator
    {
    public:
        static MeshData CreateCube();
        static MeshData CreateQuad();
        static MeshData CreateFullscreenQuad();
    };
}
