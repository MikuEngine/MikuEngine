#include "GamePCH.h"
#include "TempMonster.h"

namespace game
{
    void TempMonster::OnGui()
    {
    }

    void TempMonster::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void TempMonster::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    std::string TempMonster::GetType() const
    {
        return "TempMonster";
    }
}