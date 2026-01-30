#include "GamePCH.h"
#include "UIKillPopupAnimator.h"

#include <Framework/Object/GameObject/GameObject.h>

namespace game
{
    static engine::Vector2 Lerp(const engine::Vector2& a, const engine::Vector2& b, float t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
    }

    void UIKillPopupAnimator::Awake()
    {
        //auto* go = CreateGameObject(engine::CreateObjectType::, "KillPopup");
        //go->GetComponent
    }

    void UIKillPopupAnimator::Start()
    {

    }

    void UIKillPopupAnimator::Update()
    {

    }

    void UIKillPopupAnimator::OnGui()
    {

    }

    void UIKillPopupAnimator::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void UIKillPopupAnimator::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}