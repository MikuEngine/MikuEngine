#include "pch.h"
#include "RenderSystem.h"

namespace engine
{
    void RenderSystem::Register(Renderer* renderer)
    {
        //if (renderer->HasRenderType(RenderType::Opaque))
        //{
            AddRenderer(m_opaqueList, renderer, RenderType::Opaque);
        //}

        //if (renderer->HasRenderType(RenderType::Transparent))
        //{
        //    AddRenderer(m_transparentList, renderer, RenderType::Transparent);
        //}

        //if (renderer->HasRenderType(RenderType::Screen))
        //{
        //    AddRenderer(m_screenList, renderer, RenderType::Screen);
        //}

        //if (renderer->HasRenderType(RenderType::Shadow))
        //{
        //    AddRenderer(m_shadowList, renderer, RenderType::Shadow);
        //}
    }

    void RenderSystem::Unregister(Renderer* renderer)
    {
        RemoveRenderer(m_opaqueList, renderer, RenderType::Opaque);
        RemoveRenderer(m_transparentList, renderer, RenderType::Transparent);
        RemoveRenderer(m_screenList, renderer, RenderType::Screen);
        RemoveRenderer(m_shadowList, renderer, RenderType::Shadow);
    }

    void RenderSystem::Render()
    {
        for (auto renderer : m_shadowList)
        {
            renderer->Draw();
        }

        for (auto renderer : m_opaqueList)
        {
            renderer->Draw();
        }

        for (auto renderer : m_transparentList)
        {
            renderer->Draw();
        }

        for (auto renderer : m_screenList)
        {
            renderer->Draw();
        }
    }

    void RenderSystem::AddRenderer(std::vector<Renderer*>& v, Renderer* renderer, RenderType type)
    {
        if (renderer->m_systemIndices[static_cast<size_t>(type)] != -1)
        {
            return; // 이미 등록됨
        }

        renderer->m_systemIndices[static_cast<size_t>(type)] = static_cast<std::int32_t>(v.size());
        v.push_back(renderer);
    }

    void RenderSystem::RemoveRenderer(std::vector<Renderer*>& v, Renderer* renderer, RenderType type)
    {
        if (v.empty())
        {
            return;
        }

        std::int32_t index = renderer->m_systemIndices[static_cast<size_t>(type)];
        if (index < 0)
        {
            return; // 등록 안됨
        }

        Renderer* back = v.back();
        v[index] = back;
        v.pop_back();

        back->m_systemIndices[static_cast<size_t>(type)] = index;
        renderer->m_systemIndices[static_cast<size_t>(type)] = -1;
    }
}
