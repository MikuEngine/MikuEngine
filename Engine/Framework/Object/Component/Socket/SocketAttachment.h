#pragma once

#include "Framework/Object/Component/Component.h"

namespace engine
{
    class Renderer;

    class SocketAttachment : public Component
    {
        REGISTER_COMPONENT(SocketAttachment, Component)

    private:
        Ptr<Renderer> m_targetRenderer = nullptr;
        std::string m_socketName;

    public:
        SocketAttachment() = default;
        virtual ~SocketAttachment();

        void SetSocket(Renderer* renderer, const std::string& socketName);
        void NotifyTargetRendererDestroyed();
    };
}
