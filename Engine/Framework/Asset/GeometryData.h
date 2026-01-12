#pragma once

#include "Framework/Asset/AssetData.h"
#include "Common/Utility/CommonTypes.h"
#include "Core/Graphics/Data/Vertex.h"

namespace engine
{
    class GeometryData :
        public AssetData
    {
    private:
        std::vector<CommonVertex> m_vertices;
        std::vector<DWORD> m_indices;

    public:
        void Create(GeometryType type);

    public:
        const std::vector<CommonVertex>& GetVertices() const;
        const std::vector<DWORD>& GetIndices() const;
    };
}