#pragma once

#include "Framework/Object/Component/Animator.h"
#include "Framework/Asset/SkeletonData.h"

namespace engine
{
    class AnimationData;

    class SkeletalAnimator :
        public Animator
    {
        REGISTER_COMPONENT(SkeletalAnimator)

    private:
        std::shared_ptr<AnimationData> m_animationData;
        std::shared_ptr<SkeletonData> m_skeletonData;
        std::string m_animationPath;

        int m_currentAnimIndex = -1;
        int m_nextAnimIndex = -1;

        float m_animationProgressTime = 0.0f;
        float m_transitionDuration = 0.0f;
        float m_transitionProgressTime = 0.0f;
        float m_playSpeed = 1.0f;

        bool m_isPlaying = false;
        bool m_isLoop = true;

        BoneMatrixArray m_finalBoneMatrices;
        std::vector<Bone> m_skeleton;

    public:
        void Awake() override;

        void SetAnimationData(const std::string& path);

        void Play(int index, bool loop = true);
        void Play(const std::string& animationName, bool loop = true);
        void PlayCrossFade(int index, float transitionDuration, bool loop = true);
        void PlayCrossFade(const std::string& animationName, float transitionDuration, bool loop = true);
        void Update();

        const BoneMatrixArray& GetFinalBoneMatrices() const;

        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
        std::string GetType() const override;

    private:
        int GetAnimationIndex(const std::string& name);
    };
}