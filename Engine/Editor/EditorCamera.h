#pragma once

namespace engine
{
    class EditorCamera
    {
    private:
        Vector3 m_position{ 0.0f, 0.0f, -10.0f };
        float m_yaw = 0.0f;
        float m_pitch = 0.0f;

        float m_fov = 90.0f;
        float m_aspectRatio = 16.0f / 9.0f;
        float m_near = 1.0f;
        float m_far = 10000.0f;

        Matrix m_view;
        Matrix m_projection;

        float m_moveSpeed = 50.0f;
        float m_rotationSpeed = 5.0f;

        bool m_isDirty = true;

    public:
        void Update();
        void OnGui();

        const Matrix& GetView() const;
        const Matrix& GetProjection() const;
        const Vector3& GetPosition() const;

        void SetPosition(const Vector3& position);
        void SetCameraInfo(float fov, float aspect, float nearZ, float farZ);
    };
}