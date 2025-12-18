#pragma once

#include "Framework/Asset/AssetData.h"
#include "Framework/Asset/SkeletonData.h"

struct aiScene;

namespace engine
{
    struct PositionKey
    {
        float time;
        Vector3 value;
    };

    struct RotationKey
    {
        float time;
        DirectX::SimpleMath::Quaternion value;
    };

    struct ScaleKey
    {
        float time;
        DirectX::SimpleMath::Vector3 value;
    };

    struct LastKeyIndex;

    struct BoneAnimation
    {
        std::vector<PositionKey> positionKeys;
        std::vector<RotationKey> rotationKeys;
        std::vector<ScaleKey> scaleKeys;
        unsigned int boneIndex = 0;

        void Evaluate(
            float time,
            LastKeyIndex& inOutLastKeyIndex,
            Vector3& outPosition,
            Quaternion& outRotation,
            Vector3& outScale) const;
    };

    struct Animation
    {
        using BoneName = std::string;
        using BoneAnimIndex = unsigned int;

        std::string name;
        std::vector<BoneAnimation> boneAnimations;
        std::unordered_map<BoneName, BoneAnimIndex> animMappingTable;
        float duration;

        void SetupBoneAnimation(std::vector<Bone>& out) const;
    };

    class SkeletonData;

    class AnimationData :
        public AssetData
    {
    private:
        std::vector<Animation> m_animations;

    public:
        void Create(const aiScene* scene, const std::shared_ptr<SkeletonData>& skeletonData);

    public:
        const std::vector<Animation>& GetAnimations() const;
    };
}