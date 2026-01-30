#pragma once

#include <directxtk/PrimitiveBatch.h>
#include <directxtk/VertexTypes.h>
#include <directxtk/Effects.h>
#include <directxtk/CommonStates.h>

namespace engine
{
    class GridMap;

    // ═══════════════════════════════════════════════════════════════
    // PathfindingDebugRenderer - 길찾기 디버그 시각화
    // 
    // DirectX ToolKit의 PrimitiveBatch를 활용한 와이어프레임 렌더링
    // PhysicsDebugRenderer와 동일한 구조
    // ═══════════════════════════════════════════════════════════════

    class PathfindingDebugRenderer :
        public Singleton<PathfindingDebugRenderer>
    {
    private:
        bool m_enabled = false;
        bool m_showGrid = true;
        bool m_showPath = true;
        bool m_showUnwalkable = true;

        // DirectX ToolKit 리소스
        std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_batch;
        std::unique_ptr<DirectX::BasicEffect> m_effect;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
        std::unique_ptr<DirectX::CommonStates> m_states;

        // 색상
        Vector4 m_gridLineColor{ 0.5f, 0.5f, 0.5f, 1.0f };        // 회색 - 그리드 라인
        Vector4 m_pathColor{ 0.0f, 1.0f, 0.0f, 1.0f };            // 초록 - 경로
        Vector4 m_unwalkableColor{ 1.0f, 0.0f, 0.0f, 1.0f };     // 빨강 - Unwalkable 셀
        Vector4 m_waypointColor{ 1.0f, 1.0f, 0.0f, 1.0f };       // 노랑 - Waypoint

        bool m_isInitialized = false;

    private:
        PathfindingDebugRenderer() = default;
        ~PathfindingDebugRenderer() = default;

    public:
        // 초기화/정리
        void Initialize();
        void Shutdown();

        // 활성화/비활성화
        void SetEnabled(bool enabled) { m_enabled = enabled; }
        bool IsEnabled() const { return m_enabled; }

        void SetShowGrid(bool show) { m_showGrid = show; }
        void SetShowPath(bool show) { m_showPath = show; }
        void SetShowUnwalkable(bool show) { m_showUnwalkable = show; }

        // 색상 설정
        void SetGridLineColor(const Vector4& color) { m_gridLineColor = color; }
        void SetPathColor(const Vector4& color) { m_pathColor = color; }
        void SetUnwalkableColor(const Vector4& color) { m_unwalkableColor = color; }
        void SetWaypointColor(const Vector4& color) { m_waypointColor = color; }

        // 렌더링
        void Render(const Matrix& view, const Matrix& projection);

        // GUI
        void OnGui();

        // 에디터 상태 저장용
        void SaveEditorData(json& j) const;
        void LoadEditorData(const json& j);

    private:
        void RenderGrid(GridMap* gridMap);
        void RenderPaths();
        void RenderUnwalkableCells(GridMap* gridMap);

        // 헬퍼
        void DrawLine(const Vector3& start, const Vector3& end, const DirectX::XMVECTOR& color);
        void DrawBox(const Vector3& center, const Vector3& halfExtents, const DirectX::XMVECTOR& color);
        void DrawSphere(const Vector3& center, float radius, const DirectX::XMVECTOR& color);

        friend class Singleton<PathfindingDebugRenderer>;
    };
}