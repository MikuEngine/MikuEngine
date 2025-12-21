#pragma once

#include "Framework/Asset/AssetData.h"
#include "Core/Graphics/Data/Vertex.h"

struct aiScene;

namespace engine
{
    struct StaticMeshSection
    {
        std::string name;
        unsigned int materialIndex;
        INT vertexOffset;
        UINT indexOffset;
        UINT indexCount;
    };

    class StaticMeshData :
        public AssetData
    {
    private:
        std::vector<CommonVertex> m_vertices;
        std::vector<DWORD> m_indices;
        std::vector<StaticMeshSection> m_meshSections;

    public:
        void Create(const std::string& filePath);
        void Create(const aiScene* scene);
        void Create(std::vector<CommonVertex>&& vertices, std::vector<DWORD>&& indices);

    public:
        const std::vector<CommonVertex>& GetVertices() const;
        const std::vector<DWORD>& GetIndices() const;
        const std::vector<StaticMeshSection>& GetMeshSections() const;
    };
}