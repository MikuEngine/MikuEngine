#include "EnginePCH.h"
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

    bool Component::IsActive() const
    {
        if (!m_active)
        {
            return false;
        }

        return m_owner->IsActive();
    }

    void Component::SetActive(bool active)
    {
        if (m_active == active)
        {
            return;
        }

        m_active = active;

        if (!m_hasAwoken)
        {
            return;
        }

        if (m_owner->IsActive())
        {
            if (m_active)
            {
                OnEnable();
            }
            else
            {
                OnDisable();
            }
        }
    }

    void Component::MarkAsAwoken()
    {
        m_hasAwoken = true;
    }

    bool Component::HasAwoken() const
    {
        return m_hasAwoken;
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