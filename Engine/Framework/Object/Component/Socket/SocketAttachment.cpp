#include "EnginePCH.h"
#include "SocketAttachment.h"
#include "Framework/Object/Component/Renderer/Renderer.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Transform.h"

namespace engine
{
    SocketAttachment::~SocketAttachment()
    {
        if (m_targetRenderer)
        {
            m_targetRenderer->UnregisterAttachedObject(GetGameObject());
        }
    }

    void SocketAttachment::SetSocket(Renderer* renderer, const std::string& socketName)
    {
        if (m_targetRenderer)
        {
            m_targetRenderer->UnregisterAttachedObject(GetGameObject());
        }

        m_targetRenderer = renderer;
        m_socketName = socketName;

        if (m_targetRenderer)
        {
            m_targetRenderer->RegisterAttachedObject(GetGameObject(), m_socketName);
        }
    }

    void SocketAttachment::NotifyTargetRendererDestroyed()
    {
        m_targetRenderer = nullptr;
    }
}