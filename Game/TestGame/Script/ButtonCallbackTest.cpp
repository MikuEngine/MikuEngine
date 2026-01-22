#include "GamePCH.h"
#include "ButtonCallbackTest.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/Component/UI/UIButton.h>

namespace game
{
    void ButtonCallbackTest::Awake()
    {
        auto go = engine::SceneManager::Get().GetScene()->FindGameObject("test button");

        auto button = go->GetComponent<engine::UIButton>();

        {
            auto callback = [self = engine::Ptr<ButtonCallbackTest>(this)]()
                {
                    if (self)
                    {
                        self->CallThis();
                    }
                };

            button->AddOnClick(std::move(callback));
        }


    }
    void ButtonCallbackTest::OnGui()
    {
    }

    void ButtonCallbackTest::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void ButtonCallbackTest::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    void ButtonCallbackTest::CallThis()
    {
        LOG_PRINT("CallThis");
    }
}