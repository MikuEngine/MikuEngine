#include "pch.h"
#include "RenderSystem.h"

namespace engine
{
    void RenderSystem::Render()
    {
        for (auto& renderer : m_components)
        {
            renderer->Draw();
        }
    }
}
