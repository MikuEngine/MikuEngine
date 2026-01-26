#pragma once

#include "Framework/Asset/AssetData.h"

struct aiScene;

namespace engine
{
    enum class MaterialKey : std::uint64_t
    {
        BASE_COLOR_TEXTURE         = 1ULL << 0,
        NORMAL_TEXTURE             = 1ULL << 1,
        EMISSIVE_TEXTURE           = 1ULL << 2,
        METALNESS_TEXTURE          = 1ULL << 3,
        ROUGHNESS_TEXTURE          = 1ULL << 4,
        AMBIENT_OCCLUSION_TEXTURE  = 1ULL << 5,
    };

    enum class MaterialRenderType
    {
        Opaque,
        Cutout,
        Transparent
    };

    struct Material
    {
        std::unordered_map<MaterialKey, std::string> texturePaths;
        std::uint64_t materialFlags = 0;
        MaterialRenderType renderType = MaterialRenderType::Opaque;
    };

    class MaterialData :
        public AssetData
    {
    private:
        std::vector<Material> m_materials;

    public:
        void Create();
        void Create(const std::string& filePath);
        void Create(const aiScene* scene, const std::string& filePath = "");

    public:
        const std::vector<Material>& GetMaterials() const;
    };
}