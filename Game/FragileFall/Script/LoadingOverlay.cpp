#include "GamePCH.h"
#include "LoadingOverlay.h"

namespace game
{
    void LoadingOverlay::OnGui()
    {
    }

    void LoadingOverlay::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void LoadingOverlay::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}