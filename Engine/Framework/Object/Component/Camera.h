#pragma once

#include "Framework/Object/Component/Component.h"

namespace engine
{
    enum class ProjectionType
    {
        Perspective,
        Orthographic
    };

    class Camera :
        public Component
    {
        REGISTER_COMPONENT(Camera, Component)

    private:
        ProjectionType m_projectionType = ProjectionType::Perspective;

        Matrix m_world; // scale이 제거된 world 행렬
        Matrix m_view;
        Matrix m_projection;
        DirectX::BoundingFrustum m_frustum;

        float m_near = 1.0f;
        float m_far = 5000.0f;
        float m_fov = 50.0f;
        float m_scale = 1.0f; // Orthographic용
        float m_width = 1.0f;
        float m_height = 1.0f;

        bool m_isDirty = true;

    public:
        Camera() = default;
        ~Camera();

    public:
        void Initialize() override;
        void Update();

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;

    public:
        void SetProjectionType(ProjectionType type);

        const Matrix& GetWorld() const;
        const Matrix& GetView() const;
        const Matrix& GetProjection() const;
        const DirectX::BoundingFrustum& GetFrustum() const;
        Vector3 GetForward() const;
        Vector3 GetPosition() const;

        void SetNear(float value);
        void SetFar(float value);
        void SetFov(float degree);
        void SetScale(float scale);
        void SetWidth(float width);
        void SetHeight(float height);

        // ─────────────────────────────────────────────
        // 좌표 변환 유틸리티
        // ─────────────────────────────────────────────
        
        // 스크린 좌표를 월드 공간의 Ray로 변환
        // screenPos: 스크린 좌표 (픽셀 단위)
        // outOrigin: Ray 시작점 (카메라 위치)
        // outDirection: Ray 방향 (정규화됨)
        // 반환값: 변환 성공 여부
        bool ScreenToWorldRay(const Vector2& screenPos, Vector3& outOrigin, Vector3& outDirection) const;
        
        // 월드 좌표를 스크린 좌표로 변환
        // worldPos: 월드 좌표
        // outScreenPos: 스크린 좌표 (픽셀 단위)
        // 반환값: 화면 내에 있으면 true, 카메라 뒤에 있으면 false
        bool WorldToScreen(const Vector3& worldPos, Vector2& outScreenPos) const;
    };
}