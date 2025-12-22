#pragma once

#include "Framework/System/System.h"
#include "Framework/Object/Component/Renderer.h"

namespace engine
{
    class ConstantBuffer;

    class RenderSystem :
        public System<Renderer>
    {
    private:
        std::vector<Renderer*> m_opaqueList;
        std::vector<Renderer*> m_transparentList;
        std::vector<Renderer*> m_screenList;
        std::vector<Renderer*> m_shadowList;
        std::shared_ptr<ConstantBuffer> m_transformCB;

    public:
        RenderSystem();

    public:
        void Register(Renderer* renderer) override;
        void Unregister(Renderer* renderer) override;

    public:
        void Render();

    private:
        void AddRenderer(std::vector<Renderer*>& v, Renderer* renderer, RenderType type);
        void RemoveRenderer(std::vector<Renderer*>& v, Renderer* renderer, RenderType type);
    };
}
