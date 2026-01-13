#pragma once

#include "Common/Utility/Singleton.h"

#include <directxtk/PrimitiveBatch.h>
#include <directxtk/VertexTypes.h>
#include <directxtk/Effects.h>
#include <directxtk/CommonStates.h>

#include <vector>
#include <unordered_set>

namespace engine
{
    class Collider;
    class BoxCollider;
    class SphereCollider;
    class CapsuleCollider;
    class CharacterController;

    // ═══════════════════════════════════════════════════════════════
    // PhysicsDebugRenderer - 물리 충돌체 디버그 시각화
    // 
    // DirectX ToolKit의 PrimitiveBatch를 활용한 와이어프레임 렌더링
    // 
    // 기본값: OFF (m_enabled = false)
    // 활성화: PhysicsDebugRenderer::Get().SetEnabled(true)
    // ═══════════════════════════════════════════════════════════════

    class PhysicsDebugRenderer : public Singleton<PhysicsDebugRenderer>
    {
    private:
        bool m_enabled = false;      // 디버그 렌더 활성화
        bool m_showPivot = false;    // 피봇 표시

        // DirectX ToolKit 리소스
        std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_batch;
        std::unique_ptr<DirectX::BasicEffect> m_effect;
        Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
        std::unique_ptr<DirectX::CommonStates> m_states;

        // 색상
        Vector4 m_colliderColor{ 0.0f, 1.0f, 0.0f, 1.0f };      // 초록 - 기본
        Vector4 m_triggerColor{ 0.0f, 0.5f, 1.0f, 1.0f };       // 파랑 - 트리거
        Vector4 m_controllerColor{ 1.0f, 1.0f, 0.0f, 1.0f };    // 노랑 - 캐릭터 컨트롤러
        Vector4 m_collidingColor{ 1.0f, 0.3f, 0.0f, 1.0f };     // 주황 - 충돌 중
        Vector4 m_pivotColor{ 1.0f, 0.0f, 1.0f, 1.0f };         // 마젠타 - 피봇

        // 충돌 중인 콜라이더 추적 (이번 프레임)
        std::unordered_set<Collider*> m_collidingColliders;

        bool m_isInitialized = false;

    private:
        PhysicsDebugRenderer() = default;
        ~PhysicsDebugRenderer() = default;

    public:
        // 초기화/정리
        void Initialize();
        void Shutdown();

        // 활성화/비활성화
        void SetEnabled(bool enabled) { m_enabled = enabled; }
        bool IsEnabled() const { return m_enabled; }

        void SetShowPivot(bool show) { m_showPivot = show; }
        bool IsShowPivot() const { return m_showPivot; }

        // 색상 설정
        void SetColliderColor(const Vector4& color) { m_colliderColor = color; }
        void SetTriggerColor(const Vector4& color) { m_triggerColor = color; }
        void SetControllerColor(const Vector4& color) { m_controllerColor = color; }

        // 충돌 상태 표시 (CollisionSystem에서 호출)
        void MarkColliding(Collider* collider);
        void ClearCollidingState();

        // 렌더링
        void Render(const Matrix& view, const Matrix& projection);

        // GUI
        void OnGui();

    private:
        void RenderColliders();
        void RenderControllers();

        void DrawBox(const Vector3& center, const Vector3& halfExtents, 
                     const Quaternion& rotation, const DirectX::XMVECTOR& color);
        void DrawSphere(const Vector3& center, float radius, const DirectX::XMVECTOR& color);
        void DrawCapsule(const Vector3& center, float radius, float height, 
                        const Quaternion& rotation, const DirectX::XMVECTOR& color);
        void DrawPoint(const Vector3& position, float size, const DirectX::XMVECTOR& color);

        // 헬퍼
        void DrawLine(const Vector3& start, const Vector3& end, const DirectX::XMVECTOR& color);
        void DrawCircle(const Vector3& center, float radius, const Vector3& normal, 
                       const DirectX::XMVECTOR& color, int segments = 32);

        friend class Singleton<PhysicsDebugRenderer>;
    };
}
