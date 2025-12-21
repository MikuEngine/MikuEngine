#pragma once

#include "Framework/Asset/AssetData.h"

struct aiScene;

namespace engine
{
    enum class MaterialKey : std::uint64_t
    {
        DIFFUSE_TEXTURE            = 1ULL << 0,
        NORMAL_TEXTURE             = 1ULL << 1,
        SPECULAR_TEXTURE           = 1ULL << 2,
        EMISSIVE_TEXTURE           = 1ULL << 3,
        OPACITY_TEXTURE            = 1ULL << 4,
        METALNESS_TEXTURE          = 1ULL << 5,
        ROUGHNESS_TEXTURE          = 1ULL << 6,
        AMBIENT_OCCLUSION_TEXTURE  = 1ULL << 7,
        DIFFUSE_COLOR              = 1ULL << 8,
        AMBIENT_COLOR              = 1ULL << 9,
        SPECULAR_COLOR             = 1ULL << 10,
        EMISSIVE_COLOR             = 1ULL << 11,
        SHININESS_FACTOR           = 1ULL << 12,
        OPACITY_FACTOR             = 1ULL << 13,
    };

    struct Material
    {
        std::unordered_map<MaterialKey, std::string> texturePaths;
        std::unordered_map<MaterialKey, Vector4> vectorValues;
        std::unordered_map<MaterialKey, float> scalarValues;
        std::uint64_t materialFlags = 0;
    };

    class MaterialData :
        public AssetData
    {
    private:
        std::vector<Material> m_materials;

    public:
        void Create();
        void Create(const std::string& filePath);
        void Create(const aiScene* scene);

    public:
        const std::vector<Material>& GetMaterials() const;
    };
}