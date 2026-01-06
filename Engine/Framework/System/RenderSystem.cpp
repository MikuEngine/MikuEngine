#include "EnginePCH.h"
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
#include "Core/Graphics/Resource/BlendState.h"
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
        m_globalConstantBuffer = ResourceManager::Get().GetOrCreateConstantBuffer("CbGlobal", sizeof(CbGlobal));

        m_comparisonSamplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Comparison);
        m_clampSamplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Clamp);

        m_skyboxEnv = ResourceManager::Get().GetOrCreateTexture("Resource/Texture/fieldEnvHDR.dds");
        m_irradianceMap = ResourceManager::Get().GetOrCreateTexture("Resource/Texture/fieldDiffuseHDR.dds");
        m_specularMap = ResourceManager::Get().GetOrCreateTexture("Resource/Texture/fieldSpecularHDR.dds");
        m_brdfLut = ResourceManager::Get().GetOrCreateTexture("Resource/Texture/fieldBrdf.dds");

        // cube
        {
            std::vector<PositionVertex> vertices{
                // Back Face (Z = -0.5)
                { { -0.5f,  0.5f, -0.5f } }, // 0: Top-Left-Back
                { {  0.5f,  0.5f, -0.5f } }, // 1: Top-Right-Back
                { { -0.5f, -0.5f, -0.5f } }, // 2: Bottom-Left-Back
                { {  0.5f, -0.5f, -0.5f } }, // 3: Bottom-Right-Back

                // Front Face (Z = +0.5)
                { { -0.5f,  0.5f,  0.5f } }, // 4: Top-Left-Front
                { {  0.5f,  0.5f,  0.5f } }, // 5: Top-Right-Front
                { { -0.5f, -0.5f,  0.5f } }, // 6: Bottom-Left-Front
                { {  0.5f, -0.5f,  0.5f } }, // 7: Bottom-Right-Front
            };

            std::vector<DWORD> indices{
                // Back Face
                0, 1, 2,
                2, 1, 3,

                // Front Face
                5, 4, 7,
                7, 4, 6,

                // Left Face
                4, 0, 6,
                6, 0, 2,

                // Right Face
                1, 5, 3,
                3, 5, 7,

                // Top Face
                4, 5, 0,
                0, 5, 1,

                // Bottom Face
                2, 3, 6,
                6, 3, 7
            };

            m_indexCount = static_cast<UINT>(indices.size());
            m_cubeVertexBuffer = ResourceManager::Get().GetOrCreateVertexBuffer<PositionVertex>("SkyboxCube", vertices);
            m_cubeIndexBuffer = ResourceManager::Get().GetOrCreateIndexBuffer("SkyboxCube", indices);
            m_skyboxVertexShader = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Skybox_VS.hlsl");
            m_skyboxPixelShader = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Skybox_PS.hlsl");
            m_cubeInputLayout = m_skyboxVertexShader->GetOrCreateInputLayout<PositionVertex>();

            m_skyboxRSState = ResourceManager::Get().GetDefaultRasterizerState(DefaultRasterizerType::SolidFront);

            m_skyboxDSState = ResourceManager::Get().GetDefaultDepthStencilState(DefaultDepthStencilType::LessEqual);
            m_linearSamplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);

            m_transparentBlendState = ResourceManager::Get().GetDefaultBlendState(DefaultBlendType::AlphaBlend);
            m_transparentDSState = ResourceManager::Get().GetDefaultDepthStencilState(DefaultDepthStencilType::DepthRead);
        }
    }

    void RenderSystem::Register(Renderer* renderer)
    {
        System<Renderer>::Register(renderer);

        if (renderer->HasRenderType(RenderType::Opaque))
        {
            AddRenderer(m_opaqueList, renderer, RenderType::Opaque);
        }

        if (renderer->HasRenderType(RenderType::Cutout))
        {
            AddRenderer(m_cutoutList, renderer, RenderType::Cutout);
        }

        if (renderer->HasRenderType(RenderType::Transparent))
        {
            AddRenderer(m_transparentList, renderer, RenderType::Transparent);
        }

        if (renderer->HasRenderType(RenderType::Screen))
        {
            AddRenderer(m_screenList, renderer, RenderType::Screen);
        }
    }

    void RenderSystem::Unregister(Renderer* renderer)
    {
        RemoveRenderer(m_opaqueList, renderer, RenderType::Opaque);
        RemoveRenderer(m_cutoutList, renderer, RenderType::Cutout);
        RemoveRenderer(m_transparentList, renderer, RenderType::Transparent);
        RemoveRenderer(m_screenList, renderer, RenderType::Screen);

        System<Renderer>::Unregister(renderer);
    }

    void RenderSystem::Update()
    {
        for (auto renderer : m_components)
        {
            renderer->Update();
        }
    }

    void RenderSystem::Render()
    {
        auto& graphics = GraphicsDevice::Get();
        auto context = GraphicsDevice::Get().GetDeviceContext();

        Matrix view, projection;
        Vector3 cameraPosition;

        bool isCameraOff = false;

#ifdef _DEBUG
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
                isCameraOff = true;
                break;
            }

            view = cam->GetView();
            projection = cam->GetProjection();
            cameraPosition = GetTranslation(cam->GetWorld());
        }
            break;
        }
#else
        auto* cam = SystemManager::Get().GetCameraSystem().GetMainCamera();
        if (cam == nullptr)
        {
            return;
        }

        view = cam->GetView();
        projection = cam->GetProjection();
        cameraPosition = GetTranslation(cam->GetWorld());
#endif // _DEBUG

        const Matrix viewProjection = view * projection;

        CbGlobal cbGlobal;
        cbGlobal.view = view.Transpose();
        cbGlobal.projection = projection.Transpose();
        cbGlobal.viewProjection = viewProjection.Transpose();
        cbGlobal.invViewProjection = viewProjection.Invert().Transpose();
        cbGlobal.cameraWorldPoistion = cameraPosition;
        cbGlobal.elapsedTime = Time::GetElapsedSeconds(g_startTime);
        cbGlobal.mainLightViewProjection = cbGlobal.viewProjection;
        cbGlobal.mainLightWorldDirection = Vector3(0.0f, 0.0f, 1.0f);
        cbGlobal.mainLightColor = Vector3(1.0f, 1.0f, 1.0f);
        cbGlobal.mainLightIntensity = 5.0f;
        cbGlobal.maxHDRNits = graphics.GetMaxHDRNits();
        cbGlobal.exposure = -2.0f;
        cbGlobal.shadowMapSize = graphics.GetShadowMapSize();
        cbGlobal.useShadowPCF = 0;
        cbGlobal.pcfSize = 2;
        cbGlobal.useIBL = 1;
        cbGlobal.bloomStrength = m_bloomStrength;
        cbGlobal.bloomThreshold = m_bloomThreshold;
        cbGlobal.bloomSoftKnee = m_bloomSoftKnee;
        cbGlobal.fxaaQualitySubpix = 0.75f;           // 0.0 to 1.0 (default: 0.75)
        cbGlobal.fxaaQualityEdgeThreshold = 0.166f;    // 0.063 to 0.333 (default: 0.166)
        cbGlobal.fxaaQualityEdgeThresholdMin = 0.0833f; // 0.0312 to 0.0833 (default: 0.0833)

        context->VSSetConstantBuffers(
            static_cast<UINT>(ConstantBufferSlot::Global),
            1,
            m_globalConstantBuffer->GetBuffer().GetAddressOf());
        context->PSSetConstantBuffers(
            static_cast<UINT>(ConstantBufferSlot::Global),
            1,
            m_globalConstantBuffer->GetBuffer().GetAddressOf());

        context->UpdateSubresource(m_globalConstantBuffer->GetRawBuffer(), 0, nullptr, &cbGlobal, 0, 0);

        graphics.ClearAllViews();

        if (!isCameraOff)
        {
            graphics.BeginDrawShadowPass();
            {
                for (auto renderer : m_opaqueList)
                {
                    renderer->Draw(RenderType::Shadow);
                }

                for (auto renderer : m_cutoutList)
                {
                    renderer->Draw(RenderType::Shadow);
                }
            }
            graphics.EndDrawShadowPass();

            graphics.BeginDrawGeometryPass();
            {
                for (auto renderer : m_opaqueList)
                {
                    renderer->Draw(RenderType::Opaque);
                }

                for (auto renderer : m_cutoutList)
                {
                    renderer->Draw(RenderType::Cutout);
                }
            }
            graphics.EndDrawGeometryPass();

            graphics.BeginDrawLightPass();
            {
                context->PSSetSamplers(static_cast<UINT>(SamplerSlot::Clamp), 1, m_clampSamplerState->GetSamplerState().GetAddressOf());
                context->PSSetSamplers(static_cast<UINT>(SamplerSlot::Comparison), 1, m_comparisonSamplerState->GetSamplerState().GetAddressOf());

                ID3D11ShaderResourceView* srvs[3]{ m_irradianceMap->GetRawSRV(), m_specularMap->GetRawSRV(), m_brdfLut->GetRawSRV() };
                context->PSSetShaderResources(static_cast<UINT>(TextureSlot::IBLIrradiance), 3, srvs);
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

                context->OMSetBlendState(m_transparentBlendState->GetRawBlendState(), nullptr, 0xFFFFFFFF);
                context->OMSetDepthStencilState(m_transparentDSState->GetRawDepthStencilState(), 0);

                static std::vector<std::pair<float, Renderer*>> sortList;
                sortList.clear();
                sortList.reserve(m_transparentList.size());
                Vector3 camPos = cameraPosition;
                
                for (auto* renderer : m_transparentList)
                {
                    float distSq = Vector3::DistanceSquared(camPos, renderer->GetTransform()->GetWorld().Translation());
                    sortList.emplace_back(distSq, renderer);
                }
                
                std::sort(sortList.begin(), sortList.end(),
                    [](const auto& a, const auto& b) {
                        return a.first > b.first;
                    });

                for (auto pair : sortList)
                {
                    pair.second->Draw(RenderType::Transparent);
                }

                context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
                context->OMSetDepthStencilState(nullptr, 0);
            }
            graphics.EndDrawForwardPass();
        }

        graphics.ExecutePostProcessing();

        graphics.BeginDrawScreenPass();
        {
            for (auto renderer : m_screenList)
            {
                renderer->Draw(RenderType::Screen);
            }
        }
        graphics.EndDrawScreenPass();
    }

    void RenderSystem::GetBloomSettings(float& bloomStrength, float& bloomThreshold, float& bloomSoftKnee)
    {
        bloomStrength = m_bloomStrength;
        bloomThreshold = m_bloomThreshold;
        bloomSoftKnee = m_bloomSoftKnee;
    }

    void RenderSystem::SetBloomSettings(float bloomStrength, float bloomThreshold, float bloomSoftKnee)
    {
        m_bloomStrength = bloomStrength;
        m_bloomThreshold = bloomThreshold;
        m_bloomSoftKnee = bloomSoftKnee;
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
