#pragma once

#include "Framework/Object/Component/Script.h"

namespace engine
{
    class Renderer;

    class SocketAttachment :
        public Script<SocketAttachment>
    {
        REGISTER_SCRIPT(SocketAttachment, Script)

    private:
        Ptr<Renderer> m_targetRenderer = nullptr;
        std::string m_socketName;

    public:
        SocketAttachment() = default;
        virtual ~SocketAttachment();

        void SetSocket(Renderer* renderer, const std::string& socketName);
        void NotifyTargetRendererDestroyed();

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
    };
}
