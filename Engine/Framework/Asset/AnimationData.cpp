#include "EnginePCH.h"
#include "AnimationData.h"

#include <assimp/scene.h>
#include <assimp/anim.h>

namespace engine
{
    void BoneAnimation::Evaluate(
        float time,
        LastKeyIndex& inOutLastKeyIndex,
        Vector3& outPosition,
        Quaternion& outRotation,
        Vector3& outScale) const
    {
        // position
        size_t positionKeyIndex = inOutLastKeyIndex.position;

        if (positionKeys[positionKeyIndex].time >= time)
        {
            positionKeyIndex = 0;
        }

        outPosition = positionKeys[positionKeyIndex].value;

        while (true)
        {
            size_t nextIndex = positionKeyIndex + 1;
            if (nextIndex >= positionKeys.size())
            {
                break;
            }

            if (positionKeys[nextIndex].time > time)
            {
                outPosition = DirectX::SimpleMath::Vector3::Lerp(
                    positionKeys[positionKeyIndex].value,
                    positionKeys[nextIndex].value,
                    (time - positionKeys[positionKeyIndex].time) / (positionKeys[nextIndex].time - positionKeys[positionKeyIndex].time));
                inOutLastKeyIndex.position = positionKeyIndex;

                break;
            }

            positionKeyIndex = nextIndex;
            outPosition = positionKeys[positionKeyIndex].value;
        }

        // rotation
        size_t rotationKeyIndex = inOutLastKeyIndex.rotation;

        if (rotationKeys[rotationKeyIndex].time >= time)
        {
            rotationKeyIndex = 0;
        }

        outRotation = rotationKeys[rotationKeyIndex].value;

        while (true)
        {
            size_t nextIndex = rotationKeyIndex + 1;
            if (nextIndex >= rotationKeys.size())
            {
                break;
            }

            if (rotationKeys[nextIndex].time > time)
            {
                outRotation = DirectX::SimpleMath::Quaternion::Slerp(
                    rotationKeys[rotationKeyIndex].value,
                    rotationKeys[nextIndex].value,
                    (time - (rotationKeys[rotationKeyIndex].time)) / (rotationKeys[nextIndex].time - rotationKeys[rotationKeyIndex].time)
                );

                inOutLastKeyIndex.rotation = rotationKeyIndex;

                break;
            }

            rotationKeyIndex = nextIndex;
            outRotation = rotationKeys[rotationKeyIndex].value;
        }

        // scale
        size_t scaleKeyIndex = inOutLastKeyIndex.scale;

        if (scaleKeys[scaleKeyIndex].time >= time)
        {
            scaleKeyIndex = 0;
        }

        outScale = scaleKeys[scaleKeyIndex].value;

        while (true)
        {
            size_t nextIndex = scaleKeyIndex + 1;
            if (nextIndex >= scaleKeys.size())
            {
                break;
            }

            if (scaleKeys[nextIndex].time > time)
            {
                outScale = DirectX::SimpleMath::Vector3::Lerp(
                    scaleKeys[scaleKeyIndex].value,
                    scaleKeys[nextIndex].value,
                    (time - (scaleKeys[scaleKeyIndex].time)) / (scaleKeys[nextIndex].time - scaleKeys[scaleKeyIndex].time)
                );

                inOutLastKeyIndex.scale = scaleKeyIndex;

                break;
            }

            scaleKeyIndex = nextIndex;
            outScale = scaleKeys[scaleKeyIndex].value;
        }
    }

    void Animation::SetupBoneAnimation(std::vector<Bone>& out) const
    {
        for (auto& bone : out)
        {
            bone.lastKeyIndex = LastKeyIndex{};

            auto find = animMappingTable.find(bone.name);
            if (find != animMappingTable.end())
            {
                bone.boneAnimation = &boneAnimations[find->second];
            }
            else
            {
                bone.boneAnimation = nullptr;
            }
        }
    }

    void AnimationData::Create(const aiScene* scene, const std::shared_ptr<SkeletonData>& skeletonData)
    {
        m_animations.reserve(scene->mNumAnimations);

        for (unsigned int k = 0; k < scene->mNumAnimations; ++k)
        {
            const aiAnimation* aiAnim = scene->mAnimations[k];

            Animation animation{};

            animation.name = aiAnim->mName.C_Str();
            animation.duration = static_cast<float>(aiAnim->mDuration / aiAnim->mTicksPerSecond);
            animation.boneAnimations.resize(aiAnim->mNumChannels);

            for (unsigned int i = 0; i < aiAnim->mNumChannels; ++i)
            {
                const aiNodeAnim* anim = aiAnim->mChannels[i];
                const std::string boneName{ anim->mNodeName.C_Str() };

                animation.boneAnimations[i].boneIndex = skeletonData->GetBoneIndexByBoneName(boneName);
                animation.animMappingTable[boneName] = i;

                animation.boneAnimations[i].positionKeys.reserve(anim->mNumPositionKeys);
                animation.boneAnimations[i].rotationKeys.reserve(anim->mNumRotationKeys);
                animation.boneAnimations[i].scaleKeys.reserve(anim->mNumScalingKeys);

                for (unsigned int j = 0; j < anim->mNumPositionKeys; ++j)
                {
                    animation.boneAnimations[i].positionKeys.emplace_back(
                        static_cast<float>(anim->mPositionKeys[j].mTime / aiAnim->mTicksPerSecond),
                        Vector3(anim->mPositionKeys[j].mValue.x,
                            anim->mPositionKeys[j].mValue.y,
                            anim->mPositionKeys[j].mValue.z));
                }

                for (unsigned int j = 0; j < anim->mNumRotationKeys; ++j)
                {
                    animation.boneAnimations[i].rotationKeys.emplace_back(
                        static_cast<float>(anim->mRotationKeys[j].mTime / aiAnim->mTicksPerSecond),
                        Quaternion(anim->mRotationKeys[j].mValue.x,
                            anim->mRotationKeys[j].mValue.y,
                            anim->mRotationKeys[j].mValue.z,
                            anim->mRotationKeys[j].mValue.w));
                }

                for (unsigned int j = 0; j < anim->mNumScalingKeys; ++j)
                {
                    animation.boneAnimations[i].scaleKeys.emplace_back(
                        static_cast<float>(anim->mScalingKeys[j].mTime / aiAnim->mTicksPerSecond),
                        Vector3(anim->mScalingKeys[j].mValue.x,
                            anim->mScalingKeys[j].mValue.y,
                            anim->mScalingKeys[j].mValue.z));
                }
            }

            m_animations.push_back(std::move(animation));
        }
    }

    const std::vector<Animation>& AnimationData::GetAnimations() const
    {
        return m_animations;
    }
}