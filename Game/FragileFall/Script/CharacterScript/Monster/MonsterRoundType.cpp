#include "GamePCH.h"
#include "MonsterRoundType.h"

namespace game
{
    void MonsterRoundType::OnGui()
    {
    }

    void MonsterRoundType::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void MonsterRoundType::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}