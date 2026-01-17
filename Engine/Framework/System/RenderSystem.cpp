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
#include "Framework/System/LightSystem.h"
#include "Framework/Object/Component/Light.h"

#include "Editor/EditorManager.h"
#include "Editor/EditorCamera.h"

#include "Framework/Physics/PhysicsDebugRenderer.h"

namespace engine
{
    namespace
    {
        TimePoint g_startTime = Clock::now();
    }

    RenderSystem::RenderSystem()
    {
        m_frameCB = ResourceManager::Get().GetOrCreateConstantBuffer("Frame", sizeof(CbFrame));

        m_comparisonSamplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Comparison);
        m_clampSamplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Clamp);

        m_skyboxEnv = ResourceManager::Get().GetOrCreateTexture("Resource/Texture/PlainsSunsetEnvHDR.dds");
        m_irradianceMap = ResourceManager::Get().GetOrCreateTexture("Resource/Texture/PlainsSunsetDiffuseHDR.dds");
        m_specularMap = ResourceManager::Get().GetOrCreateTexture("Resource/Texture/PlainsSunsetSpecularHDR.dds");
        m_brdfLut = ResourceManager::Get().GetOrCreateTexture("Resource/Texture/PlainsSunsetBrdf.dds");

        // cube
        {
            m_cubeVertexBuffer = ResourceManager::Get().GetGeometryVertexBuffer("DefaultCube");
            m_cubeIndexBuffer = ResourceManager::Get().GetGeometryIndexBuffer("DefaultCube");
            m_skyboxVertexShader = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/Skybox_VS.hlsl");
            m_skyboxPixelShader = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/Skybox_PS.hlsl");
            m_cubeInputLayout = m_skyboxVertexShader->GetOrCreateInputLayout<PositionVertex>();

            m_skyboxRSState = ResourceManager::Get().GetDefaultRasterizerState(DefaultRasterizerType::SolidFront);

            m_skyboxDSState = ResourceManager::Get().GetDefaultDepthStencilState(DefaultDepthStencilType::LessEqual);
            m_linearSamplerState = ResourceManager::Get().GetDefaultSamplerState(DefaultSamplerType::Linear);

            m_transparentBlendState = ResourceManager::Get().GetDefaultBlendState(DefaultBlendType::AlphaBlend);
            m_transparentDSState = ResourceManager::Get().GetDefaultDepthStencilState(DefaultDepthStencilType::DepthRead);
        }

        // light
        {
            m_sphereVB = ResourceManager::Get().GetGeometryVertexBuffer("DefaultSphere");
            m_sphereIB = ResourceManager::Get().GetGeometryIndexBuffer("DefaultSphere");
            m_coneVB = ResourceManager::Get().GetGeometryVertexBuffer("DefaultCone");
            m_coneIB = ResourceManager::Get().GetGeometryIndexBuffer("DefaultCone");

            m_lightVolumeVS = ResourceManager::Get().GetOrCreateVertexShader("Resource/Shader/Vertex/LightVolume_VS.hlsl");
            m_pointLightPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/DeferredPointLight_PS.hlsl");
            m_spotLightPS = ResourceManager::Get().GetOrCreatePixelShader("Resource/Shader/Pixel/DeferredSpotLight_PS.hlsl");
            m_lightVolumeInputLayout = m_lightVolumeVS->GetOrCreateInputLayout<PositionVertex>();
            m_localLightCB = ResourceManager::Get().GetOrCreateConstantBuffer("LocalLight", sizeof(CbLocalLight));
            m_objectCB = ResourceManager::Get().GetOrCreateConstantBuffer("Object", sizeof(CbObject));

            {
                D3D11_BLEND_DESC desc{};
                desc.RenderTarget[0].BlendEnable = TRUE;
                desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
                desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
                desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
                desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
                desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
                desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
                desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

                m_additiveBS = ResourceManager::Get().GetOrCreateBlendState("LightAdditive", desc);
            }

            {
                D3D11_RASTERIZER_DESC desc{};
                desc.FillMode = D3D11_FILL_SOLID;
                desc.CullMode = D3D11_CULL_FRONT;

                m_frontRSS = ResourceManager::Get().GetOrCreateRasterizerState("LightVolume", desc);
            }

            {
                D3D11_DEPTH_STENCIL_DESC desc{};
                desc.DepthEnable = TRUE;
                desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // Disable depth write
                desc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
                desc.StencilEnable = FALSE;

                m_lightVolumeDSS = ResourceManager::Get().GetOrCreateDepthStencilState("LightVolume", desc);
            }
        }

        // picking
        {
            m_pickingIdCB = ResourceManager::Get().GetOrCreateConstantBuffer("PickingId", sizeof(CbPickingId));
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
        const auto& context = GraphicsDevice::Get().GetDeviceContext();

        Matrix view, projection;
        Vector3 cameraPosition;
        Vector3 cameraForward;

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
            cameraForward = GetForward(view.Invert());
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
            cameraPosition = cam->GetPosition();
            cameraForward = cam->GetForward();
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
        cameraPosition = cam->GetPosition();
        cameraForward = cam->GetForward();
#endif // _DEBUG

        const Matrix viewProjection = view * projection;

        auto& lightSystem = SystemManager::Get().GetLightSystem();
        auto* mainLight = lightSystem.GetMainLight();
        Vector3 lightDir = Vector3(0.0f, -1.0f, 0.0f); // 조명 없을 때 기본 방향
        Vector3 lightColor = Vector3(0.0f, 0.0f, 0.0f);

        Matrix lightView, lightProjection;

        float lightIntensity = 1.0f;
        if (mainLight != nullptr)
        {
            Vector3 lightUp = mainLight->GetTransform()->GetForward();
            lightDir = -mainLight->GetTransform()->GetUp();
            lightColor = mainLight->GetColor();
            lightIntensity = mainLight->GetIntensity();

            Vector3 focusPosition = cameraPosition + cameraForward * mainLight->GetForwardDist();
            Vector3 lightPosition = focusPosition + -lightDir * mainLight->GetLightFar() * mainLight->GetHeightRatio();
            lightView = DirectX::XMMatrixLookAtLH(lightPosition, focusPosition, lightUp);
            lightProjection = DirectX::XMMatrixPerspectiveFovLH(
                ToRadian(mainLight->GetAngle()),
                1.0f,
                mainLight->GetLightNear(),
                mainLight->GetLightFar());
        }
        lightDir.Normalize();

        CbFrame cbFrame;
        cbFrame.view = view.Transpose();
        cbFrame.projection = projection.Transpose();
        cbFrame.viewProjection = viewProjection.Transpose();
        cbFrame.invViewProjection = viewProjection.Invert().Transpose();
        cbFrame.cameraWorldPoistion = cameraPosition;
        cbFrame.elapsedTime = Time::GetElapsedSeconds(g_startTime);
        cbFrame.mainLightViewProjection = (lightView * lightProjection).Transpose();
        cbFrame.mainLightWorldDirection = lightDir;
        cbFrame.mainLightColor = lightColor;
        cbFrame.mainLightIntensity = lightIntensity;
        cbFrame.maxHDRNits = graphics.GetMaxHDRNits();
        cbFrame.exposure = -2.5f;
        cbFrame.shadowMapSize = graphics.GetShadowMapSize();
        cbFrame.useShadowPCF = 1;
        cbFrame.pcfSize = 2;
        cbFrame.useIBL = 1;
        cbFrame.bloomStrength = m_bloomStrength;
        cbFrame.bloomThreshold = m_bloomThreshold;
        cbFrame.bloomSoftKnee = m_bloomSoftKnee;
        cbFrame.fxaaQualitySubpix = 0.75f;           // 0.0 to 1.0 (default: 0.75)
        cbFrame.fxaaQualityEdgeThreshold = 0.166f;    // 0.063 to 0.333 (default: 0.166)
        cbFrame.fxaaQualityEdgeThresholdMin = 0.0833f; // 0.0312 to 0.0833 (default: 0.0833)

        context->VSSetConstantBuffers(
            static_cast<UINT>(ConstantBufferSlot::Frame),
            1,
            m_frameCB->GetBuffer().GetAddressOf());
        context->PSSetConstantBuffers(
            static_cast<UINT>(ConstantBufferSlot::Frame),
            1,
            m_frameCB->GetBuffer().GetAddressOf());

        context->UpdateSubresource(m_frameCB->GetRawBuffer(), 0, nullptr, &cbFrame, 0, 0);

        graphics.ClearAllViews();

        if (!isCameraOff)
        {
            graphics.BeginDrawShadowPass();
            {
                for (auto renderer : m_opaqueList)
                {
                    if (renderer->IsActive())
                    {
                        renderer->Draw(RenderType::Shadow);
                    }
                }

                for (auto renderer : m_cutoutList)
                {
                    if (renderer->IsActive())
                    {
                        renderer->Draw(RenderType::Shadow);
                    }
                }
            }
            graphics.EndDrawShadowPass();

            graphics.BeginDrawGeometryPass();
            {
                for (auto renderer : m_opaqueList)
                {
                    if (renderer->IsActive())
                    {
                        renderer->Draw(RenderType::Opaque);
                    }
                }

                for (auto renderer : m_cutoutList)
                {
                    if (renderer->IsActive())
                    {
                        renderer->Draw(RenderType::Cutout);
                    }
                }
            }
            graphics.EndDrawGeometryPass();

            graphics.BeginDrawLightPass();
            {
                DrawGlobalLight();

                DrawLocalLight();
            }
            graphics.EndDrawLightPass();

            graphics.BeginDrawForwardPass();
            {
                context->OMSetDepthStencilState(m_skyboxDSState->GetRawDepthStencilState(), 0);
                context->RSSetState(m_skyboxRSState->GetRawRasterizerState());

                const UINT stride = m_cubeVertexBuffer->GetBufferStride();
                const UINT offset = 0;
                context->IASetVertexBuffers(0, 1, m_cubeVertexBuffer->GetBuffer().GetAddressOf(), &stride, &offset);
                context->IASetIndexBuffer(m_cubeIndexBuffer->GetRawBuffer(), m_cubeIndexBuffer->GetIndexFormat(), 0);
                context->IASetInputLayout(m_cubeInputLayout->GetRawInputLayout());

                context->VSSetShader(m_skyboxVertexShader->GetRawShader(), nullptr, 0);

                context->PSSetShader(m_skyboxPixelShader->GetRawShader(), nullptr, 0);
                context->PSSetSamplers(0, 1, m_linearSamplerState->GetSamplerState().GetAddressOf());
                context->PSSetShaderResources(static_cast<UINT>(TextureSlot::IBLEnvironment), 1, m_skyboxEnv->GetSRV().GetAddressOf());

                context->RSSetState(m_skyboxRSState->GetRawRasterizerState());

                context->OMSetDepthStencilState(m_skyboxDSState->GetRawDepthStencilState(), 0);

                context->DrawIndexed(m_cubeIndexBuffer->GetIndexCount(), 0, 0);

                context->RSSetState(nullptr);
                context->OMSetDepthStencilState(nullptr, 0);

#ifdef _DEBUG
                if (EditorManager::Get().GetEditorState() != EditorState::Play)
                {
                    EditorManager::Get().DrawEditorGrid();
                }
#endif // _DEBUG

                context->OMSetBlendState(m_transparentBlendState->GetRawBlendState(), nullptr, 0xFFFFFFFF);
                context->OMSetDepthStencilState(m_transparentDSState->GetRawDepthStencilState(), 0);

                static std::vector<std::pair<float, Renderer*>> sortList;
                sortList.clear();
                if (sortList.capacity() < m_transparentList.size())
                {
                    sortList.reserve(static_cast<size_t>(m_transparentList.size() * 1.5f));
                }
                Vector3 camPos = cameraPosition;
                
                for (auto* renderer : m_transparentList)
                {
                    if (renderer->IsActive())
                    {
                        float distSq = Vector3::DistanceSquared(camPos, renderer->GetTransform()->GetWorld().Translation());
                        sortList.emplace_back(distSq, renderer);
                    }
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
                if (renderer->IsActive())
                {
                    renderer->Draw(RenderType::Screen);
                }
            }
        }
        graphics.EndDrawScreenPass();

#ifdef _DEBUG
        if (auto gameObject = EditorManager::Get().GetSelectedObject(); gameObject != nullptr)
        {
            graphics.BeginDrawOutlinePass();
            {
                auto renderer = gameObject->GetComponent<Renderer>();
                if (renderer != nullptr)
                {
                    renderer->DrawMask();
                }
            }
            graphics.EndDrawOutlinePass();
        }
#endif

        // Physics Debug Rendering (PostProcessing 이후, finalBuffer에 렌더링)
        graphics.BeginDrawDebugPass();
        {
            PhysicsDebugRenderer::Get().Render(view, projection);
        }
        graphics.EndDrawDebugPass();
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

    GameObject* RenderSystem::PickObject(int mouseX, int mouseY)
    {
        auto& graphics = GraphicsDevice::Get();
        const auto& context = graphics.GetDeviceContext();

        graphics.BeginDrawPickingPass();
        {
            context->PSSetConstantBuffers(static_cast<UINT>(ConstantBufferSlot::PickingId), 1, m_pickingIdCB->GetBuffer().GetAddressOf());
            
            for (unsigned int i = 0; i < m_components.size(); ++i)
            {
                if (!m_components[i]->IsActive())
                {
                    continue;
                }

                CbPickingId cb{};
                cb.pickingId = i + 1;

                context->UpdateSubresource(m_pickingIdCB->GetRawBuffer(), 0, nullptr, &cb, 0, 0);

                m_components[i]->DrawPickingID();
            }
        }
        unsigned int picked = graphics.EndDrawPickingPass(mouseX, mouseY);

        if (picked != 0)
        {
            return m_components[picked - 1]->GetGameObject();
        }

        return nullptr;
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

    void RenderSystem::DrawGlobalLight()
    {
        const auto& context = GraphicsDevice::Get().GetDeviceContext();

        context->PSSetSamplers(static_cast<UINT>(SamplerSlot::Clamp), 1, m_clampSamplerState->GetSamplerState().GetAddressOf());
        context->PSSetSamplers(static_cast<UINT>(SamplerSlot::Comparison), 1, m_comparisonSamplerState->GetSamplerState().GetAddressOf());

        ID3D11ShaderResourceView* srvs[3]{ m_irradianceMap->GetRawSRV(), m_specularMap->GetRawSRV(), m_brdfLut->GetRawSRV() };
        context->PSSetShaderResources(static_cast<UINT>(TextureSlot::IBLIrradiance), 3, srvs);

        GraphicsDevice::Get().DrawFullscreenQuad();

    }

    void RenderSystem::DrawLocalLight()
    {
        auto& graphics = GraphicsDevice::Get();
        auto* context = graphics.GetDeviceContext().Get();
        auto& lightSystem = SystemManager::Get().GetLightSystem();
        const auto& lights = lightSystem.GetLights();

        context->VSSetShader(m_lightVolumeVS->GetRawShader(), nullptr, 0);
        context->RSSetState(m_frontRSS->GetRawRasterizerState());
        context->OMSetDepthStencilState(m_lightVolumeDSS->GetRawDepthStencilState(), 0);
        static const float blendFactor[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
        context->OMSetBlendState(m_additiveBS->GetRawBlendState(), blendFactor, 0xffffffff);

        // CB 바인딩
        context->PSSetConstantBuffers(6, 1, m_localLightCB->GetBuffer().GetAddressOf());
        for (auto* light : lights)
        {
            if (!light->IsActive())
            {
                continue;
            }
            if (light->GetLightType() == LightType::Directional)
            {
                continue; // 이미 처리함
            }

            // Transform 계산 (라이트 위치/크기에 맞춰 World Matrix 생성)
            Matrix world = Matrix::Identity;
            float range = light->GetRange();

            if (light->GetLightType() == LightType::Point)
            {
                // Point Light: 구체 스케일 = Range
                world = Matrix::CreateScale(range * 2) * Matrix::CreateTranslation(light->GetTransform()->GetWorldPosition());

                // CB 데이터 채우기
                CbLocalLight cbData;
                cbData.lightColor = light->GetColor();
                cbData.lightIntensity = light->GetIntensity();
                cbData.lightPosition = light->GetTransform()->GetWorldPosition();
                cbData.lightRange = range;

                // PS 설정
                context->PSSetShader(m_pointLightPS->GetRawShader(), nullptr, 0);

                // 버퍼 업데이트
                context->UpdateSubresource(m_localLightCB->GetRawBuffer(), 0, nullptr, &cbData, 0, 0);

                // World Matrix 업데이트 (Object CB 사용)
                CbObject cbObj;
                cbObj.world = world.Transpose(); // HLSL은 열우선
                context->UpdateSubresource(m_objectCB->GetRawBuffer(), 0, nullptr, &cbObj, 0, 0);

                // Draw Sphere
                static const UINT stride = m_sphereVB->GetBufferStride();
                static const UINT offset = 0;
                context->IASetVertexBuffers(0, 1, m_sphereVB->GetBuffer().GetAddressOf(), &stride, &offset);
                context->IASetIndexBuffer(m_sphereIB->GetRawBuffer(), m_sphereIB->GetIndexFormat(), 0);
                context->DrawIndexed(m_sphereIB->GetIndexCount(), 0, 0);
            }
            else if (light->GetLightType() == LightType::Spot)
            {
                // 1. Light의 World Matrix에서 Basis Vector 추출 및 정규화 (Scale 제거)
                    //    Decompose 비용 회피
                Matrix lightWorld = light->GetTransform()->GetWorld();
                Vector3 right(lightWorld._11, lightWorld._12, lightWorld._13);
                Vector3 up(lightWorld._21, lightWorld._22, lightWorld._23);
                Vector3 forward(lightWorld._31, lightWorld._32, lightWorld._33);
                right.Normalize();
                up.Normalize();
                forward.Normalize(); // 이것이 실제 Light Direction (Z+)
                // 2. Scale이 제거된 Unscaled Light World Matrix (Rotation + Translation)
                Matrix unscaledLightWorld = Matrix::Identity;
                unscaledLightWorld._11 = right.x;   unscaledLightWorld._12 = right.y;   unscaledLightWorld._13 = right.z;
                unscaledLightWorld._21 = up.x;      unscaledLightWorld._22 = up.y;      unscaledLightWorld._23 = up.z;
                unscaledLightWorld._31 = forward.x; unscaledLightWorld._32 = forward.y; unscaledLightWorld._33 = forward.z;

                // Translation
                unscaledLightWorld._41 = lightWorld._41;
                unscaledLightWorld._42 = lightWorld._42;
                unscaledLightWorld._43 = lightWorld._43;
                // ------------------------------------------------------------------
                float angle = light->GetAngle(); // Half-Angle (Degree)
                // 3. Offset: Cone Mesh의 Tip(0, 0.5, 0)을 원점(0, 0, 0)으로 내림
                Matrix offset = Matrix::CreateTranslation(0.0f, -0.5f, 0.0f);
                // 4. Scale: Cone 크기 설정
                //    - 높이(Y): 1.0 -> range
                //    - 밑면 반지름(XZ): 0.5 -> range * tan(angle) 이므로 2배 적용
                float coneRadius = range * std::tan(ToRadian(angle));

                // Cone Mesh는 Y축으로 서 있으므로 Y축에 range(길이)를 곱함
                Matrix coneScale = Matrix::CreateScale(coneRadius * 2.0f * 2.0f, range * 2.0f, coneRadius * 2.0f * 2.0f);
                // 5. Pre-Rotate: 
                //    Cone Mesh는 기본적으로 -Y 방향을 보고 있음 (Tip이 위, Bottom이 아래).
                //    우리는 Light의 Forward인 Z+ 방향으로 빛을 쏘고 싶음.
                //    -Y -> +Z 로 회전시키려면 X축 기준 +90도 회전
                Matrix preRotate = Matrix::CreateRotationX(ToRadian(-90.0f));
                // 6. 최종 World Matrix 조합
                //    순서: Offset -> ConeScale -> PreRotate -> UnscaledLightWorld
                world = offset * coneScale * preRotate * unscaledLightWorld;
                // --- CB 업데이트 ---
                CbLocalLight cbData;
                cbData.lightColor = light->GetColor();
                cbData.lightIntensity = light->GetIntensity();
                cbData.lightPosition = Vector3(lightWorld._41, lightWorld._42, lightWorld._43);
                cbData.lightRange = range;

                // Light Direction: 이제 Z+ (Forward) 벡터를 사용
                cbData.lightDirection = forward;

                cbData.lightAngle = angle;

                // 셰이더 설정 및 그리기
                context->PSSetShader(m_spotLightPS->GetRawShader(), nullptr, 0);
                context->UpdateSubresource(m_localLightCB->GetRawBuffer(), 0, nullptr, &cbData, 0, 0);
                CbObject cbObj;
                cbObj.world = world.Transpose();
                context->UpdateSubresource(m_objectCB->GetRawBuffer(), 0, nullptr, &cbObj, 0, 0);
                static const UINT stride = m_coneVB->GetBufferStride();
                static const UINT offsetVal = 0;
                context->IASetVertexBuffers(0, 1, m_coneVB->GetBuffer().GetAddressOf(), &stride, &offsetVal);
                context->IASetIndexBuffer(m_coneIB->GetRawBuffer(), m_coneIB->GetIndexFormat(), 0);
                context->DrawIndexed(m_coneIB->GetIndexCount(), 0, 0);
            }
        }

        context->RSSetState(nullptr);
        context->OMSetDepthStencilState(nullptr, 0);
        context->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
    }

    void RenderSystem::DrawSkybox()
    {
        const auto& context = GraphicsDevice::Get().GetDeviceContext();

        context->OMSetDepthStencilState(m_skyboxDSState->GetRawDepthStencilState(), 0);
        context->RSSetState(m_skyboxRSState->GetRawRasterizerState());

        const UINT stride = m_cubeVertexBuffer->GetBufferStride();
        const UINT offset = 0;
        context->IASetVertexBuffers(0, 1, m_cubeVertexBuffer->GetBuffer().GetAddressOf(), &stride, &offset);
        context->IASetIndexBuffer(m_cubeIndexBuffer->GetRawBuffer(), m_cubeIndexBuffer->GetIndexFormat(), 0);
        context->IASetInputLayout(m_cubeInputLayout->GetRawInputLayout());

        context->VSSetShader(m_skyboxVertexShader->GetRawShader(), nullptr, 0);

        context->PSSetShader(m_skyboxPixelShader->GetRawShader(), nullptr, 0);
        context->PSSetSamplers(0, 1, m_linearSamplerState->GetSamplerState().GetAddressOf());
        context->PSSetShaderResources(static_cast<UINT>(TextureSlot::IBLEnvironment), 1, m_skyboxEnv->GetSRV().GetAddressOf());

        context->RSSetState(m_skyboxRSState->GetRawRasterizerState());

        context->OMSetDepthStencilState(m_skyboxDSState->GetRawDepthStencilState(), 0);

        context->DrawIndexed(m_cubeIndexBuffer->GetIndexCount(), 0, 0);

        context->RSSetState(nullptr);
        context->OMSetDepthStencilState(nullptr, 0);
    }

    void RenderSystem::DrawTransparents(const Vector3& cameraPosition)
    {
        const auto& context = GraphicsDevice::Get().GetDeviceContext();

        context->OMSetBlendState(m_transparentBlendState->GetRawBlendState(), nullptr, 0xFFFFFFFF);
        context->OMSetDepthStencilState(m_transparentDSState->GetRawDepthStencilState(), 0);

        static std::vector<std::pair<float, Renderer*>> sortList;
        sortList.clear();
        if (sortList.capacity() < m_transparentList.size())
        {
            sortList.reserve(static_cast<size_t>(m_transparentList.size() * 1.5f));
        }

        for (auto* renderer : m_transparentList)
        {
            if (renderer->IsActive())
            {
                float distSq = Vector3::DistanceSquared(cameraPosition, renderer->GetTransform()->GetWorld().Translation());
                sortList.emplace_back(distSq, renderer);
            }
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
}
