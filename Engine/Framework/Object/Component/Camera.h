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
        REGISTER_COMPONENT(Camera)
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
        std::string GetType() const override;

    public:
        void SetProjectionType(ProjectionType type);

        const Matrix& GetWorld() const;
        const Matrix& GetView() const;
        const Matrix& GetProjection() const;
        const DirectX::BoundingFrustum& GetFrustum() const;

        void SetNear(float value);
        void SetFar(float value);
        void SetFov(float degree);
        void SetScale(float scale);
        void SetWidth(float width);
        void SetHeight(float height);
    };
}