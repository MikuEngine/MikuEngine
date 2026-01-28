#include "EnginePCH.h"
#include "SocketDebugRenderer.h"

#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/Object/Component/Renderer/Renderer.h"
#include "Core/Graphics/Device/GraphicsDevice.h"
#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Resource/DepthStencilState.h"

namespace engine
{
    void SocketDebugRenderer::Initialize()
    {
        if (m_isInitialized)
            return;

        auto device = GraphicsDevice::Get().GetDevice().Get();
        auto context = GraphicsDevice::Get().GetDeviceContext().Get();

        m_states = std::make_unique<DirectX::CommonStates>(device);
        m_effect = std::make_unique<DirectX::BasicEffect>(device);
        m_effect->SetVertexColorEnabled(true);
        m_effect->SetLightingEnabled(false);

        void const* shaderByteCode;
        size_t byteCodeLength;

        m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

        device->CreateInputLayout(
            DirectX::VertexPositionColor::InputElements,
            DirectX::VertexPositionColor::InputElementCount,
            shaderByteCode, byteCodeLength,
            m_inputLayout.ReleaseAndGetAddressOf()
        );

        m_batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);
        m_isInitialized = true;
    }

    void SocketDebugRenderer::Shutdown()
    {
        m_batch.reset();
        m_effect.reset();
        m_inputLayout.Reset();
        m_states.reset();
        m_isInitialized = false;
    }

    void SocketDebugRenderer::Render(const Matrix& view, const Matrix& projection)
    {
        if (!m_enabled || !m_isInitialized)
            return;

        const auto& renderers = SystemManager::Get().GetRenderSystem().GetRegisteredRenderers();

        if (renderers.empty())
            return;

        auto context = GraphicsDevice::Get().GetDeviceContext().Get();

        m_effect->SetView(view);
        m_effect->SetProjection(projection);
        m_effect->SetWorld(Matrix::Identity);

        auto depthNoneState = ResourceManager::Get().GetDefaultDepthStencilState(DefaultDepthStencilType::None);
        context->OMSetDepthStencilState(depthNoneState->GetRawDepthStencilState(), 0);
        context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
        context->RSSetState(m_states->CullNone());
        m_effect->Apply(context);
        context->IASetInputLayout(m_inputLayout.Get());
        m_batch->Begin();

        for (const auto* renderer : renderers)
        {
            if (!renderer || !renderer->IsActive())
                continue;

            const auto& sockets = renderer->GetSocketInstances();

            for (const auto& socket : sockets)
            {
                DrawSocket(socket.worldMatrix);
            }
        }

        m_batch->End();
    }

    void SocketDebugRenderer::DrawSocket(const Matrix& worldMatrix)
    {
        Vector3 pos = worldMatrix.Translation();

        if (m_showAxis)
        {
            Vector3 right = worldMatrix.Right();
            Vector3 up = worldMatrix.Up();
            Vector3 forward = worldMatrix.Forward();
            right.Normalize();
            up.Normalize();
            forward.Normalize();
            DrawLine(pos, pos + right * m_axisLength, DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f));
            DrawLine(pos, pos + up * m_axisLength, DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f));
            DrawLine(pos, pos + forward * m_axisLength, DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f));
        }
        else
        {
            // Simple Point or small shape
            DrawLine(pos - Vector3(0.05f, 0, 0), pos + Vector3(0.05f, 0, 0), DirectX::XMVectorSet(1.0f, 1.0f, 0.0f, 1.0f));
            DrawLine(pos - Vector3(0, 0.05f, 0), pos + Vector3(0, 0.05f, 0), DirectX::XMVectorSet(1.0f, 1.0f, 0.0f, 1.0f));
            DrawLine(pos - Vector3(0, 0, 0.05f), pos + Vector3(0, 0, 0.05f), DirectX::XMVectorSet(1.0f, 1.0f, 0.0f, 1.0f));
        }
    }

    void SocketDebugRenderer::DrawLine(const Vector3& start, const Vector3& end, const DirectX::XMVECTOR& color)
    {
        DirectX::VertexPositionColor v1(DirectX::XMLoadFloat3(&start), color);
        DirectX::VertexPositionColor v2(DirectX::XMLoadFloat3(&end), color);
        m_batch->DrawLine(v1, v2);
    }

    void SocketDebugRenderer::OnGui()
    {
        if (ImGui::CollapsingHeader("Socket Debug"))
        {
            ImGui::Checkbox("Enabled2", &m_enabled);
            ImGui::Checkbox("Show Axis", &m_showAxis);
            ImGui::DragFloat("Axis Length", &m_axisLength, 0.01f, 0.01f, 5.0f);
        }
    }
}