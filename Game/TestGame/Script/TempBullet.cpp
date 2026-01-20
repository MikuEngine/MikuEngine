#include "GamePCH.h"
#include "TempBullet.h"

namespace game
{
    void TempBullet::OnGui()
    {
    }

    void TempBullet::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void TempBullet::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    std::string TempBullet::GetType() const
    {
        return "TempBullet";
    }
}