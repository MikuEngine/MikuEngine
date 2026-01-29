#pragma once

#include "Framework/Object/Component/Script.h"

namespace engine
{
    class Renderer;

    class SocketAttachment :
        public Script<SocketAttachment>
    {
        REGISTER_SCRIPT(SocketAttachment, Script<SocketAttachment>)

    private:
        Renderer* m_targetRenderer = nullptr;
        std::string m_socketName;

    public:
        SocketAttachment() = default;
        virtual ~SocketAttachment();

        void SetSocket(Renderer* renderer, const std::string& socketName);
        void NotifyTargetRendererDestroyed() { m_targetRenderer = nullptr; }

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
    };
}
