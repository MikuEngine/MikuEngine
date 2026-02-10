#include "GamePCH.h"
#include "PlayerPreviewScript.h"

#include <Engine/Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Engine/Framework/System/SoundSystem.h>

namespace game
{
    void PlayerPreviewScript::Awake()
    {
        auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
        if (!animator) return;

        animator->BindNotify("Event_LeftFootStep", [this]()
            {
                engine::SoundSystem::Get().Play("Player_FootStep_Left_Random", "SFX/Player", true);
            });

        animator->BindNotify("Event_RightFootStep", [this]()
            {
                engine::SoundSystem::Get().Play("Player_FootStep_Right_Random", "SFX/Player", true);
            });

    }

    void PlayerPreviewScript::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void PlayerPreviewScript::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}