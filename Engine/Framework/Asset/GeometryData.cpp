#include "EnginePCH.h"
#include "GeometryData.h"

#include "Framework/Asset/GeometryGenerator.h"

namespace engine
{
    void GeometryData::Create(GeometryType type)
    {
        GeometryGenerator::Data data;

        switch (type)
        {
        case GeometryType::Quad:
            data = GeometryGenerator::MakeQuad(1.0f, 1.0f);
            break;

        case GeometryType::Cube:
            data = GeometryGenerator::MakeCube(1.0f);
            break;

        case GeometryType::Sphere:
            data = GeometryGenerator::MakeSphere(0.5f, 20, 20);
            break;

        case GeometryType::Plane:
            data = GeometryGenerator::MakePlane(1.0f, 1.0f);
            break;

        case GeometryType::Cone:
            data = GeometryGenerator::MakeCone(0.5f, 1.0f, 20);
            break;
        }

        m_vertices = std::move(data.vertices);
        m_indices = std::move(data.indices);
    }

    const std::vector<CommonVertex>& GeometryData::GetVertices() const
    {
        return m_vertices;
    }

    const std::vector<DWORD>& GeometryData::GetIndices() const
    {
        return m_indices;
    }
}