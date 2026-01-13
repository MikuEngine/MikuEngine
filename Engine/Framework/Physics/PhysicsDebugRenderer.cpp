#include "EnginePCH.h"
#include "PhysicsDebugRenderer.h"

#include "Framework/Physics/PhysicsSystem.h"
#include "Framework/Object/Component/Collider.h"
#include "Framework/Object/Component/BoxCollider.h"
#include "Framework/Object/Component/SphereCollider.h"
#include "Framework/Object/Component/CapsuleCollider.h"
#include "Framework/Object/Component/CharacterController.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"
#include "Core/Graphics/Device/GraphicsDevice.h"

namespace engine
{
    void PhysicsDebugRenderer::Initialize()
    {
        if (m_isInitialized)
        {
            return;
        }

        auto device = GraphicsDevice::Get().GetDevice().Get();
        auto context = GraphicsDevice::Get().GetDeviceContext().Get();

        // CommonStates
        m_states = std::make_unique<DirectX::CommonStates>(device);

        // BasicEffect 생성
        m_effect = std::make_unique<DirectX::BasicEffect>(device);
        m_effect->SetVertexColorEnabled(true);
        m_effect->SetLightingEnabled(false);  // 조명 비활성화 (와이어프레임용)

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
            LOG_ERROR("[PhysicsDebugRenderer] Failed to create input layout");
            return;
        }

        // PrimitiveBatch 생성
        m_batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(context);

        m_isInitialized = true;
        LOG_INFO("[PhysicsDebugRenderer] Initialized");
    }

    void PhysicsDebugRenderer::Shutdown()
    {
        m_batch.reset();
        m_effect.reset();
        m_inputLayout.Reset();
        m_states.reset();
        m_isInitialized = false;
    }

    void PhysicsDebugRenderer::MarkColliding(Collider* collider)
    {
        if (collider)
        {
            m_collidingColliders.insert(collider);
        }
    }

    void PhysicsDebugRenderer::ClearCollidingState()
    {
        m_collidingColliders.clear();
    }

    void PhysicsDebugRenderer::Render(const Matrix& view, const Matrix& projection)
    {
        if (!m_enabled || !m_isInitialized)
        {
            return;
        }

        auto context = GraphicsDevice::Get().GetDeviceContext().Get();

        // 이전 상태 저장
        Microsoft::WRL::ComPtr<ID3D11BlendState> prevBlendState;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> prevDepthState;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> prevRasterState;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> prevInputLayout;
        Microsoft::WRL::ComPtr<ID3D11VertexShader> prevVS;
        Microsoft::WRL::ComPtr<ID3D11PixelShader> prevPS;
        D3D11_PRIMITIVE_TOPOLOGY prevTopology;
        FLOAT prevBlendFactor[4];
        UINT prevSampleMask;
        UINT prevStencilRef;

        context->OMGetBlendState(prevBlendState.GetAddressOf(), prevBlendFactor, &prevSampleMask);
        context->OMGetDepthStencilState(prevDepthState.GetAddressOf(), &prevStencilRef);
        context->RSGetState(prevRasterState.GetAddressOf());
        context->IAGetInputLayout(prevInputLayout.GetAddressOf());
        context->VSGetShader(prevVS.GetAddressOf(), nullptr, nullptr);
        context->PSGetShader(prevPS.GetAddressOf(), nullptr, nullptr);
        context->IAGetPrimitiveTopology(&prevTopology);

        // Effect 설정
        m_effect->SetView(view);
        m_effect->SetProjection(projection);
        m_effect->SetWorld(Matrix::Identity);

        // 상태 설정
        context->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
        context->OMSetDepthStencilState(m_states->DepthDefault(), 0);
        context->RSSetState(m_states->CullNone());

        // Effect 적용 및 InputLayout 설정
        m_effect->Apply(context);
        context->IASetInputLayout(m_inputLayout.Get());

        // 렌더링 시작
        m_batch->Begin();
        {
            RenderColliders();
            RenderControllers();
        }
        m_batch->End();

        // 이전 상태 복원
        context->OMSetBlendState(prevBlendState.Get(), prevBlendFactor, prevSampleMask);
        context->OMSetDepthStencilState(prevDepthState.Get(), prevStencilRef);
        context->RSSetState(prevRasterState.Get());
        context->IASetInputLayout(prevInputLayout.Get());
        context->VSSetShader(prevVS.Get(), nullptr, 0);
        context->PSSetShader(prevPS.Get(), nullptr, 0);
        context->IASetPrimitiveTopology(prevTopology);
    }

    void PhysicsDebugRenderer::OnGui()
    {
        if (ImGui::CollapsingHeader("Physics Debug"))
        {
            ImGui::Checkbox("Show Colliders", &m_enabled);
            
            if (m_enabled)
            {
                ImGui::Indent();
                ImGui::Checkbox("Show Pivot", &m_showPivot);
                
                ImGui::ColorEdit4("Collider Color", &m_colliderColor.x);
                ImGui::ColorEdit4("Trigger Color", &m_triggerColor.x);
                ImGui::ColorEdit4("Colliding Color", &m_collidingColor.x);
                ImGui::ColorEdit4("Controller Color", &m_controllerColor.x);
                ImGui::Unindent();
            }
        }
    }

    void PhysicsDebugRenderer::RenderColliders()
    {
        // 에디터 모드에서도 작동하도록 씬에서 직접 콜라이더를 찾음
        std::vector<Collider*> colliders;
        
        // PhysicsSystem에 등록된 콜라이더 (Play 모드)
        const auto& registeredColliders = PhysicsSystem::Get().GetRegisteredColliders();
        if (!registeredColliders.empty())
        {
            colliders = registeredColliders;
        }
        else
        {
            // 씬에서 직접 찾기 (Edit 모드)
            Scene* scene = SceneManager::Get().GetScene();
            if (scene)
            {
                for (const auto& go : scene->GetGameObjects())
                {
                    if (!go) continue;
                    if (Collider* col = go->GetComponent<Collider>())
                    {
                        colliders.push_back(col);
                    }
                }
            }
        }

        for (Collider* collider : colliders)
        {
            if (!collider || !collider->IsActive())
            {
                continue;
            }

            // 색상 결정
            DirectX::XMVECTOR color;
            if (m_collidingColliders.find(collider) != m_collidingColliders.end())
            {
                color = DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(&m_collidingColor));
            }
            else if (collider->IsTrigger())
            {
                color = DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(&m_triggerColor));
            }
            else
            {
                color = DirectX::XMLoadFloat4(reinterpret_cast<const DirectX::XMFLOAT4*>(&m_colliderColor));
            }

            Transform* transform = collider->GetTransform();
            if (!transform)
            {
                continue;
            }

            Vector3 worldPos = transform->GetWorldPosition();
            Vector3 center = collider->GetCenter();
            Vector3 localRotation = collider->GetRotation();

            // 로컬 회전 (오일러 각도 -> 쿼터니언)
            Vector3 radians = localRotation * (DirectX::XM_PI / 180.0f);
            Quaternion localRot = Quaternion::CreateFromYawPitchRoll(radians.y, radians.x, radians.z);

            // 타입별 렌더링
            if (BoxCollider* box = dynamic_cast<BoxCollider*>(collider))
            {
                // BoxCollider: 월드 회전 적용
                Matrix world = transform->GetWorld();
                Vector3 scale;
                Quaternion worldRot;
                Vector3 translation;
                world.Decompose(scale, worldRot, translation);

                Vector3 rotatedCenter = Vector3::Transform(center, worldRot);
                Vector3 finalPos = worldPos + rotatedCenter;
                Quaternion finalRot = worldRot * localRot;

                DrawBox(finalPos, box->GetHalfExtents(), finalRot, color);
            }
            else if (SphereCollider* sphere = dynamic_cast<SphereCollider*>(collider))
            {
                // SphereCollider: 월드 회전 무시, 로컬 오프셋만 적용
                Vector3 finalPos = worldPos + center;
                DrawSphere(finalPos, sphere->GetRadius(), color);
            }
            else if (CapsuleCollider* capsule = dynamic_cast<CapsuleCollider*>(collider))
            {
                // CapsuleCollider: 월드 회전 무시, 로컬 회전만 적용
                Vector3 finalPos = worldPos + center;
                DrawCapsule(finalPos, capsule->GetRadius(), capsule->GetHeight(), localRot, color);
            }

            // 피봇 표시
            if (m_showPivot)
            {
                DirectX::XMVECTOR pivotColor = DirectX::XMLoadFloat4(
                    reinterpret_cast<const DirectX::XMFLOAT4*>(&m_pivotColor));
                DrawPoint(worldPos, 0.1f, pivotColor);
            }
        }
    }

    void PhysicsDebugRenderer::RenderControllers()
    {
        // 에디터 모드에서도 작동하도록 씬에서 직접 컨트롤러를 찾음
        std::vector<CharacterController*> controllers;
        
        const auto& registeredControllers = PhysicsSystem::Get().GetRegisteredControllers();
        if (!registeredControllers.empty())
        {
            controllers = registeredControllers;
        }
        else
        {
            Scene* scene = SceneManager::Get().GetScene();
            if (scene)
            {
                for (const auto& go : scene->GetGameObjects())
                {
                    if (!go) continue;
                    if (CharacterController* ctrl = go->GetComponent<CharacterController>())
                    {
                        controllers.push_back(ctrl);
                    }
                }
            }
        }

        DirectX::XMVECTOR color = DirectX::XMLoadFloat4(
            reinterpret_cast<const DirectX::XMFLOAT4*>(&m_controllerColor));

        for (CharacterController* controller : controllers)
        {
            if (!controller || !controller->IsActive())
            {
                continue;
            }

            Vector3 pos = controller->GetPosition();
            float radius = controller->GetRadius();
            float height = controller->GetHeight();

            // 캡슐 그리기 (캐릭터 컨트롤러는 항상 Y축 정렬)
            DrawCapsule(pos + Vector3(0, height * 0.5f, 0), radius, height, 
                       Quaternion::Identity, color);

            // 피봇 표시
            if (m_showPivot)
            {
                DirectX::XMVECTOR pivotColor = DirectX::XMLoadFloat4(
                    reinterpret_cast<const DirectX::XMFLOAT4*>(&m_pivotColor));
                DrawPoint(pos, 0.1f, pivotColor);
            }
        }
    }

    void PhysicsDebugRenderer::DrawBox(
        const Vector3& center, 
        const Vector3& halfExtents, 
        const Quaternion& rotation, 
        const DirectX::XMVECTOR& color)
    {
        // 박스의 8개 꼭짓점 (로컬 좌표)
        Vector3 corners[8] = {
            Vector3(-halfExtents.x, -halfExtents.y, -halfExtents.z),
            Vector3( halfExtents.x, -halfExtents.y, -halfExtents.z),
            Vector3( halfExtents.x, -halfExtents.y,  halfExtents.z),
            Vector3(-halfExtents.x, -halfExtents.y,  halfExtents.z),
            Vector3(-halfExtents.x,  halfExtents.y, -halfExtents.z),
            Vector3( halfExtents.x,  halfExtents.y, -halfExtents.z),
            Vector3( halfExtents.x,  halfExtents.y,  halfExtents.z),
            Vector3(-halfExtents.x,  halfExtents.y,  halfExtents.z),
        };

        // 회전 및 이동 적용
        for (int i = 0; i < 8; ++i)
        {
            corners[i] = Vector3::Transform(corners[i], rotation) + center;
        }

        // 12개의 엣지 그리기
        // 바닥면
        DrawLine(corners[0], corners[1], color);
        DrawLine(corners[1], corners[2], color);
        DrawLine(corners[2], corners[3], color);
        DrawLine(corners[3], corners[0], color);

        // 윗면
        DrawLine(corners[4], corners[5], color);
        DrawLine(corners[5], corners[6], color);
        DrawLine(corners[6], corners[7], color);
        DrawLine(corners[7], corners[4], color);

        // 수직 엣지
        DrawLine(corners[0], corners[4], color);
        DrawLine(corners[1], corners[5], color);
        DrawLine(corners[2], corners[6], color);
        DrawLine(corners[3], corners[7], color);
    }

    void PhysicsDebugRenderer::DrawSphere(
        const Vector3& center, 
        float radius, 
        const DirectX::XMVECTOR& color)
    {
        // 3개의 원으로 구 표현 (XY, YZ, XZ 평면)
        DrawCircle(center, radius, Vector3::UnitZ, color);  // XY 평면
        DrawCircle(center, radius, Vector3::UnitX, color);  // YZ 평면
        DrawCircle(center, radius, Vector3::UnitY, color);  // XZ 평면
    }

    void PhysicsDebugRenderer::DrawCapsule(
        const Vector3& center, 
        float radius, 
        float height, 
        const Quaternion& rotation, 
        const DirectX::XMVECTOR& color)
    {
        // 캡슐 = 원기둥 + 위아래 반구
        float halfHeight = (height - 2.0f * radius) * 0.5f;
        if (halfHeight < 0) halfHeight = 0;

        // 로컬 Y축 방향으로 캡슐이 정렬됨
        Vector3 up = Vector3::Transform(Vector3::UnitY, rotation);
        Vector3 right = Vector3::Transform(Vector3::UnitX, rotation);
        Vector3 forward = Vector3::Transform(Vector3::UnitZ, rotation);

        Vector3 topCenter = center + up * halfHeight;
        Vector3 bottomCenter = center - up * halfHeight;

        // 원기둥 부분 - 4개의 수직선
        DrawLine(topCenter + right * radius, bottomCenter + right * radius, color);
        DrawLine(topCenter - right * radius, bottomCenter - right * radius, color);
        DrawLine(topCenter + forward * radius, bottomCenter + forward * radius, color);
        DrawLine(topCenter - forward * radius, bottomCenter - forward * radius, color);

        // 위/아래 원
        DrawCircle(topCenter, radius, up, color);
        DrawCircle(bottomCenter, radius, up, color);

        // 반구 표현 (간략화 - 2개의 반원)
        const int halfSegments = 8;
        const float angleStep = DirectX::XM_PI / halfSegments;

        // 상단 반구 (right-up 평면)
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

        // 하단 반구
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

    void PhysicsDebugRenderer::DrawPoint(
        const Vector3& position, 
        float size, 
        const DirectX::XMVECTOR& color)
    {
        // 작은 십자가로 점 표현
        DrawLine(position - Vector3::UnitX * size, position + Vector3::UnitX * size, color);
        DrawLine(position - Vector3::UnitY * size, position + Vector3::UnitY * size, color);
        DrawLine(position - Vector3::UnitZ * size, position + Vector3::UnitZ * size, color);
    }

    void PhysicsDebugRenderer::DrawLine(
        const Vector3& start, 
        const Vector3& end, 
        const DirectX::XMVECTOR& color)
    {
        DirectX::VertexPositionColor v1(
            DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&start)), 
            color);
        DirectX::VertexPositionColor v2(
            DirectX::XMLoadFloat3(reinterpret_cast<const DirectX::XMFLOAT3*>(&end)), 
            color);

        m_batch->DrawLine(v1, v2);
    }

    void PhysicsDebugRenderer::DrawCircle(
        const Vector3& center, 
        float radius, 
        const Vector3& normal, 
        const DirectX::XMVECTOR& color, 
        int segments)
    {
        // normal에 수직인 두 벡터 계산
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

            Vector3 p1 = center + tangent * (radius * cosf(angle1)) + bitangent * (radius * sinf(angle1));
            Vector3 p2 = center + tangent * (radius * cosf(angle2)) + bitangent * (radius * sinf(angle2));

            DrawLine(p1, p2, color);
        }
    }
}
