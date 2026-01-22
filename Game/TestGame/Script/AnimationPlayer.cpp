#include "GamePCH.h"
#include "AnimationPlayer.h"

#include "Framework/Object/Component/Animator/SkeletalAnimator.h"
#include "Framework/Scene/SceneManager.h"

namespace game
{
    void AnimationPlayer::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::D1))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            animator->PlayCrossFade("TestIdle", 0.5f, true, 0, 1.0f);
            animator->PlayCrossFade("TestIdle", 0.5f, true, 1, 1.0f);
            animator->SetLayerWeight(1, 0.0f);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D2))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            animator->PlayCrossFade("TestForward", 0.5f, true, 0, 1.0f);
            animator->PlayCrossFade("TestForward", 0.5f, true, 1, 1.0f);
            animator->SetLayerWeight(1, 0.0f);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D3))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            animator->PlayCrossFade("TestBackward", 0.5f, true, 0, 1.0f);
            animator->PlayCrossFade("TestBackward", 0.5f, true, 1, 1.0f);
            animator->SetLayerWeight(1, 0.0f);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D4))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            animator->PlayCrossFade("TestPunch", 0.5f, false, 1, 1.0f);
            animator->SetLayerWeight(1, 1.0f);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D5))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            animator->PlayCrossFade("TestElbow", 0.5f, false, 1, 1.0f);
            animator->SetLayerWeight(1, 1.0f);
        }

        auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
        auto name = animator->GetCurrentAnimationName(1);
        if (name == "TestPunch" || name == "TestElbow")
        {
            if (animator->GetNormalizedTime(1) > 0.8f)
            {
                animator->PlayCrossFade("TestIdle", 0.5f, true, 1, 1.0f);
            }
        }

        constexpr float rotationSpeed = engine::ToRadian(90.0f); // 초당 90도
        float dt = engine::Time::DeltaTime();

        // 1. 입력에 따라 각도(Scalar) 값만 변경
        if (engine::Input::IsKeyHeld(engine::Keys::Up))
        {
            m_currentPitch += rotationSpeed * dt;
        }

        if (engine::Input::IsKeyHeld(engine::Keys::Down))
        {
            m_currentPitch -= rotationSpeed * dt;
        }

        if (engine::Input::IsKeyHeld(engine::Keys::Left))
        {
            m_currentYaw -= rotationSpeed * dt; // 좌표계에 따라 -, + 방향 확인 필요
        }

        if (engine::Input::IsKeyHeld(engine::Keys::Right))
        {
            m_currentYaw += rotationSpeed * dt;
        }

        m_currentPitch = std::clamp(m_currentPitch, engine::ToRadian(-80.0f), engine::ToRadian(80.0f));
        m_currentYaw = std::clamp(m_currentYaw, engine::ToRadian(-70.0f), engine::ToRadian(70.0f));

        engine::Quaternion pitchRot = engine::Quaternion::CreateFromAxisAngle(engine::Vector3::UnitX, m_currentPitch);
        engine::Quaternion yawRot = engine::Quaternion::CreateFromAxisAngle(engine::Vector3::UnitY, m_currentYaw);

        // 일반적인 순서: Global Y(Yaw) -> Local X(Pitch)
        m_upperBodyRotation = yawRot * pitchRot;

        // 4. 애니메이터에 적용
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            if (animator)
            {
                animator->SetProceduralRotation("mixamorig:Spine", m_upperBodyRotation);
            }
        }

        if (engine::Input::IsKeyPressed(engine::Keys::B))
        {
            engine::SceneManager::Get().ChangeScene("StaticMeshTest");
        }
    }

    void AnimationPlayer::OnGui()
    {
    }

    void AnimationPlayer::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void AnimationPlayer::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}