#include "GamePCH.h"
#include "DontDestroyTest.h"

namespace game
{
    static engine::Ptr<engine::GameObject> s_instance = nullptr;

    void DontDestroyTest::Start()
    {
        if (s_instance == nullptr)
        {
            s_instance = GetGameObject();

            GetGameObject()->DontDestroyOnLoad();
        }
        else
        {
            GetGameObject()->Destroy();
        }
    }

    void DontDestroyTest::OnGui()
    {
    }

    void DontDestroyTest::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void DontDestroyTest::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    std::string DontDestroyTest::GetType() const
    {
        return "DontDestroyTest";
    }
}