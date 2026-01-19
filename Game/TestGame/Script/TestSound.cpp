#include "GamePCH.h"
#include "TestSound.h"

namespace game
{
    void SoundTest::Awake()
    {
        CreateGameObject("In SoundTest");
        GetGameObject()->Destroy();
    }

    void SoundTest::OnGui()
    {
    }

    void SoundTest::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void SoundTest::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    std::string SoundTest::GetType() const
    {
        return "SoundTest";
    }
}