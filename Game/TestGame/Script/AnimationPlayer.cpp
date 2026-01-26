#include "GamePCH.h"
#include "AnimationPlayer.h"

#include "Framework/Object/Component/Animator/SkeletalAnimator.h"
#include "Framework/Scene/SceneManager.h"

namespace game
{
    void AnimationPlayer::Awake()
    {
		/*/ notify 테스트 코드
        auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
        if (!animator) return;

        animator->AddNotify("TestPunch", "PunchSound", 0.2f);
        animator->AddNotify("TestPunch", "CreateHitBox", 0.2f);

        animator->BindNotify("PunchSound", [this]()
            {
                // 펀치 소리 재생 로직만 작성
                LOG_INFO("AnimationPlayer::Awake() Notify  퍽!");
            });

        animator->BindNotify("CreateHitBox", [this]()
            {
                // 히트박스 생성 로직만 작성
                LOG_INFO("AnimationPlayer::Awake() Notify  공격 판정 ON");
            });
		//*/
    }

    void AnimationPlayer::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::D1))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            animator->PlayCrossFade("1", 0.5f, true, 0, 1.0f);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D2))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            animator->PlayCrossFade("2", 0.5f, true, 0, 1.0f);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D3))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            animator->PlayCrossFade("3", 0.5f, true, 0, 1.0f);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D4))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            animator->PlayCrossFade("4", 0.5f, false, 1, 1.0f);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D5))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            animator->PlayCrossFade("5", 0.5f, false, 1, 1.0f);
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