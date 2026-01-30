#include "EnginePCH.h"
#include "LoadingScreenRenderer.h"

namespace engine
{
    void LoadingScreenRenderer::SetRenderCallback(std::function<void(float)> fn)
    {
        m_renderCallback = std::move(fn);
    }

    void LoadingScreenRenderer::Render(float progress)
    {
        if (m_renderCallback)
        {
            m_renderCallback(progress);
        }
    }
}
