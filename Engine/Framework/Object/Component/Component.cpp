#include "pch.h"
#include "Component.h"

#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"

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

    void Component::Destroy()
    {
        if (m_isPendingKill)
        {
            return;
        }

        m_isPendingKill = true;

        SceneManager::Get().GetScene()->RegisterPendingKill(this);
    }

    bool Component::IsPendingKill() const
    {
        return m_isPendingKill;
    }
}