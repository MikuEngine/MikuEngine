#include "EnginePCH.h"
#include "LightDebugRenderer.h"

#include "Framework/Object/Component/Light/Light.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/LightSystem.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"
#include "Core/Graphics/Device/GraphicsDevice.h"
#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Resource/DepthStencilState.h"

namespace engine
{
    void LightDebugRenderer::Initialize()
    {
        if (m_isInitialized)
            return;

        auto device = GraphicsDevice::Get().GetDevice().Get();
        auto context = GraphicsDevice::Get().GetDeviceContext().Get();

        // CommonStates
        m_states = std::make_unique<DirectX::CommonStates>(device);

        // BasicEffect 생성
        m_effect = std::make_unique<DirectX::BasicEffect>(device);
        m_effect->SetVertexColorEnabled(true);
        m_effect->SetLightingEnabled(false);

        // InputLayout 생성
        void const* shaderByteCode;
        size_t byteCodeLength;
        m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        HRESULT hr = device->CreateInputLayout(
            DirectX::VertexPositionColor::InputElements,
            DirectX::VertexPositionColor::InputElementCount,
            shaderByteCode, byteCodeLength,
            m_inputLayout.ReleaseAndGetAddressOf()
        );

        if (FAILED(hr))
        {
            LOG_ERROR("[LightDebugRenderer] Failed to create input layout");
            return;
        }

        // PrimitiveBatch 생성
        m_batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);

        m_isInitialized = true;
        LOG_INFO("[LightDebugRenderer] Initialized");
    }

    void LightDebugRenderer::Shutdown()
    {
        m_batch.reset();
        m_effect.reset();
        m_inputLayout.Reset();
        m_states.reset();
        m_isInitialized = false;
    }

    void LightDebugRenderer::Render(const Matrix& view, const Matrix& projection)
    {
        if (!m_enabled || !m_isInitialized)
            return;

        auto context = GraphicsDevice::Get().GetDeviceContext().Get();

        // ═══════════════════════════════════════════════════════════════
        // 이전 상태 저장
        // ═══════════════════════════════════════════════════════════════
        Microsoft::WRL::ComPtr<ID3D11BlendState> prevBlendState;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> prevDepthState;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> prevRasterState;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> prevInputLayout;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> prevVS;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> prevPS;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView> prevDSV;
        D3D11_PRIMITIVE_TOPOLOGY prevTopology;
        D3D11_VIEWPORT prevViewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        UINT numViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
        FLOAT prevBlendFactor[4];
        UINT prevSampleMask;
        UINT prevStencilRef;

        // 현재 상태 가져오기
        context->RSGetViewports(&numViewports, prevViewports);
        context->OMGetBlendState(prevBlendState.GetAddressOf(), prevBlendFactor, &prevSampleMask);
        context->OMGetDepthStencilState(prevDepthState.GetAddressOf(), &prevStencilRef);
        context->RSGetState(prevRasterState.GetAddressOf());
        context->IAGetInputLayout(prevInputLayout.GetAddressOf());
        context->VSGetShader(prevVS.GetAddressOf(), nullptr, nullptr);
        context->PSGetShader(prevPS.GetAddressOf(), nullptr, nullptr);
        context->IAGetPrimitiveTopology(&prevTopology);

        // 렌더링 설정
        context->IASetInputLayout(m_inputLayout.Get());
        context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
        
        // 깊이 테스트 완전 비활성화 (PhysicsDebugRenderer와 동일)
        auto depthNoneState = ResourceManager::Get().GetDefaultDepthStencilState(DefaultDepthStencilType::None);
        context->OMSetDepthStencilState(depthNoneState->GetRawDepthStencilState(), 0);
        context->RSSetState(m_states->CullNone());

        m_effect->SetView(view);
        m_effect->SetProjection(projection);
        m_effect->SetWorld(Matrix::Identity);
        m_effect->Apply(context);

        m_batch->Begin();

        // Light 렌더링
        RenderLights();

        m_batch->End();

        // 이전 상태 복원
        context->RSSetViewports(numViewports, prevViewports);
        context->OMSetBlendState(prevBlendState.Get(), prevBlendFactor, prevSampleMask);
        context->OMSetDepthStencilState(prevDepthState.Get(), prevStencilRef);
        context->RSSetState(prevRasterState.Get());
        context->IASetInputLayout(prevInputLayout.Get());
        context->VSSetShader(prevVS.Get(), nullptr, 0);
        context->PSSetShader(prevPS.Get(), nullptr, 0);
        context->IASetPrimitiveTopology(prevTopology);
    }

    void LightDebugRenderer::RenderLights()
    {
        // LightSystem에서 등록된 Light들 가져오기
        const auto& lights = SystemManager::Get().GetLightSystem().GetLights();

        // 에디터 모드 체크 (등록된 Light가 없으면 씬에서 직접 찾기)
        bool isEditMode = lights.empty();
        std::vector<Light*> lightList;

        if (!isEditMode)
        {
            lightList = lights;
        }
        else
        {
            // 씬에서 직접 찾기 (Edit 모드)
            Scene* scene = SceneManager::Get().GetScene();
            if (scene)
            {
                for (const auto& go : scene->GetGameObjects())
                {
                    if (!go) continue;
                    if (Light* light = go->GetComponent<Light>())
                    {
                        lightList.push_back(light);
                    }
                }
            }
        }

        for (Light* light : lightList)
        {
            if (!light || !light->IsActive())
                continue;

            LightType type = light->GetLightType();

            switch (type)
            {
            case LightType::Directional:
                if (m_showDirectional)
                    RenderDirectionalLight(light);
                break;
            case LightType::Point:
                if (m_showPoint)
                    RenderPointLight(light);
                break;
            case LightType::Spot:
                if (m_showSpot)
                    RenderSpotLight(light);
                break;
            }
        }
    }

    void LightDebugRenderer::RenderDirectionalLight(Light* light)
    {
        if (!light)
            return;

        Transform* transform = light->GetTransform();
        if (!transform)
            return;

        Vector3 worldPos = transform->GetWorldPosition();
        Matrix world = transform->GetWorld();
        
        // 방향 추출 (Transform의 forward 방향)
        Vector3 forward = Vector3::TransformNormal(Vector3::UnitZ, world);
        forward.Normalize();

        DirectX::XMVECTOR color = DirectX::XMLoadFloat4(
            reinterpret_cast<const DirectX::XMFLOAT4*>(&m_directionalColor));

        // 방향 화살표 그리기
        if (m_showDirection)
        {
            float arrowLength = 5.0f; // Directional Light는 무한대이므로 시각화용 길이
            DrawArrow(worldPos, forward, arrowLength, color);
        }
    }

    void LightDebugRenderer::RenderPointLight(Light* light)
    {
        if (!light)
            return;

        Transform* transform = light->GetTransform();
        if (!transform)
            return;

        Vector3 worldPos = transform->GetWorldPosition();
        float range = light->GetRange();

        DirectX::XMVECTOR color = DirectX::XMLoadFloat4(
            reinterpret_cast<const DirectX::XMFLOAT4*>(&m_pointColor));
        DirectX::XMVECTOR rangeColor = DirectX::XMLoadFloat4(
            reinterpret_cast<const DirectX::XMFLOAT4*>(&m_rangeColor));

        // 범위 표시 (구체)
        if (m_showRange)
        {
            DrawSphere(worldPos, range, rangeColor);
        }

        // 중심점 표시
        DrawSphere(worldPos, 0.2f, color);
    }

    void LightDebugRenderer::RenderSpotLight(Light* light)
    {
        if (!light)
            return;

        Transform* transform = light->GetTransform();
        if (!transform)
            return;

        Vector3 worldPos = transform->GetWorldPosition();
        Matrix world = transform->GetWorld();
        
        // 방향 추출 (Transform의 forward 방향)
        Vector3 forward = Vector3::TransformNormal(Vector3::UnitZ, world);
        forward.Normalize();

        float range = light->GetRange();
        float angle = light->GetAngle() * (DirectX::XM_PI / 180.0f); // 도를 라디안으로 변환

        DirectX::XMVECTOR color = DirectX::XMLoadFloat4(
            reinterpret_cast<const DirectX::XMFLOAT4*>(&m_spotColor));
        DirectX::XMVECTOR rangeColor = DirectX::XMLoadFloat4(
            reinterpret_cast<const DirectX::XMFLOAT4*>(&m_rangeColor));

        // 원뿔 그리기
        Vector3 tip = worldPos;
        if (m_showRange)
        {
            DrawCone(tip, forward, angle, range, rangeColor);
        }

        // 방향 표시
        if (m_showDirection)
        {
            float arrowLength = std::min(range * 0.3f, 2.0f);
            DrawArrow(tip, forward, arrowLength, color);
        }

        // 중심점 표시
        DrawSphere(tip, 0.2f, color);
    }

    void LightDebugRenderer::DrawLine(const Vector3& start, const Vector3& end, const DirectX::XMVECTOR& color)
    {
        DirectX::VertexPositionColor v1(
            DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&start)),
            color);
        DirectX::VertexPositionColor v2(
            DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&end)),
            color);

        m_batch->DrawLine(v1, v2);
    }

    void LightDebugRenderer::DrawSphere(const Vector3& center, float radius, const DirectX::XMVECTOR& color)
    {
        // 3개의 원으로 구 표현 (XY, YZ, XZ 평면)
        DrawCircle(center, radius, Vector3::UnitZ, color);  // XY 평면
        DrawCircle(center, radius, Vector3::UnitX, color);  // YZ 평면
        DrawCircle(center, radius, Vector3::UnitY, color);  // XZ 평면
    }

    void LightDebugRenderer::DrawCircle(const Vector3& center, float radius, const Vector3& normal, 
                                       const DirectX::XMVECTOR& color, int segments)
    {
        // normal에 수직인 두 벡터 계산
        Vector3 tangent;
        if (fabsf(normal.y) < 0.99f)
        {
            tangent = Vector3::UnitY.Cross(normal);
        }
        else
        {
            tangent = Vector3::UnitX.Cross(normal);
        }
        tangent.Normalize();

        Vector3 bitangent = normal.Cross(tangent);
        bitangent.Normalize();

        const float angleStep = DirectX::XM_2PI / segments;

        for (int i = 0; i < segments; ++i)
        {
            float angle1 = i * angleStep;
            float angle2 = (i + 1) * angleStep;

            Vector3 p1 = center + tangent * (radius * cosf(angle1)) + bitangent * (radius * sinf(angle1));
            Vector3 p2 = center + tangent * (radius * cosf(angle2)) + bitangent * (radius * sinf(angle2));

            DrawLine(p1, p2, color);
        }
    }

    void LightDebugRenderer::DrawCone(const Vector3& tip, const Vector3& direction, float angle, float range,
                                     const DirectX::XMVECTOR& color)
    {
        // 원뿔의 밑면 반지름 계산
        float baseRadius = range * tanf(angle * 0.5f);
        Vector3 baseCenter = tip + direction * range;

        // normal에 수직인 두 벡터 계산 (밑면 평면)
        Vector3 tangent;
        if (fabsf(direction.y) < 0.99f)
        {
            tangent = Vector3::UnitY.Cross(direction);
        }
        else
        {
            tangent = Vector3::UnitX.Cross(direction);
        }
        tangent.Normalize();

        Vector3 bitangent = direction.Cross(tangent);
        bitangent.Normalize();

        // 밑면 원 그리기
        int segments = 32;
        const float angleStep = DirectX::XM_2PI / segments;

        for (int i = 0; i < segments; ++i)
        {
            float angle1 = i * angleStep;
            float angle2 = (i + 1) * angleStep;

            Vector3 p1 = baseCenter + tangent * (baseRadius * cosf(angle1)) + bitangent * (baseRadius * sinf(angle1));
            Vector3 p2 = baseCenter + tangent * (baseRadius * cosf(angle2)) + bitangent * (baseRadius * sinf(angle2));

            DrawLine(p1, p2, color);
        }

        // 원뿔의 모서리 선 그리기 (tip에서 밑면 원의 일부 점들로)
        int edgeCount = 8; // 원뿔 모서리 개수
        for (int i = 0; i < edgeCount; ++i)
        {
            float angle = i * (DirectX::XM_2PI / edgeCount);
            Vector3 edgePoint = baseCenter + tangent * (baseRadius * cosf(angle)) + bitangent * (baseRadius * sinf(angle));
            DrawLine(tip, edgePoint, color);
        }
    }

    void LightDebugRenderer::DrawArrow(const Vector3& start, const Vector3& direction, float length,
                                      const DirectX::XMVECTOR& color)
    {
        Vector3 end = start + direction * length;
        
        // 화살표 몸통
        DrawLine(start, end, color);

        // 화살표 머리 (작은 원뿔)
        float headLength = length * 0.2f;
        float headRadius = headLength * 0.3f;
        Vector3 headBase = end - direction * headLength;

        // normal에 수직인 두 벡터 계산
        Vector3 tangent;
        if (fabsf(direction.y) < 0.99f)
        {
            tangent = Vector3::UnitY.Cross(direction);
        }
        else
        {
            tangent = Vector3::UnitX.Cross(direction);
        }
        tangent.Normalize();

        Vector3 bitangent = direction.Cross(tangent);
        bitangent.Normalize();

        // 화살표 머리 그리기 (3개의 선)
        int arrowSegments = 3;
        for (int i = 0; i < arrowSegments; ++i)
        {
            float angle = i * (DirectX::XM_2PI / arrowSegments);
            Vector3 arrowPoint = headBase + tangent * (headRadius * cosf(angle)) + bitangent * (headRadius * sinf(angle));
            DrawLine(end, arrowPoint, color);
        }
    }

    void LightDebugRenderer::OnGui()
    {
        if (!ImGui::CollapsingHeader("Light Debug"))
            return;

        ImGui::Checkbox("Enabled", &m_enabled);
        
        if (m_enabled)
        {
            ImGui::Indent();
            ImGui::Checkbox("Show Directional", &m_showDirectional);
            ImGui::Checkbox("Show Point", &m_showPoint);
            ImGui::Checkbox("Show Spot", &m_showSpot);
            ImGui::Checkbox("Show Range", &m_showRange);
            ImGui::Checkbox("Show Direction", &m_showDirection);
            
            ImGui::Separator();
            ImGui::ColorEdit4("Directional Color", &m_directionalColor.x);
            ImGui::ColorEdit4("Point Color", &m_pointColor.x);
            ImGui::ColorEdit4("Spot Color", &m_spotColor.x);
            ImGui::ColorEdit4("Range Color", &m_rangeColor.x);
            ImGui::Unindent();
        }
    }
}
