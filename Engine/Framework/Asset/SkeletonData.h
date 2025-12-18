#pragma once

#include "Framework/Asset/AssetData.h"

struct aiScene;

namespace engine
{
    constexpr size_t MAX_BONE_NUM = 128;
    using BoneMatrixArray = std::array<DirectX::SimpleMath::Matrix, MAX_BONE_NUM>;

    struct BoneInfo
    {
        std::string name;
        Matrix relative;
        unsigned int index;
        int parentIndex;
    };

    struct LastKeyIndex
    {
        size_t position;
        size_t rotation;
        size_t scale;
    };

    struct BoneAnimation;

    struct Bone
    {
        std::string name;
        Matrix local;
        Matrix model;
        int parentIndex;
        unsigned int index;
        const BoneAnimation* boneAnimation = nullptr;
        LastKeyIndex lastKeyIndex{};

        Bone(const std::string& name, int parentIndex, unsigned int index, const Matrix& local)
            : name{ name }, parentIndex{ parentIndex }, index{ index }, local{ local }
        {

        }
    };

    class SkeletonData :
        public AssetData
    {
    private:
        using BoneIndex = unsigned int;
        using BoneName = std::string;
        using MeshName = std::string;

        std::vector<BoneInfo> m_bones;
        std::unordered_map<BoneName, BoneIndex> m_boneMappingTable;
        std::unordered_map<MeshName, BoneIndex> m_meshMappingTable;
        BoneMatrixArray m_boneOffsets;

    public:
        void Create(const std::string& filePath);
        void Create(const aiScene* scene);

    public:
        const std::vector<BoneInfo>& GetBones() const;
        unsigned int GetBoneIndexByBoneName(const std::string& boneName) const;
        unsigned int GetBoneIndexByMeshName(const std::string& meshName) const;
        BoneInfo* GetBoneInfoByIndex(size_t index);
        const BoneMatrixArray& GetBoneOffsets() const;

        void SetBoneOffset(const Matrix& offset, unsigned int boneIndex);

        void SetupSkeletonInstance(std::vector<Bone>& out) const;
    };
}