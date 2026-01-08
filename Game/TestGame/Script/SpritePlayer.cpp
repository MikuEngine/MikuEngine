#include "GamePCH.h"
#include "SpritePlayer.h"

#include "Framework/Object/Component/SpriteAnimator.h"

namespace game
{
    void SpritePlayer::Start()
    {
        auto spriteAnimator = GetGameObject()->GetComponent<engine::SpriteAnimator>();
        spriteAnimator->Play(0);
    }

    void SpritePlayer::OnGui()
    {
    }

    void SpritePlayer::Save(engine::json& j) const
    {
        j["Type"] = GetType();
    }

    void SpritePlayer::Load(const engine::json& j)
    {
    }

    std::string SpritePlayer::GetType() const
    {
        return "SpritePlayer";
    }
}