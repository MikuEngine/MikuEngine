#pragma once

#include "Framework/Object/Component/Animator/Animator.h"
#include "Framework/Asset/SkeletonData.h"

namespace engine
{
    class AnimationData;

    enum class AnimationBlendMode
    {
        Override,
        Additive
    };

    struct AnimationNotify
    {
        std::string name;
        float time;
    };

    struct AnimationResource
    {
        std::shared_ptr<AnimationData> data;
        std::string path;
        std::vector<AnimationNotify> notifies;
    };

    struct AnimationState
    {
        std::string name;
        std::shared_ptr<AnimationData> data;
        int animIndex = 0;
        float time = 0.0f;
        float speed = 0.0f;
        bool loop = true;
        bool active = false;

        void Reset()
        {
            name = "";
            data = nullptr;
            animIndex = 0;
            time = 0.0f;
            speed = 1.0f;
            loop = true;
            active = false;
        }
    };

    struct AnimationLayer
    {
        std::string name;
        float weight = 1.0f;
        AnimationBlendMode blendMode = AnimationBlendMode::Override;
        std::vector<std::uint8_t> mask; // mask[boneIndex] == 1 이면 해당 본은 이 레이어의 영향을 받음
        std::vector<std::string> pendingMaskBones;
        AnimationState current;
        AnimationState next;
        float transitionDuration = 0.0f;
        float transitionTime = 0.0f;
        
        AnimationLayer(const std::string& name)
            : name{ name }
        {

        }
    };

    class SkeletalAnimator :
        public Animator
    {
        REGISTER_COMPONENT(SkeletalAnimator, Animator)

    private:
        std::string m_selectedAnimName = "";
        std::unordered_map<std::string, AnimationResource> m_animations;
        std::vector<AnimationLayer> m_layers;
        std::shared_ptr<SkeletonData> m_skeletonData;
        std::vector<Bone> m_skeleton;
        BoneMatrixArray m_finalBoneMatrices;
        std::unordered_map<int, Quaternion> m_proceduralRotations;
        std::unordered_map<std::string, EventCallBack> m_notifyCallbacks;

    public:
        void Initialize() override;
        void Awake() override;

        void RegisterAnimation(const std::string& name, const std::string& path);
        void UnregisterAnimation(const std::string& name);
        void RenameAnimation(const std::string& oldName, const std::string& newName);

        void AddLayer(const std::string& layerName, float weight = 1.0f);
        void RemoveLayer(int layerIndex);

        void AddNotify(const std::string& animName, const std::string& notifyName, float time = 0.0f);
        void BindNotify(const std::string& notifyName, EventCallBack callback);
        void UnbindNotify(const std::string& notifyName);

        void SetLayerMask(int layerIndex, const std::vector<std::string>& boneNames, bool active, bool isRecursive = true);
        void SetLayerWeight(int layerIndex, float weight);
        void SetLayerBlendMode(int layerIndex, AnimationBlendMode mode);
        void SetProceduralRotation(const std::string& boneName, const Quaternion& rotation);

        bool IsFinished(int layerIndex = 0) const;
        bool IsPlaying(int layerIndex = 0) const;
        bool IsInTransition(int layerIndex = 0) const;
        std::string GetCurrentAnimationName(int layerIndex = 0) const;
        float GetCurrentAnimationTime(int layerIndex = 0) const;
        float GetCurrentAnimationDuration(int layerIndex = 0) const;
        float GetNormalizedTime(int layerIndex = 0) const;

        void Play(const std::string& name, bool loop = true, int layerIndex = 0, float speed = 1.0f);
        void PlayCrossFade(
            const std::string& name,
            float transitionDuration,
            bool loop = true,
            int layerIndex = 0,
            float speed = 1.0f);
        void Update() override;
        void DrawTimeline(const std::string& animName);
        void UpdateTimeinePose();

        const BoneMatrixArray& GetFinalBoneMatrices() const;

        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;

    private:
        void InitializeSkeleton();
        void MarkBoneRecursive(std::vector<std::uint8_t>& mask, int parentIndex, bool active);
        bool EvaluateBone(
            const AnimationState& state,
            const std::string& boneName,
            float time,
            Vector3& outPos,
            Quaternion& outRot,
            Vector3& outScale);
        void RenderBoneTree(int boneIndex, std::vector<std::uint8_t>& mask);
        void SetLayerMaskInternal(AnimationLayer& layer, const std::vector<std::string>& boneNames);
        void CheckNotifies(const AnimationState& state, float prevTime, float currTime);
    };
}