#pragma once

#include "AssetData.h"

namespace engine
{
    class StaticMeshData;
    class MaterialData;
    class SkeletalMeshData;
    class SkeletonData;
    class AnimationData;

    enum class FBXAssetKind
    {
        Static,
        Skeletal
    };

    class FBXAssetData :
        public AssetData
    {
    private:
        FBXAssetKind m_kind = FBXAssetKind::Static;
        std::shared_ptr<StaticMeshData> m_staticMesh;
        std::shared_ptr<MaterialData> m_material;
        std::shared_ptr<SkeletalMeshData> m_skeletalMesh;
        std::shared_ptr<SkeletonData> m_skeleton;
        std::shared_ptr<AnimationData> m_animation;

    public:
        void Create(FBXAssetKind kind, const std::string& filePath);

        std::shared_ptr<StaticMeshData> GetStaticMeshData() const;
        std::shared_ptr<MaterialData> GetMaterialData() const;
        std::shared_ptr<SkeletalMeshData> GetSkeletalMeshData() const;
        std::shared_ptr<SkeletonData> GetSkeletonData() const;
        std::shared_ptr<AnimationData> GetAnimationData() const;

    private:
        void LoadStaticMesh(const std::string& filePath);
        void LoadSkeletalMesh(const std::string& filePath);
    };
}