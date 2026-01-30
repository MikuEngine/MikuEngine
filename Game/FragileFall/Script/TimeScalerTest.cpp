#include "GamePCH.h"
#include "TimeScalerTest.h"

#include "Manager/TimeScaler.h"

namespace game
{
    void TimeScalerTest::Update()
    {
        if (engine::Input::IsKeyPressed(engine::Keys::D1))
        {
            TimeScaler::ApplyWorldTimeScale(0.5f, 1.0f);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D2))
        {
            TimeScaler::ApplyCrossfadeWorldTimeScale(0.1f, 2.5f, 1.2f, 1.2f);
        }
    }

    void TimeScalerTest::OnGui()
    {
    }

    void TimeScalerTest::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void TimeScalerTest::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}