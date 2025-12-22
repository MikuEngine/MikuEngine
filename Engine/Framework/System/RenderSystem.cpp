#include "pch.h"
#include "RenderSystem.h"

#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Data/ConstantBufferTypes.h"
#include "Core/Graphics/Resource/ConstantBuffer.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"
#include "Framework/Object/Component/Camera.h"

namespace engine
{
    RenderSystem::RenderSystem()
    {
        m_transformCB = ResourceManager::Get().GetOrCreateConstantBuffer("Transform", sizeof(TransformBuffer));
    }

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
        //for (auto renderer : m_shadowList)
        //{
        //    renderer->Draw();
        //}

        auto deviceContext = GraphicsDevice::Get().GetDeviceContext();
        const auto camera = SceneManager::Get().GetCurrentScene()->GetMainCamera();
        camera->Update();
        TransformBuffer transformBuffer;
        transformBuffer.view = camera->GetView().Transpose();
        transformBuffer.projection = camera->GetProjection().Transpose();

        deviceContext->VSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::Transform), 1, m_transformCB->GetBuffer().GetAddressOf());
        deviceContext->UpdateSubresource(m_transformCB->GetRawBuffer(), 0, nullptr, &transformBuffer, 0, 0);
        for (auto renderer : m_opaqueList)
        {
            renderer->Draw();
        }

        //for (auto renderer : m_transparentList)
        //{
        //    renderer->Draw();
        //}

        //for (auto renderer : m_screenList)
        //{
        //    renderer->Draw();
        //}
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
