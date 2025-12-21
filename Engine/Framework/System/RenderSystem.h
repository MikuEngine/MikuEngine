#pragma once

#include "Framework/System/System.h"
#include "Framework/Object/Component/Renderer.h"

namespace engine
{
    class RenderSystem :
        public System<Renderer>
    {
    private:
        std::vector<Renderer*> m_opaqueList;
        std::vector<Renderer*> m_transparentList;
        std::vector<Renderer*> m_screenList;
        std::vector<Renderer*> m_shadowList;

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
