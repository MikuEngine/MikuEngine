#include "EnginePCH.h"
#include "DebugRenderer.h"

#include "Core/Graphics/Device/GraphicsDevice.h"
#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Resource/DepthStencilState.h"

namespace engine
{
    void DebugRenderer::Initialize()
    {
        if (m_isInitialized) return;

        auto device = GraphicsDevice::Get().GetDevice().Get();
        auto context = GraphicsDevice::Get().GetDeviceContext().Get();

        // m_states = std::make_unique<DirectX::CommonStates>(device);
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

    void DebugRenderer::Shutdown()
    {
        m_batch.reset();
        m_effect.reset();
        m_inputLayout.Reset();
        // m_states.reset();
        m_isInitialized = false;
    }

    void DebugRenderer::Begin(const Matrix& view, const Matrix& projection)
    {
        if (!m_isInitialized) return;

        auto context = GraphicsDevice::Get().GetDeviceContext().Get();

        auto depthNone = ResourceManager::Get().GetDefaultDepthStencilState(DefaultDepthStencilType::None);

        context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
        context->OMSetDepthStencilState(depthNone->GetRawDepthStencilState(), 0);
        context->RSSetState(nullptr);

        m_effect->SetView(view);
        m_effect->SetProjection(projection);
        m_effect->SetWorld(Matrix::Identity);
        m_effect->Apply(context);

        context->IASetInputLayout(m_inputLayout.Get());

        m_batch->Begin();
    }

    void DebugRenderer::End()
    {
        if (!m_isInitialized) return;
        m_batch->End();
    }

    void DebugRenderer::DrawLine(const Vector3& start, const Vector3& end, const DirectX::XMVECTOR& color)
    {
        DirectX::VertexPositionColor v1(DirectX::XMLoadFloat3(&start), color);
        DirectX::VertexPositionColor v2(DirectX::XMLoadFloat3(&end), color);
        m_batch->DrawLine(v1, v2);
    }

    void DebugRenderer::DrawRing(const Vector3& center, float radius, const Vector3& normal, DirectX::FXMVECTOR color)
    {
        Vector3 axisY = normal;
        axisY.Normalize();

        Vector3 arbitrary = (abs(axisY.y) > 0.99f) ? Vector3(1.0f, 0.0f, 0.0f) : Vector3(0.0f, 1.0f, 0.0f);

        Vector3 axisX = axisY.Cross(arbitrary);
        axisX.Normalize();

        Vector3 axisZ = axisX.Cross(axisY);

        const int stepCount = 32;
        float stepAngle = DirectX::XM_2PI / stepCount;

        Vector3 prevPos = center + axisX * radius;

        for (int i = 1; i <= stepCount; ++i)
        {
            float angle = i * stepAngle;

            Vector3 nextPos = center + (axisX * cos(angle) + axisZ * sin(angle)) * radius;

            DrawLine(prevPos, nextPos, color);

            prevPos = nextPos;
        }
    }

    void DebugRenderer::DrawBox(const Vector3& center, const Vector3& halfExtents, const Quaternion& rotation, const DirectX::XMVECTOR& color)
    {
        Vector3 corners[8] = {
            Vector3(-halfExtents.x, -halfExtents.y, -halfExtents.z),
            Vector3(halfExtents.x, -halfExtents.y, -halfExtents.z),
            Vector3(halfExtents.x, -halfExtents.y,  halfExtents.z),
            Vector3(-halfExtents.x, -halfExtents.y,  halfExtents.z),

            Vector3(-halfExtents.x,  halfExtents.y, -halfExtents.z),
            Vector3(halfExtents.x,  halfExtents.y, -halfExtents.z),
            Vector3(halfExtents.x,  halfExtents.y,  halfExtents.z),
            Vector3(-halfExtents.x,  halfExtents.y,  halfExtents.z)
        };

        // 회전 및 이동 적용 (Local -> World)
        for (int i = 0; i < 8; ++i)
        {
            corners[i] = Vector3::Transform(corners[i], rotation) + center;
        }

        // 바닥면 (0-1-2-3)
        DrawLine(corners[0], corners[1], color);
        DrawLine(corners[1], corners[2], color);
        DrawLine(corners[2], corners[3], color);
        DrawLine(corners[3], corners[0], color);

        // 윗면 (4-5-6-7)
        DrawLine(corners[4], corners[5], color);
        DrawLine(corners[5], corners[6], color);
        DrawLine(corners[6], corners[7], color);
        DrawLine(corners[7], corners[4], color);

        // 기둥 (0-4, 1-5, 2-6, 3-7)
        DrawLine(corners[0], corners[4], color);
        DrawLine(corners[1], corners[5], color);
        DrawLine(corners[2], corners[6], color);
        DrawLine(corners[3], corners[7], color);
    }

    void DebugRenderer::DrawSphere(const Vector3& center, float radius, const DirectX::XMVECTOR& color)
    {
        // X축 기준 원 (YZ 평면)
        DrawRing(center, radius, Vector3(1.0f, 0.0f, 0.0f), color);

        // Y축 기준 원 (XZ 평면 - 바닥)
        DrawRing(center, radius, Vector3(0.0f, 1.0f, 0.0f), color);

        // Z축 기준 원 (XY 평면)
        DrawRing(center, radius, Vector3(0.0f, 0.0f, 1.0f), color);
    }

    void DebugRenderer::DrawCapsule(const Vector3& center, float radius, float height, const Quaternion& rotation, const DirectX::XMVECTOR& color)
    {
        float halfHeight = (height - 2.0f * radius) * 0.5f;
        if (halfHeight < 0) halfHeight = 0;

        Vector3 up = Vector3::Transform(Vector3::UnitY, rotation);
        Vector3 right = Vector3::Transform(Vector3::UnitX, rotation);
        Vector3 forward = Vector3::Transform(Vector3::UnitZ, rotation);

        Vector3 topCenter = center + up * halfHeight;
        Vector3 bottomCenter = center - up * halfHeight;

        DrawLine(topCenter + right * radius, bottomCenter + right * radius, color);
        DrawLine(topCenter - right * radius, bottomCenter - right * radius, color);
        DrawLine(topCenter + forward * radius, bottomCenter + forward * radius, color);
        DrawLine(topCenter - forward * radius, bottomCenter - forward * radius, color);

        DrawCircle(topCenter, radius, up, color);
        DrawCircle(bottomCenter, radius, up, color);

        const int halfSegments = 8;
        const float angleStep = DirectX::XM_PI / halfSegments;

        for (int i = 0; i < halfSegments; ++i)
        {
            float angle1 = i * angleStep;
            float angle2 = (i + 1) * angleStep;

            Vector3 p1 = topCenter + right * (radius * cosf(angle1)) + up * (radius * sinf(angle1));
            Vector3 p2 = topCenter + right * (radius * cosf(angle2)) + up * (radius * sinf(angle2));
            DrawLine(p1, p2, color);

            p1 = topCenter + forward * (radius * cosf(angle1)) + up * (radius * sinf(angle1));
            p2 = topCenter + forward * (radius * cosf(angle2)) + up * (radius * sinf(angle2));
            DrawLine(p1, p2, color);
        }

        for (int i = 0; i < halfSegments; ++i)
        {
            float angle1 = i * angleStep;
            float angle2 = (i + 1) * angleStep;

            Vector3 p1 = bottomCenter + right * (radius * cosf(angle1)) - up * (radius * sinf(angle1));
            Vector3 p2 = bottomCenter + right * (radius * cosf(angle2)) - up * (radius * sinf(angle2));
            DrawLine(p1, p2, color);

            p1 = bottomCenter + forward * (radius * cosf(angle1)) - up * (radius * sinf(angle1));
            p2 = bottomCenter + forward * (radius * cosf(angle2)) - up * (radius * sinf(angle2));
            DrawLine(p1, p2, color);
        }

    }

    void DebugRenderer::DrawCircle(const Vector3& center, float radius, const Vector3& normal, const DirectX::XMVECTOR& color, int segments)
    {
        if (normal.LengthSquared() < FLT_EPSILON) return;

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

            // 원의 공식: Center + (Tangent * cos) + (BiTangent * sin) * radius
            Vector3 p1 = center + tangent * (radius * cosf(angle1)) + bitangent * (radius * sinf(angle1));
            Vector3 p2 = center + tangent * (radius * cosf(angle2)) + bitangent * (radius * sinf(angle2));

            DrawLine(p1, p2, color);
        }
    }

    void DebugRenderer::DrawCone(const Vector3& tip, const Vector3& direction, float angle, float range, const DirectX::XMVECTOR& color)
    {
        float baseRadius = range * tanf(angle * 0.5f);
        Vector3 baseCenter = tip + direction * range;

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

        int edgeCount = 8;
        for (int i = 0; i < edgeCount; ++i)
        {
            float angle = i * (DirectX::XM_2PI / edgeCount);
            Vector3 edgePoint = baseCenter + tangent * (baseRadius * cosf(angle)) + bitangent * (baseRadius * sinf(angle));
            DrawLine(tip, edgePoint, color);
        }
    }
}