#pragma once

#include "Common/Utility/Singleton.h"

#include <directxtk/PrimitiveBatch.h>
#include <directxtk/VertexTypes.h>
#include <directxtk/Effects.h>
#include <directxtk/CommonStates.h>

namespace engine
{
    class Light;

    // ═══════════════════════════════════════════════════════════════
    // LightDebugRenderer - 조명 디버그 시각화
    // 
    // DirectX ToolKit의 PrimitiveBatch를 활용한 와이어프레임 렌더링
    // PathfindingDebugRenderer, PhysicsDebugRenderer와 동일한 구조
    // 
    // 기본값: OFF (m_enabled = false)
    // 활성화: LightDebugRenderer::Get().SetEnabled(true)
    // ═══════════════════════════════════════════════════════════════

    class LightDebugRenderer : public Singleton<LightDebugRenderer>
    {
    private:
        bool m_enabled = false;      // 디버그 렌더 활성화
        bool m_showDirectional = true;
        bool m_showPoint = true;
        bool m_showSpot = true;
        bool m_showRange = true;     // 범위 표시 (Point, Spot)
        bool m_showDirection = true; // 방향 표시 (Directional, Spot)

        // DirectX ToolKit 리소스
        std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_batch;
        std::unique_ptr<DirectX::BasicEffect> m_effect;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
        std::unique_ptr<DirectX::CommonStates> m_states;

        // 색상
        Vector4 m_directionalColor{ 1.0f, 1.0f, 0.0f, 1.0f };    // 노랑 - Directional Light
        Vector4 m_pointColor{ 0.0f, 1.0f, 1.0f, 1.0f };         // 시안 - Point Light
        Vector4 m_spotColor{ 1.0f, 0.5f, 0.0f, 1.0f };          // 주황 - Spot Light
        Vector4 m_rangeColor{ 0.5f, 0.5f, 0.5f, 0.5f };         // 회색 반투명 - 범위

        bool m_isInitialized = false;

    private:
        LightDebugRenderer() = default;
        ~LightDebugRenderer() = default;

    public:
        // 초기화/정리
        void Initialize();
        void Shutdown();

        // 활성화/비활성화
        void SetEnabled(bool enabled) { m_enabled = enabled; }
        bool IsEnabled() const { return m_enabled; }

        void SetShowDirectional(bool show) { m_showDirectional = show; }
        void SetShowPoint(bool show) { m_showPoint = show; }
        void SetShowSpot(bool show) { m_showSpot = show; }
        void SetShowRange(bool show) { m_showRange = show; }
        void SetShowDirection(bool show) { m_showDirection = show; }

        // 색상 설정
        void SetDirectionalColor(const Vector4& color) { m_directionalColor = color; }
        void SetPointColor(const Vector4& color) { m_pointColor = color; }
        void SetSpotColor(const Vector4& color) { m_spotColor = color; }
        void SetRangeColor(const Vector4& color) { m_rangeColor = color; }

        // 렌더링
        void Render(const Matrix& view, const Matrix& projection);

        // GUI
        void OnGui();

        // 에디터 상태 저장용
        void SaveEditorData(json& j) const;
        void LoadEditorData(const json& j);

    private:
        void RenderLights();

        // Light 타입별 렌더링
        void RenderDirectionalLight(Light* light);
        void RenderPointLight(Light* light);
        void RenderSpotLight(Light* light);

        // 헬퍼 함수
        void DrawLine(const Vector3& start, const Vector3& end, const DirectX::XMVECTOR& color);
        void DrawSphere(const Vector3& center, float radius, const DirectX::XMVECTOR& color);
        void DrawCircle(const Vector3& center, float radius, const Vector3& normal, 
                       const DirectX::XMVECTOR& color, int segments = 32);
        void DrawCone(const Vector3& tip, const Vector3& direction, float angle, float range,
                     const DirectX::XMVECTOR& color);
        void DrawArrow(const Vector3& start, const Vector3& direction, float length,
                      const DirectX::XMVECTOR& color);

        friend class Singleton<LightDebugRenderer>;
    };
}
