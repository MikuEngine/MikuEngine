#include "EnginePCH.h"
#include "GeometryGenerator.h"

namespace engine
{
    constexpr float PI = 3.14159265359f;

    GeometryGenerator::Data GeometryGenerator::MakeQuad(float width, float height)
    {
        Data data;

        float w2 = width * 0.5f;
        float h2 = height * 0.5f;

        data.vertices.resize(4);

        Vector3 normal(0.0f, 0.0f, -1.0f);
        Vector3 tangent(1.0f, 0.0f, 0.0f);
        Vector3 binormal(0.0f, -1.0f, 0.0f); // Normal x Tangent = (0,0,-1)x(1,0,0) = (0,-1,0) (LHS)

        // 0: Bottom-Left
        data.vertices[0] = CommonVertex(
            Vector3(-w2, -h2, 0.0f), Vector2(0.0f, 1.0f), normal, tangent, binormal);
        // 1: Top-Left
        data.vertices[1] = CommonVertex(
            Vector3(-w2, h2, 0.0f), Vector2(0.0f, 0.0f), normal, tangent, binormal);
        // 2: Top-Right
        data.vertices[2] = CommonVertex(
            Vector3(w2, h2, 0.0f), Vector2(1.0f, 0.0f), normal, tangent, binormal);
        // 3: Bottom-Right
        data.vertices[3] = CommonVertex(
            Vector3(w2, -h2, 0.0f), Vector2(1.0f, 1.0f), normal, tangent, binormal);

        // 인덱스 (Clockwise)
        // Triangle 1: 0-1-2
        // Triangle 2: 0-2-3
        data.indices = { 0, 1, 2, 0, 2, 3 };

        return data;
    }

    GeometryGenerator::Data GeometryGenerator::MakeCube(float size)
    {
        float s = size * 0.5f;
        Data data;

        data.vertices =
        {
            // Normal Z -
            // Normal Z +
            // Normal X -
            // Normal X +
            // Normal Y -
            // Normal Y +

            // position, uv, normal, tangent, binormal
            { { -s,  s, -s }, { 0.0f, 0.0f }, {  0.0f,  0.0f, -1.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f, -1.0f,  0.0f } }, // 0 - 0
            { {  s,  s, -s }, { 1.0f, 0.0f }, {  0.0f,  0.0f, -1.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f, -1.0f,  0.0f } }, // 1 - 1
            { { -s, -s, -s }, { 0.0f, 1.0f }, {  0.0f,  0.0f, -1.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f, -1.0f,  0.0f } }, // 2 - 2
            { {  s, -s, -s }, { 1.0f, 1.0f }, {  0.0f,  0.0f, -1.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f, -1.0f,  0.0f } }, // 3 - 3

            { {  s,  s,  s }, { 0.0f, 0.0f }, {  0.0f,  0.0f,  1.0f }, { -1.0f, 0.0f,  0.0f }, { 0.0f, -1.0f,  0.0f } }, // 4 - 4
            { { -s,  s,  s }, { 1.0f, 0.0f }, {  0.0f,  0.0f,  1.0f }, { -1.0f, 0.0f,  0.0f }, { 0.0f, -1.0f,  0.0f } }, // 5 - 5
            { {  s, -s,  s }, { 0.0f, 1.0f }, {  0.0f,  0.0f,  1.0f }, { -1.0f, 0.0f,  0.0f }, { 0.0f, -1.0f,  0.0f } }, // 6 - 6
            { { -s, -s,  s }, { 1.0f, 1.0f }, {  0.0f,  0.0f,  1.0f }, { -1.0f, 0.0f,  0.0f }, { 0.0f, -1.0f,  0.0f } }, // 7 - 7

            { { -s,  s,  s }, { 0.0f, 0.0f }, { -1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f, -1.0f }, { 0.0f, -1.0f,  0.0f } }, // 5 - 8
            { { -s,  s, -s }, { 1.0f, 0.0f }, { -1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f, -1.0f }, { 0.0f, -1.0f,  0.0f } }, // 0 - 9
            { { -s, -s,  s }, { 0.0f, 1.0f }, { -1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f, -1.0f }, { 0.0f, -1.0f,  0.0f } }, // 7 - 10
            { { -s, -s, -s }, { 1.0f, 1.0f }, { -1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f, -1.0f }, { 0.0f, -1.0f,  0.0f } }, // 2 - 11

            { {  s,  s, -s }, { 0.0f, 0.0f }, {  1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f,  1.0f }, { 0.0f, -1.0f,  0.0f } }, // 1 - 12
            { {  s,  s,  s }, { 1.0f, 0.0f }, {  1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f,  1.0f }, { 0.0f, -1.0f,  0.0f } }, // 4 - 13
            { {  s, -s, -s }, { 0.0f, 1.0f }, {  1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f,  1.0f }, { 0.0f, -1.0f,  0.0f } }, // 3 - 14
            { {  s, -s,  s }, { 1.0f, 1.0f }, {  1.0f,  0.0f,  0.0f }, {  0.0f, 0.0f,  1.0f }, { 0.0f, -1.0f,  0.0f } }, // 6 - 15

            { { -s, -s, -s }, { 0.0f, 0.0f }, {  0.0f, -1.0f,  0.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f,  0.0f,  1.0f } }, // 2 - 16
            { {  s, -s, -s }, { 1.0f, 0.0f }, {  0.0f, -1.0f,  0.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f,  0.0f,  1.0f } }, // 3 - 17
            { { -s, -s,  s }, { 0.0f, 1.0f }, {  0.0f, -1.0f,  0.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f,  0.0f,  1.0f } }, // 7 - 18
            { {  s, -s,  s }, { 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f,  0.0f,  1.0f } }, // 6 - 19

            { { -s,  s,  s }, { 0.0f, 0.0f }, {  0.0f,  1.0f,  0.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f,  0.0f, -1.0f } }, // 5 - 20
            { {  s,  s,  s }, { 1.0f, 0.0f }, {  0.0f,  1.0f,  0.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f,  0.0f, -1.0f } }, // 4 - 21
            { { -s,  s, -s }, { 0.0f, 1.0f }, {  0.0f,  1.0f,  0.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f,  0.0f, -1.0f } }, // 0 - 22
            { {  s,  s, -s }, { 1.0f, 1.0f }, {  0.0f,  1.0f,  0.0f }, {  1.0f, 0.0f,  0.0f }, { 0.0f,  0.0f, -1.0f } }, // 1 - 23
        };

        data.indices =
        {
            0, 1, 2,
            2, 1, 3,

            4, 5, 6,
            6, 5, 7,

            8, 9, 10,
            10, 9, 11,

            12, 13, 14,
            14, 13, 15,

            16, 17, 18,
            18, 17, 19,

            20, 21, 22,
            22, 21, 23,
        };

        return data;
    }

    GeometryGenerator::Data GeometryGenerator::MakeSphere(float radius, int sliceCount, int stackCount)
    {
        Data data;

        // 정점 생성
        // 북극에서 남극으로 내려오면서 링(Ring) 생성
        CommonVertex topVertex(Vector3(0, radius, 0), Vector2(0, 0), Vector3(0, 1, 0), Vector3(1, 0, 0), Vector3(0, 0, 1));
        CommonVertex bottomVertex(Vector3(0, -radius, 0), Vector2(0, 1), Vector3(0, -1, 0), Vector3(1, 0, 0), Vector3(0, 0, -1));

        data.vertices.push_back(topVertex);

        float phiStep = PI / stackCount;
        float thetaStep = 2.0f * PI / sliceCount;

        // 북극, 남극 제외한 링들
        for (int i = 1; i <= stackCount - 1; ++i)
        {
            float phi = i * phiStep;

            for (int j = 0; j <= sliceCount; ++j)
            {
                float theta = j * thetaStep;

                // 구면 좌표계 -> 직교 좌표계 (왼손, Y Up)
                float x = radius * sinf(phi) * cosf(theta);
                float y = radius * cosf(phi);
                float z = radius * sinf(phi) * sinf(theta);

                // Normal, Tangent, Binormal
                Vector3 pos(x, y, z);
                Vector3 normal = pos;
                // Normalize logic needed if Vector3 doesn't normalize automatically
                float len = sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
                normal.x /= len; normal.y /= len; normal.z /= len;

                // Tangent: theta(위도)에 대한 미분. (-sin(theta), 0, cos(theta))
                Vector3 tangent(-sinf(theta), 0.0f, cosf(theta));
                // Binormal: Normal x Tangent
                Vector3 binormal;
                binormal.x = normal.y * tangent.z - normal.z * tangent.y;
                binormal.y = normal.z * tangent.x - normal.x * tangent.z;
                binormal.z = normal.x * tangent.y - normal.y * tangent.x;

                Vector2 uv(theta / (2.0f * PI), phi / PI);

                data.vertices.emplace_back(pos, uv, normal, tangent, binormal);
            }
        }

        data.vertices.push_back(bottomVertex);

        // 인덱스 생성
        // 1. 북극 (Top cap)
        for (int i = 1; i <= sliceCount; ++i)
        {
            data.indices.push_back(0);
            data.indices.push_back(i + 1);
            data.indices.push_back(i);
        }

        // 2. 중간 링 (Stacks)
        int baseIndex = 1;
        int ringVertexCount = sliceCount + 1; // 텍스처 랩핑 때문에 시작점과 끝점이 겹침

        for (int i = 0; i < stackCount - 2; ++i)
        {
            for (int j = 0; j < sliceCount; ++j)
            {
                data.indices.push_back(baseIndex + i * ringVertexCount + j);
                data.indices.push_back(baseIndex + i * ringVertexCount + j + 1);
                data.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

                data.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
                data.indices.push_back(baseIndex + i * ringVertexCount + j + 1);
                data.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
            }
        }

        // 3. 남극 (Bottom cap)
        int southPoleIndex = (int)data.vertices.size() - 1;
        baseIndex = southPoleIndex - ringVertexCount;

        for (int i = 0; i < sliceCount; ++i)
        {
            data.indices.push_back(southPoleIndex);
            data.indices.push_back(baseIndex + i);
            data.indices.push_back(baseIndex + i + 1);
        }

        return data;
    }

    GeometryGenerator::Data GeometryGenerator::MakePlane(float width, float depth)
    {
        // MakeQuad와 유사하지만 XZ 평면에 눕힘 (Grid)
        Data data;

        float w2 = width * 0.5f;
        float d2 = depth * 0.5f;

        data.vertices.resize(4);

        Vector3 normal(0.0f, 1.0f, 0.0f); // Up
        Vector3 tangent(1.0f, 0.0f, 0.0f); // Right
        Vector3 binormal(0.0f, 0.0f, 1.0f); // Forward

        // 0: Bottom-Left (Min X, Min Z)
        data.vertices[0] = CommonVertex(
            Vector3(-w2, 0.0f, -d2), Vector2(0.0f, 1.0f), normal, tangent, binormal);
        // 1: Top-Left (Min X, Max Z)
        data.vertices[1] = CommonVertex(
            Vector3(-w2, 0.0f, d2), Vector2(0.0f, 0.0f), normal, tangent, binormal);
        // 2: Top-Right (Max X, Max Z)
        data.vertices[2] = CommonVertex(
            Vector3(w2, 0.0f, d2), Vector2(1.0f, 0.0f), normal, tangent, binormal);
        // 3: Bottom-Right (Max X, Min Z)
        data.vertices[3] = CommonVertex(
            Vector3(w2, 0.0f, -d2), Vector2(1.0f, 1.0f), normal, tangent, binormal);

        // Indices (Clockwise)
        // 0-1-2, 0-2-3
        data.indices = { 0, 1, 2, 0, 2, 3 };

        return data;
    }

    GeometryGenerator::Data GeometryGenerator::MakeCone(float radius, float height, int sliceCount)
    {
        Data data;
        // 원뿔은 옆면(Conical surface)과 바닥면(Disk)으로 구성됨.

        // 1. 옆면 (Sides)
        float halfHeight = height * 0.5f;

        // 옆면 생성
        float thetaStep = 2.0f * PI / sliceCount;

        // 옆면의 각도 계산 (Normal, Tangent 계산용)
        float slope = radius / height; // tan(alpha)
        float cosAlpha = height / sqrt(height * height + radius * radius);
        float sinAlpha = radius / sqrt(height * height + radius * radius);

        for (int i = 0; i <= sliceCount; ++i)
        {
            float theta = i * thetaStep;
            float c = cosf(theta);
            float s = sinf(theta);

            // 하단 링의 위치
            Vector3 bottomPos(radius * c, -halfHeight, radius * s);

            // 텍스처 좌표
            float u = (float)i / sliceCount;

            Vector3 n(c * cosAlpha, sinAlpha, s * cosAlpha);
            Vector3 t(-s, 0.0f, c);
            Vector3 b; // N x T
            b.x = n.y * t.z - n.z * t.y; b.y = n.z * t.x - n.x * t.z; b.z = n.x * t.y - n.y * t.x;

            data.vertices.emplace_back(Vector3(0, halfHeight, 0), Vector2(u, 0.0f), n, t, b);

            data.vertices.emplace_back(bottomPos, Vector2(u, 1.0f), n, t, b);
        }

        // 옆면 인덱스
        for (int i = 0; i < sliceCount; ++i)
        {
            // 짝수 인덱스는 Tip, 홀수 인덱스는 Bottom
            int tip = i * 2;
            int bot = i * 2 + 1;
            int nextBot = (i * 2 + 3);

            data.indices.push_back(tip);
            data.indices.push_back(nextBot);
            data.indices.push_back(bot);
        }

        // 2. 바닥면 (Bottom Cap)
        int baseCenterIndex = (int)data.vertices.size();

        // 바닥 중심점
        data.vertices.emplace_back(Vector3(0, -halfHeight, 0), Vector2(0.5f, 0.5f),
            Vector3(0, -1, 0), Vector3(1, 0, 0), Vector3(0, 0, -1));

        // 바닥 링 정점 (Normal이 다름 - 바닥은 평평하므로)
        for (int i = 0; i <= sliceCount; ++i)
        {
            float theta = i * thetaStep;
            float c = cosf(theta);
            float s = sinf(theta);

            Vector3 pos(radius * c, -halfHeight, radius * s);
            // 바닥 UV는 원형 매핑
            Vector2 uv(0.5f + 0.5f * c, 0.5f + 0.5f * s);

            data.vertices.emplace_back(pos, uv, Vector3(0, -1, 0), Vector3(1, 0, 0), Vector3(0, 0, -1));
        }

        // 바닥 인덱스
        for (int i = 0; i < sliceCount; ++i)
        {
            data.indices.push_back(baseCenterIndex);
            data.indices.push_back(baseCenterIndex + 1 + i);
            data.indices.push_back(baseCenterIndex + 1 + i + 1);
        }

        return data;
    }
}