#include "EnginePCH.h"
#include "PathfindingDebugRenderer.h"

#include "Framework/Object/Component/Pathfinding/GridMap.h"
#include "Framework/Object/Component/Pathfinding/PathfindingAgent.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/PathfindingSystem.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"
#include "Core/Graphics/Device/GraphicsDevice.h"
#include "Core/Graphics/Resource/DepthStencilState.h"

namespace engine
{
    void PathfindingDebugRenderer::Initialize()
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
            LOG_ERROR("[PathfindingDebugRenderer] Failed to create input layout");
            return;
        }

        // PrimitiveBatch 생성
        m_batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);

        m_isInitialized = true;
        LOG_INFO("[PathfindingDebugRenderer] Initialized");
    }

    void PathfindingDebugRenderer::Shutdown()
    {
        m_batch.reset();
        m_effect.reset();
        m_inputLayout.Reset();
        m_states.reset();
        m_isInitialized = false;
    }

    void PathfindingDebugRenderer::Render(const Matrix& view, const Matrix& projection)
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
        context->OMSetDepthStencilState(m_states->DepthNone(), 0);
        context->RSSetState(m_states->CullNone());

        m_effect->SetView(view);
        m_effect->SetProjection(projection);
        m_effect->Apply(context);

        m_batch->Begin();

        // 그리드 렌더링
        if (m_showGrid)
        {
            auto gridMap = SystemManager::Get().GetPathfindingSystem().GetGridMap();
            if (gridMap != nullptr)
            {
                RenderGrid(gridMap);

                if (m_showUnwalkable)
                {
                    RenderUnwalkableCells(gridMap);
                }
            }
        }

        // 경로 렌더링
        if (m_showPath)
        {
            RenderPaths();
        }

        m_batch->End();

        context->RSSetViewports(numViewports, prevViewports);
        context->OMSetBlendState(prevBlendState.Get(), prevBlendFactor, prevSampleMask);
        context->OMSetDepthStencilState(prevDepthState.Get(), prevStencilRef);
        context->RSSetState(prevRasterState.Get());
        context->IASetInputLayout(prevInputLayout.Get());
        context->VSSetShader(prevVS.Get(), nullptr, 0);
        context->PSSetShader(prevPS.Get(), nullptr, 0);
        context->IASetPrimitiveTopology(prevTopology);
    }

    void PathfindingDebugRenderer::RenderGrid(GridMap* gridMap)
    {
        if (!gridMap)
            return;

        DirectX::XMVECTOR color = DirectX::XMLoadFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(&m_gridLineColor));
        float cellSize = gridMap->GetCellSize();
        int width = gridMap->GetWidth();
        int height = gridMap->GetHeight();
        Vector3 origin = gridMap->GetOrigin();

        float o = -gridMap->GetCellSize() / 2;
        Vector3 offset{ o, 0.0f, o };

        // 수평선 (Z 방향)
        for (int z = 0; z <= height; ++z)
        {
            Vector3 start = gridMap->GridToWorld(0, z) + offset;
            Vector3 end = gridMap->GridToWorld(width, z) + offset;
            DrawLine(start, end, color);
        }

        // 수직선 (X 방향)
        for (int x = 0; x <= width; ++x)
        {
            Vector3 start = gridMap->GridToWorld(x, 0) + offset;
            Vector3 end = gridMap->GridToWorld(x, height) + offset;
            DrawLine(start, end, color);
        }
    }

    void PathfindingDebugRenderer::RenderUnwalkableCells(GridMap* gridMap)
    {
        if (!gridMap)
            return;

        DirectX::XMVECTOR color = DirectX::XMLoadFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(&m_unwalkableColor));
        float cellSize = gridMap->GetCellSize();
        float halfSize = cellSize * 0.5f;

        for (int z = 0; z < gridMap->GetHeight(); ++z)
        {
            for (int x = 0; x < gridMap->GetWidth(); ++x)
            {
                if (!gridMap->IsWalkable(x, z))
                {
                    Vector3 center = gridMap->GridToWorld(x, z);
                    Vector3 halfExtents(halfSize, 0.1f, halfSize);  // Y는 작게
                    DrawBox(center, halfExtents, color);
                }
            }
        }
    }

    void PathfindingDebugRenderer::RenderPaths()
    {
        auto* scene = SceneManager::Get().GetScene();
        if (!scene)
            return;

        DirectX::XMVECTOR pathColor = DirectX::XMLoadFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(&m_pathColor));
        DirectX::XMVECTOR waypointColor = DirectX::XMLoadFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(&m_waypointColor));

        const auto& gameObjects = scene->GetGameObjects();
        for (const auto& go : gameObjects)
        {
            auto* agent = go->GetComponent<PathfindingAgent>();
            if (!agent || !agent->HasPath())
                continue;

            const auto& path = agent->GetPath();

            // 경로 라인 그리기
            for (size_t i = 1; i < path.size(); ++i)
            {
                DrawLine(path[i - 1], path[i], pathColor);
            }

            // Waypoint 표시
            for (size_t i = 0; i < path.size(); ++i)
            {
                DrawSphere(path[i], 0.2f, waypointColor);
            }

            // 현재 Waypoint 강조
            Vector3 currentWaypoint = agent->GetCurrentWaypoint();
            if (currentWaypoint != Vector3::Zero)
            {
                DrawSphere(currentWaypoint, 0.3f, waypointColor);
            }
        }
    }

    void PathfindingDebugRenderer::DrawLine(const Vector3& start, const Vector3& end, const DirectX::XMVECTOR& color)
    {
        DirectX::VertexPositionColor v1(
            DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&start)),
            color);
        DirectX::VertexPositionColor v2(
            DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&end)),
            color);

        m_batch->DrawLine(v1, v2);
    }

    void PathfindingDebugRenderer::DrawBox(const Vector3& center, const Vector3& halfExtents, const DirectX::XMVECTOR& color)
    {
        // 박스의 12개 엣지 그리기
        Vector3 min = center - halfExtents;
        Vector3 max = center + halfExtents;

        // 하단 4개 엣지
        DrawLine(Vector3(min.x, min.y, min.z), Vector3(max.x, min.y, min.z), color);
        DrawLine(Vector3(max.x, min.y, min.z), Vector3(max.x, min.y, max.z), color);
        DrawLine(Vector3(max.x, min.y, max.z), Vector3(min.x, min.y, max.z), color);
        DrawLine(Vector3(min.x, min.y, max.z), Vector3(min.x, min.y, min.z), color);

        // 상단 4개 엣지
        DrawLine(Vector3(min.x, max.y, min.z), Vector3(max.x, max.y, min.z), color);
        DrawLine(Vector3(max.x, max.y, min.z), Vector3(max.x, max.y, max.z), color);
        DrawLine(Vector3(max.x, max.y, max.z), Vector3(min.x, max.y, max.z), color);
        DrawLine(Vector3(min.x, max.y, max.z), Vector3(min.x, max.y, min.z), color);

        // 수직 4개 엣지
        DrawLine(Vector3(min.x, min.y, min.z), Vector3(min.x, max.y, min.z), color);
        DrawLine(Vector3(max.x, min.y, min.z), Vector3(max.x, max.y, min.z), color);
        DrawLine(Vector3(max.x, min.y, max.z), Vector3(max.x, max.y, max.z), color);
        DrawLine(Vector3(min.x, min.y, max.z), Vector3(min.x, max.y, max.z), color);
    }

    void PathfindingDebugRenderer::DrawSphere(const Vector3& center, float radius, const DirectX::XMVECTOR& color)
    {
        // 간단한 원으로 표시 (X-Z 평면)
        int segments = 16;
        const float angleStep = DirectX::XM_2PI / segments;

        for (int i = 0; i < segments; ++i)
        {
            float angle1 = i * angleStep;
            float angle2 = (i + 1) * angleStep;

            Vector3 p1 = center + Vector3(cosf(angle1) * radius, 0.0f, sinf(angle1) * radius);
            Vector3 p2 = center + Vector3(cosf(angle2) * radius, 0.0f, sinf(angle2) * radius);

            DrawLine(p1, p2, color);
        }
    }

    void PathfindingDebugRenderer::OnGui()
    {
        if (!ImGui::CollapsingHeader("Pathfinding Debug"))
        {
            return;
        }

        ImGui::PushID(this);

        ImGui::Checkbox("Enabled", &m_enabled);
        ImGui::Checkbox("Show Grid", &m_showGrid);
        ImGui::Checkbox("Show Path", &m_showPath);
        ImGui::Checkbox("Show Unwalkable", &m_showUnwalkable);

        ImGui::PopID();
    }

    // 에디터 상태 저장용
    void PathfindingDebugRenderer::SaveEditorData(json& j) const
    {
        j["Enable"] = m_enabled;
        j["ShowGrid"] = m_showGrid;
        j["ShowPath"] = m_showPath;
        j["ShowUnwalkable"] = m_showUnwalkable;
        j["GridLineColor"] = m_gridLineColor;
        j["PathColor"] = m_pathColor;
        j["UnwalkableColor"] = m_unwalkableColor;
        j["WaypointColor"] = m_waypointColor;
    }

    void PathfindingDebugRenderer::LoadEditorData(const json& j)
    {
        JsonGet(j, "Enable", m_enabled, false);
        JsonGet(j, "ShowGrid", m_showGrid, true);
        JsonGet(j, "ShowPath", m_showPath, true);
        JsonGet(j, "ShowUnwalkable", m_showUnwalkable, true);
        JsonGet(j, "GridLineColor", m_gridLineColor, { 0.5f, 0.5f, 0.5f, 1.0f });
        JsonGet(j, "PathColor", m_pathColor, { 0.0f, 1.0f, 0.0f, 1.0f });
        JsonGet(j, "UnwalkableColor", m_unwalkableColor, { 1.0f, 0.0f, 0.0f, 1.0f });
        JsonGet(j, "WaypointColor", m_waypointColor, { 1.0f, 1.0f, 0.0f, 1.0f });
    }
}