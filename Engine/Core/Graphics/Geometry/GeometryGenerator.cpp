#include "pch.h"
#include "GeometryGenerator.h"

namespace engine
{
    MeshData GeometryGenerator::CreateQuad(float w, float h)
    {
        MeshData meshData;

        float hw = w * 0.5f;
        float hh = h * 0.5f;

        meshData.vertices = {
            // Position                // TexCoord    // Normal
            { { -hw,  hh, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
            { {  hw,  hh, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
            { { -hw, -hh, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } },
            { {  hw, -hh, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } }
        };

        meshData.indices = {
            0, 1, 2,
            2, 1, 3
        };

        return meshData;
    }

    MeshData GeometryGenerator::CreateFullscreenQuad()
    {
        MeshData meshData;

        // Normalized Device Coordinates (NDC) covers -1 to 1.
        // 1. Position: Full screen (-1 ~ 1)
        // 2. TexCoord: 0 ~ 1
        meshData.vertices = {
            // Position                // TexCoord    // Normal (Not used for blit)
            { { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
            { {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f } },
            { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } },
            { {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } }
        };

        meshData.indices = {
            0, 1, 2,
            2, 1, 3
        };

        return meshData;
    }
}
