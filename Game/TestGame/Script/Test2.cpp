#include "GamePCH.h"
#include "Test2.h"

#include "TestScript.h"

namespace game
{
    void Test2::Awake()
    {
        CreateGameObject("In test2");
        GetGameObject()->Destroy();
    }

    void Test2::OnGui()
    {
    }

    void Test2::Save(engine::json& j) const
    {
        j["Type"] = GetType();
    }

    void Test2::Load(const engine::json& j)
    {
    }

    std::string Test2::GetType() const
    {
        return "Test2";
    }
}