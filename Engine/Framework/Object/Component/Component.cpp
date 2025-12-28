#include "pch.h"
#include "Component.h"

#include "Framework/Object/GameObject/GameObject.h"

namespace engine
{
    GameObject* Component::GetGameObject() const
    {
        return m_owner;
    }

    Transform* Component::GetTransform() const
    {
        return m_owner->GetTransform();
    }
}