#include "pch.h"
#include "SkeletalAnimator.h"

#include "Framework/Asset/AssetManager.h"
#include "Framework/Asset/AnimationData.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/SkeletalMeshRenderer.h"

namespace engine
{
    void SkeletalAnimator::Awake()
    {
        if (m_animationData != nullptr)
        {
            return;
        }

        if (auto renderer = GetGameObject()->GetComponent<SkeletalMeshRenderer>())
        {
            SetAnimationData(renderer->GetMeshPath());
            m_skeletonData = renderer->GetSkeletonData(); // 캐싱
            m_skeletonData->SetupSkeletonInstance(m_skeleton);
        }
    }

    void SkeletalAnimator::SetAnimationData(const std::string& path)
    {
        m_animationPath = path;

        m_animationData = AssetManager::Get().GetOrCreateAnimationData(path);
    }

    void SkeletalAnimator::Play(int index, bool loop)
    {
        if (m_animationData == nullptr)
        {
            LOG_INFO("AnimationData 없음");
            return;
        }

        const auto& animations = m_animationData->GetAnimations();
        if (index < 0 || animations.size() <= index)
        {
            return;
        }

        m_currentAnimIndex = index;
        m_animationProgressTime = 0.0f;
        m_isLoop = loop;
        m_isPlaying = true;

        m_nextAnimIndex = -1;
        animations[index].SetupBoneAnimation(m_skeleton);
    }

    void SkeletalAnimator::Play(const std::string& animationName, bool loop)
    {
        if (m_animationData == nullptr)
        {
            LOG_INFO("AnimationData 없음");
            return;
        }

        const int index = GetAnimationIndex(animationName);
        if (index == -1)
        {
            return;
        }

        Play(index, loop);
    }

    void SkeletalAnimator::PlayCrossFade(int index, float transitionDuration, bool loop)
    {
        if (!m_animationData)
        {
            LOG_INFO("AnimationData 없음");
            return;
        }

        const auto& animations = m_animationData->GetAnimations();
        if (index < 0 || animations.size() >= index || (m_currentAnimIndex == index && m_nextAnimIndex == -1))
        {
            return;
        }

        m_nextAnimIndex = index;
        m_transitionDuration = transitionDuration;
        m_transitionProgressTime = 0.0f;
        m_isLoop = loop;
        m_isPlaying = true;

        animations[index].SetupBoneAnimation(m_skeleton);
    }

    void SkeletalAnimator::PlayCrossFade(const std::string& animationName, float transitionDuration, bool loop)
    {
        if (!m_animationData)
        {
            LOG_INFO("AnimationData 없음");
            return;
        }

        const int index = GetAnimationIndex(animationName);
        if (index == -1 || (m_currentAnimIndex == index && m_nextAnimIndex == -1))
        {
            return;
        }

        PlayCrossFade(index, transitionDuration, loop);
    }

    void SkeletalAnimator::Update()
    {
        if (!m_isPlaying || !m_animationData || m_currentAnimIndex == -1)
        {
            return;
        }

        const float dt = m_playSpeed * Time::DeltaTime();

        const auto& animations = m_animationData->GetAnimations();
        const auto& currentAnim = animations[m_currentAnimIndex];
        const float duration = currentAnim.duration;

        if (m_nextAnimIndex != -1)
        {
            m_transitionProgressTime += dt;
            if (m_transitionProgressTime >= m_transitionDuration)
            {
                m_currentAnimIndex = m_nextAnimIndex;
                m_nextAnimIndex = -1;
                m_animationProgressTime = 0.0f;
            }
        }

        m_animationProgressTime += dt;

        if (m_animationProgressTime >= duration)
        {
            if (m_isLoop)
            {
                m_animationProgressTime = std::fmod(m_animationProgressTime, duration);
            }
            else
            {
                m_animationProgressTime = duration;
            }
        }

        if (m_skeleton.empty() || !m_skeletonData)
        {
            return;
        }

        const auto& boneOffsets = m_skeletonData->GetBoneOffsets();
        for (auto& bone : m_skeleton)
        {
            Matrix nodeTransform = bone.local;
            if (m_animationData && m_currentAnimIndex != -1)
            {
                Vector3 curPos;
                Quaternion curRot;
                Vector3 curScale;

                bool hasCurrent = false;
                if (bone.boneAnimation) // Play 시점에 캐싱된 채널
                {
                    bone.boneAnimation->Evaluate(m_animationProgressTime, bone.lastKeyIndex, curPos, curRot, curScale);
                    hasCurrent = true;
                }

                // Blending (Next Animation이 존재하고 과도기일 때)
                if (m_nextAnimIndex != -1 && hasCurrent)
                {
                    const auto& nextAnim = m_animationData->GetAnimations()[m_nextAnimIndex];

                    // Next Animation 채널 찾기 (Map Lookup)
                    // 최적화를 위해 Bone 구조체에 nextBoneAnimation 포인터를 추가하거나
                    // 별도 캐시를 쓸 수 있지만 여기선 기본 조회 사용
                    
                    if (auto iter = nextAnim.animMappingTable.find(bone.name); iter != nextAnim.animMappingTable.end())
                    {
                        const auto& nextChannel = nextAnim.boneAnimations[iter->second];

                        Vector3 nextPos;
                        Quaternion nextRot;
                        Vector3 nextScale;
                        LastKeyIndex tempKeyIndex{}; // Next는 상태 저장 없이 매번 검색 (혹은 별도 관리 필요)

                        nextChannel.Evaluate(m_transitionProgressTime, tempKeyIndex, nextPos, nextRot, nextScale);
                        // 블렌딩 비율 (0.0 ~ 1.0)
                        float t = std::clamp(m_transitionProgressTime / m_transitionDuration, 0.0f, 1.0f);

                        curPos = Vector3::Lerp(curPos, nextPos, t);
                        curRot = Quaternion::Slerp(curRot, nextRot, t);
                        curScale = Vector3::Lerp(curScale, nextScale, t);
                    }
                }

                if (hasCurrent)
                {
                    nodeTransform = Matrix::CreateScale(curScale)
                        * Matrix::CreateFromQuaternion(curRot)
                        * Matrix::CreateTranslation(curPos);
                }
            }
            
            if (bone.parentIndex != -1)
            {
                bone.model = nodeTransform * m_skeleton[bone.parentIndex].model;
            }
            else
            {
                bone.model = nodeTransform;
            }

            m_finalBoneMatrices[bone.index] = (boneOffsets[bone.index] * bone.model).Transpose();
        }
    }

    const BoneMatrixArray& SkeletalAnimator::GetFinalBoneMatrices() const
    {
        return m_finalBoneMatrices;
    }

    void SkeletalAnimator::OnGui()
    {
    }

    void SkeletalAnimator::Save(json& j) const
    {
        j["Type"] = GetType();
    }

    void SkeletalAnimator::Load(const json& j)
    {
    }

    std::string SkeletalAnimator::GetType() const
    {
        return "SkeletalAnimator";
    }

    int SkeletalAnimator::GetAnimationIndex(const std::string& name)
    {
        if (!m_animationData)
        {
            return -1;
        }

        const auto& animations = m_animationData->GetAnimations();

        for (size_t i = 0; i < animations.size(); ++i)
        {
            if (animations[i].name == name)
            {
                return static_cast<int>(i);
            }
        }

        return -1;
    }
}