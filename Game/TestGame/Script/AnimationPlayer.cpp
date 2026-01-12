#include "GamePCH.h"
#include "AnimationPlayer.h"

#include "Framework/Object/Component/SkeletalAnimator.h"

namespace game
{
    void AnimationPlayer::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::Q))
        {
            auto animator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();
            animator->Play(0, true);
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

    std::string AnimationPlayer::GetType() const
    {
        return "AnimationPlayer";
    }
}