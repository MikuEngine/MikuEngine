#include "GamePCH.h"
#include "TempBulletFactory.h"

namespace game
{
    void TempBulletFactory::OnGui()
    {
    }

    void TempBulletFactory::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void TempBulletFactory::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    std::string TempBulletFactory::GetType() const
    {
        return "TempBulletFactory";
    }
}