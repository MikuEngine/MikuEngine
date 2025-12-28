#include "pch.h"
#include "RenderSystem.h"

#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Data/ConstantBufferTypes.h"
#include "Core/Graphics/Data/ShaderSlotTypes.h"
#include "Core/Graphics/Resource/ConstantBuffer.h"
#include "Core/Graphics/Resource/SamplerState.h"
#include "Core/Graphics/Resource/Texture.h"
#include "Core/Graphics/Resource/VertexShader.h"
#include "Core/Graphics/Resource/PixelShader.h"
#include "Core/Graphics/Resource/DepthStencilState.h"
#include "Core/Graphics/Resource/RasterizerState.h"
#include "Core/Graphics/Resource/IndexBuffer.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"
#include "Framework/Object/Component/Camera.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/CameraSystem.h"
#include "Editor/EditorManager.h"
#include "Editor/EditorCamera.h"

namespace engine
{
    namespace
    {
        TimePoint g_startTime = Clock::now();
    }

    RenderSystem::RenderSystem()
    {
        m_globalConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("Transform", sizeof(CbGlobal));

        m_comparisonSamplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Comparison);
        m_clampSamplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Clamp);

        m_skyboxEnv = ResourceManager::Get().GetOrCreateTexture("Resource/Texture/MirroredHallEnvHDR.dds");
        m_irradianceMap = ResourceManager::Get().GetOrCreateTexture("Resource/Texture/MirroredHallDiffuseHDR.dds");
        m_specularMap = ResourceManager::Get().GetOrCreateTexture("Resource/Texture/MirroredHallSpecularHDR.dds");
        m_brdfLut = ResourceManager::Get().GetOrCreateTexture("Resource/Texture/MirroredHallBrdf.dds");

        // cube
        {
            std::vector<PositionVertex> vertices{
                // position
                { { -0.5f,  0.5f, -0.5f } }, // 0 - 0
                { {  0.5f,  0.5f, -0.5f } }, // 1 - 1
                { { -0.5f, -0.5f, -0.5f } }, // 2 - 2
                { {  0.5f, -0.5f, -0.5f } }, // 3 - 3

                { {  0.5f,  0.5f,  0.5f } }, // 4 - 4
                { { -0.5f,  0.5f,  0.5f } }, // 5 - 5
                { {  0.5f, -0.5f,  0.5f } }, // 6 - 6
                { { -0.5f, -0.5f,  0.5f } }, // 7 - 7

                { { -0.5f,  0.5f,  0.5f } }, // 5 - 8
                { { -0.5f,  0.5f, -0.5f } }, // 0 - 9
                { { -0.5f, -0.5f,  0.5f } }, // 7 - 10
                { { -0.5f, -0.5f, -0.5f } }, // 2 - 11

                { {  0.5f,  0.5f, -0.5f } }, // 1 - 12
                { {  0.5f,  0.5f,  0.5f } }, // 4 - 13
                { {  0.5f, -0.5f, -0.5f } }, // 3 - 14
                { {  0.5f, -0.5f,  0.5f } }, // 6 - 15

                { { -0.5f, -0.5f, -0.5f } }, // 2 - 16
                { {  0.5f, -0.5f, -0.5f } }, // 3 - 17
                { { -0.5f, -0.5f,  0.5f } }, // 7 - 18
                { {  0.5f, -0.5f,  0.5f } }, // 6 - 19

                { { -0.5f,  0.5f,  0.5f } }, // 5 - 20
                { {  0.5f,  0.5f,  0.5f } }, // 4 - 21
                { { -0.5f,  0.5f, -0.5f } }, // 0 - 22
                { {  0.5f,  0.5f, -0.5f } }, // 1 - 23
            };

            std::vector<DWORD> indices{
                0, 1, 2,
                2, 1, 3,

                4, 5, 6,
                6, 5, 7,

                8, 9, 10,
                10, 9, 11,

                12, 13, 14,
                14, 13, 15,

                16, 17, 18,
                18, 17, 19,

                20, 21, 22,
                22, 21, 23,
            };

            m_indexCount = static_cast<UINT>(indices.size());
            m_cubeVertexBuffer = ResourceManager::Get().GetOrCreateVertexBuffer<PositionVertex>("Cube", vertices);
            m_cubeIndexBuffer = ResourceManager::Get().GetOrCreateIndexBuffer("Cube", indices);
            m_skyboxVertexShader = ResourceManager::Get().GetOrCreateVertexShader("Shader/Vertex/Skybox_VS.hlsl");
            m_skyboxPixelShader = ResourceManager::Get().GetOrCreatePixelShader("Shader/Pixel/Skybox_PS.hlsl");
            m_cubeInputLayout = m_skyboxVertexShader->GetOrCreateInputLayout<PositionVertex>();

            m_skyboxRSState = ResourceManager::Get().GetDefaultRasterizerState(DefaultRasterizerType::SolidFront);

            m_skyboxDSState = ResourceManager::Get().GetDefaultDepthStencilState(DefaultDepthStencilType::LessEqual);
            m_linearSamplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);
        }
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
        auto& graphics = GraphicsDevice::Get();
        auto context = GraphicsDevice::Get().GetDeviceContext();

        Matrix view, projection;
        Vector3 cameraPosition;

        switch (EditorManager::Get().GetEditorState())
        {
        case EditorState::Edit:
        case EditorState::Pause:
        {
            auto* cam = EditorManager::Get().GetEditorCamera();
            view = cam->GetView();
            projection = cam->GetProjection();
            cameraPosition = cam->GetPosition();
        }
            break;

        case EditorState::Play:
        {
            auto* cam = SystemManager::Get().GetCameraSystem().GetMainCamera();
            if (cam == nullptr)
            {
                return;
            }

            view = cam->GetView();
            projection = cam->GetProjection();
            cameraPosition = GetTranslation(cam->GetWorld());
        }
            break;
        }


        CbGlobal cbGlobal;
        cbGlobal.view = view.Transpose();
        cbGlobal.projection = projection.Transpose();
        cbGlobal.viewProjection = (view * projection).Transpose();
        cbGlobal.cameraWorldPoistion = cameraPosition;
        cbGlobal.elapsedTime = Time::GetElapsedSeconds(g_startTime);
        cbGlobal.mainLightViewProjection = cbGlobal.viewProjection;
        cbGlobal.mainLightWorldDirection = Vector3(0.0f, 0.0f, 1.0f);
        cbGlobal.mainLightColor = Vector3(1.0f, 1.0f, 1.0f);
        cbGlobal.mainLightIntensity = 10.0f;
        cbGlobal.maxHDRNits = graphics.GetMaxHDRNits();
        cbGlobal.exposure = -2.5f;
        cbGlobal.shadowMapSize = graphics.GetShadowMapSize();
        cbGlobal.useShadowPCF = 0;
        cbGlobal.pcfSize = 2;
        cbGlobal.useIBL = 1;

        context->VSSetConstantBuffers(
            static_cast<UINT>(ConstantBufferSlot::Global),
            1,
            m_globalConstantBuffer->GetBuffer().GetAddressOf());
        context->PSSetConstantBuffers(
            static_cast<UINT>(ConstantBufferSlot::Global),
            1,
            m_globalConstantBuffer->GetBuffer().GetAddressOf());

        context->UpdateSubresource(m_globalConstantBuffer->GetRawBuffer(), 0, nullptr, &cbGlobal, 0, 0);

        graphics.BeginDrawShadowPass();
        {

        }
        graphics.EndDrawShadowPass();

        graphics.BeginDrawGeometryPass();
        {
            for (auto renderer : m_opaqueList)
            {
                renderer->Draw();
            }
        }
        graphics.EndDrawGeometryPass();

        graphics.BeginDrawLightPass();
        {
            context->PSSetSamplers(static_cast<UINT>(SamplerSlot::Clamp), 1, m_clampSamplerState->GetSamplerState().GetAddressOf());
            context->PSSetSamplers(static_cast<UINT>(SamplerSlot::Comparison), 1, m_comparisonSamplerState->GetSamplerState().GetAddressOf());

            ID3D11ShaderResourceView* srvs[3]{ m_irradianceMap->GetRawSRV(), m_specularMap->GetRawSRV(), m_brdfLut->GetRawSRV() };
            context->PSSetShaderResources(static_cast<UINT>(TextureSlot::IBLIrradiance), 3, srvs);

            graphics.DrawFullscreenQuad();
        }
        graphics.EndDrawLightPass();

        graphics.BeginDrawForwardPass();
        {
            context->OMSetDepthStencilState(m_skyboxDSState->GetRawDepthStencilState(), 0);
            context->RSSetState(m_skyboxRSState->GetRawRasterizerState());

            const UINT stride = m_cubeVertexBuffer->GetBufferStride();
            const UINT offset = 0;
            context->IASetVertexBuffers(0, 1, m_cubeVertexBuffer->GetBuffer().GetAddressOf(), &stride, &offset);
            context->IASetIndexBuffer(m_cubeIndexBuffer->GetRawBuffer(), DXGI_FORMAT_R32_UINT, 0);
            context->IASetInputLayout(m_cubeInputLayout->GetRawInputLayout());

            context->VSSetShader(m_skyboxVertexShader->GetRawShader(), nullptr, 0);

            context->PSSetShader(m_skyboxPixelShader->GetRawShader(), nullptr, 0);
            context->PSSetSamplers(0, 1, m_linearSamplerState->GetSamplerState().GetAddressOf());
            context->PSSetShaderResources(static_cast<UINT>(TextureSlot::IBLEnvironment), 1, m_skyboxEnv->GetSRV().GetAddressOf());

            context->RSSetState(m_skyboxRSState->GetRawRasterizerState());

            context->OMSetDepthStencilState(m_skyboxDSState->GetRawDepthStencilState(), 0);

            context->DrawIndexed(m_indexCount, 0, 0);

            context->RSSetState(nullptr);
            context->OMSetDepthStencilState(nullptr, 0);
        }
        graphics.EndDrawForwardPass();

        graphics.BeginDrawPostProccessingPass();
        {

            graphics.DrawFullscreenQuad();
        }
        graphics.EndDrawPostProccessingPass();
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
