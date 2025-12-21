#include "pch.h"
#include "GeometryGenerator.h"


namespace engine
{
    MeshData GeometryGenerator::CreateCube()
    {
        std::vector<CommonVertex> vertices;
        std::vector<DWORD> indices;
        vertices.reserve(24);
        indices.reserve(36);
        // [앞면] Normal(0,0,-1), Tangent(1,0,0)
        vertices.push_back({ Vector3(-0.5f,  0.5f, -0.5f), Vector2(0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f) });
        vertices.push_back({ Vector3(0.5f,  0.5f, -0.5f), Vector2(1.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f) });
        vertices.push_back({ Vector3(0.5f, -0.5f, -0.5f), Vector2(1.0f, 1.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f) });
        vertices.push_back({ Vector3(-0.5f, -0.5f, -0.5f), Vector2(0.0f, 1.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f) });
        // [뒷면] Normal(0,0,1), Tangent(-1,0,0)
        vertices.push_back({ Vector3(0.5f,  0.5f,  0.5f), Vector2(0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f) });
        vertices.push_back({ Vector3(-0.5f,  0.5f,  0.5f), Vector2(1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f) });
        vertices.push_back({ Vector3(-0.5f, -0.5f,  0.5f), Vector2(1.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f) });
        vertices.push_back({ Vector3(0.5f, -0.5f,  0.5f), Vector2(0.0f, 1.0f), Vector3(0.0f, 0.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f) });
        // [윗면] Normal(0,1,0), Tangent(1,0,0)
        vertices.push_back({ Vector3(-0.5f,  0.5f,  0.5f), Vector2(0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f) });
        vertices.push_back({ Vector3(0.5f,  0.5f,  0.5f), Vector2(1.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f) });
        vertices.push_back({ Vector3(0.5f,  0.5f, -0.5f), Vector2(1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f) });
        vertices.push_back({ Vector3(-0.5f,  0.5f, -0.5f), Vector2(0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f) });
        // [아랫면] Normal(0,-1,0), Tangent(1,0,0)
        vertices.push_back({ Vector3(-0.5f, -0.5f, -0.5f), Vector2(0.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) });
        vertices.push_back({ Vector3(0.5f, -0.5f, -0.5f), Vector2(1.0f, 0.0f), Vector3(0.0f, -1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) });
        vertices.push_back({ Vector3(0.5f, -0.5f,  0.5f), Vector2(1.0f, 1.0f), Vector3(0.0f, -1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) });
        vertices.push_back({ Vector3(-0.5f, -0.5f,  0.5f), Vector2(0.0f, 1.0f), Vector3(0.0f, -1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) });
        // [왼쪽면] Normal(-1,0,0), Tangent(0,0,-1)
        vertices.push_back({ Vector3(-0.5f,  0.5f,  0.5f), Vector2(0.0f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 1.0f, 0.0f) });
        vertices.push_back({ Vector3(-0.5f,  0.5f, -0.5f), Vector2(1.0f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 1.0f, 0.0f) });
        vertices.push_back({ Vector3(-0.5f, -0.5f, -0.5f), Vector2(1.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 1.0f, 0.0f) });
        vertices.push_back({ Vector3(-0.5f, -0.5f,  0.5f), Vector2(0.0f, 1.0f), Vector3(-1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 1.0f, 0.0f) });
        // [오른쪽면] Normal(1,0,0), Tangent(0,0,1)
        vertices.push_back({ Vector3(0.5f,  0.5f, -0.5f), Vector2(0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f) });
        vertices.push_back({ Vector3(0.5f,  0.5f,  0.5f), Vector2(1.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f) });
        vertices.push_back({ Vector3(0.5f, -0.5f,  0.5f), Vector2(1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f) });
        vertices.push_back({ Vector3(0.5f, -0.5f, -0.5f), Vector2(0.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f) });
        // 인덱스 (Clockwise Winding)
        // 6개의 면에 대해 동일한 패턴 적용
        for (int i = 0; i < 6; ++i)
        {
            indices.push_back(i * 4 + 0);
            indices.push_back(i * 4 + 1);
            indices.push_back(i * 4 + 2);
            indices.push_back(i * 4 + 0);
            indices.push_back(i * 4 + 2);
            indices.push_back(i * 4 + 3);
        }
        return MeshData{ std::move(vertices), std::move(indices) };
    }

    MeshData GeometryGenerator::CreateQuad()
    {
        std::vector<CommonVertex> vertices = {
            { { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f} },
            { {  0.5f,  0.5f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f} },
            { { -0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f} },
            { {  0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f} }
        };

        std::vector<DWORD> indices = {
            0, 1, 2,
            2, 1, 3
        };

        return { std::move(vertices), std::move(indices) };
    }

    MeshData GeometryGenerator::CreateFullscreenQuad()
    {
        std::vector<CommonVertex> vertices = {
            { { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f} },
            { {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f} },
            { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f} },
            { {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f} }
        };

        std::vector<DWORD> indices = {
            0, 1, 2,
            2, 1, 3
        };

        return { std::move(vertices), std::move(indices) };
    }
}
