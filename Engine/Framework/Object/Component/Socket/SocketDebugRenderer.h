#pragma once

#include "Common/Utility/Singleton.h"
#include <directxtk/PrimitiveBatch.h>
#include <directxtk/VertexTypes.h>
#include <directxtk/Effects.h>
#include <directxtk/CommonStates.h>

namespace engine
{
    class SocketDebugRenderer :
        public Singleton<SocketDebugRenderer>
    {
    private:
        bool m_enabled = false;
        bool m_showAxis = true;
        float m_axisLength = 5.0f;

        std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_batch;
        std::unique_ptr<DirectX::BasicEffect> m_effect;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
        std::unique_ptr<DirectX::CommonStates> m_states;

        bool m_isInitialized = false;

    private:
        SocketDebugRenderer() = default;
        ~SocketDebugRenderer() = default;

    public:
        void Initialize();
        void Shutdown();

        void SetEnabled(bool enabled) { m_enabled = enabled; }
        bool IsEnabled() const { return m_enabled; }

        void SetShowAxis(bool show) { m_showAxis = show; }
        void SetAxisLength(float length) { m_axisLength = length; }

        void Render(const Matrix& view, const Matrix& projection);

        void OnGui();

    private:
        void DrawLine(const Vector3& start, const Vector3& end, const DirectX::XMVECTOR& color);
        void DrawSocket(const Matrix& worldMatrix);

        friend class Singleton<SocketDebugRenderer>;
    };
}