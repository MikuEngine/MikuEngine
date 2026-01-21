#pragma once

namespace engine
{
    enum class AssetType
    {
        StaticMesh,
        SkeletalMesh,
        Animation,
        Material,
        Skeleton,
        Prefab,
        None
    };

    class AssetData
    {
    public:
        virtual ~AssetData() = default;
        // type?
    };
}