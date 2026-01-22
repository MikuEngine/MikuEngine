#include "GamePCH.h"
#include "ButtonCallbackTest2.h"

#include <Framework/Object/Component/UI/UIButton.h>

namespace game
{
    void ButtonCallbackTest2::Awake()
    {
        auto go = engine::GameObject::Find("test button");

        auto button = go->GetComponent<engine::UIButton>();

        {
            auto callback = [self = engine::Ptr<ButtonCallbackTest2>(this)]()
                {
                    if (self)
                    {
                        self->CallThat();
                    }
                };

            button->AddOnClick(std::move(callback));
        }
    }
    void ButtonCallbackTest2::OnGui()
    {
    }

    void ButtonCallbackTest2::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void ButtonCallbackTest2::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    void ButtonCallbackTest2::CallThat()
    {
        LOG_PRINT("ButtonCallbackTest2 call");
    }
}