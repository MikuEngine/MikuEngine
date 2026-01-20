#include "GamePCH.h"
#include "AimPointer.h"

namespace game
{
    void AimPointer::OnGui()
    {
    }

    void AimPointer::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void AimPointer::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    std::string AimPointer::GetType() const
    {
        return "ScriptTemplate";
    }
}