#include "GamePCH.h"
#include "TempPlayer.h"

namespace game
{
    void TempPlayer::OnGui()
    {
    }

    void TempPlayer::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void TempPlayer::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    std::string TempPlayer::GetType() const
    {
        return "tempPlayer";
    }
}