#pragma once

#include <directxtk/PrimitiveBatch.h>
#include <directxtk/VertexTypes.h>
#include <directxtk/Effects.h>
// #include <directxtk/CommonStates.h>

namespace engine
{
    class DebugRenderer :
        public Singleton<DebugRenderer>
    {
    private:
        bool m_isInitialized = false;

        // std::unique_ptr<DirectX::CommonStates> m_states;
        std::unique_ptr<DirectX::BasicEffect> m_effect;
        std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_batch;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

    public:
        void Initialize();
        void Shutdown();

        void Begin(const Matrix& view, const Matrix& projection);
        void End();

        void DrawLine(const Vector3& start, const Vector3& end, const DirectX::XMVECTOR& color);
        //void DrawRay(const Vector3& origin, const Vector3& direction, float length, const DirectX::XMVECTOR& color);
        void DrawRing(const Vector3& center, float radius, const Vector3& normal, DirectX::FXMVECTOR color);

        void DrawBox(const Vector3& center, const Vector3& halfExtents, const Quaternion& rotation, const DirectX::XMVECTOR& color);
        void DrawSphere(const Vector3& center, float radius, const DirectX::XMVECTOR& color);
        void DrawCapsule(const Vector3& center, float radius, float height, const Quaternion& rotation, const DirectX::XMVECTOR& color);

        void DrawCircle(const Vector3& center, float radius, const Vector3& normal, const DirectX::XMVECTOR& color, int segments = 24);
        void DrawCone(const Vector3& tip, const Vector3& direction, float angle, float range, const DirectX::XMVECTOR& color);
        //void DrawArrow(const Vector3& start, const Vector3& direction, float length, const DirectX::XMVECTOR& color);
        //void DrawPoint(const Vector3& position, float size, const DirectX::XMVECTOR& color);

    private:
        friend class Singleton<DebugRenderer>;
    };
}