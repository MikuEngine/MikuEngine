#pragma once

#include "Framework/Object/Component/Component.h"
#include "Framework/Object/Component/ComponentFactory.h"
#include "Framework/Physics/PhysicsLayer.h"
#include <PxPhysicsAPI.h>
#include <characterkinematic/PxController.h>

namespace engine
{
    // ═══════════════════════════════════════════════════════════════
    // 충돌 플래그 (Move 결과)
    // ═══════════════════════════════════════════════════════════════

    enum class ControllerCollisionFlags : uint32_t
    {
        None = 0,
        Sides = (1 << 0),   // 측면 충돌
        Above = (1 << 1),   // 위쪽 충돌 (천장)
        Below = (1 << 2)    // 아래쪽 충돌 (바닥)
    };

    inline ControllerCollisionFlags operator|(ControllerCollisionFlags a, ControllerCollisionFlags b)
    {
        return static_cast<ControllerCollisionFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    inline bool operator&(ControllerCollisionFlags a, ControllerCollisionFlags b)
    {
        return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
    }

    // ═══════════════════════════════════════════════════════════════
    // CharacterController - 캐릭터 이동 컨트롤러
    // 
    // Rigidbody 대신 사용하는 캐릭터 전용 컨트롤러
    // - 계단/경사면 자동 처리
    // - 물리 힘과 입력 제어 공존
    // - 벽 뚫기 방지
    // ═══════════════════════════════════════════════════════════════

    class CharacterController : public Component
    {
        DEFINE_COMPONENT_TYPE(CharacterController, Component)

    private:
        physx::PxController* m_controller = nullptr;

        // 형태 (캡슐)
        float m_height = 1.8f;      // 총 높이
        float m_radius = 0.4f;      // 반지름
        float m_skinWidth = 0.08f;  // 충돌 여유

        // 이동 설정
        float m_stepOffset = 0.3f;      // 오를 수 있는 계단 높이
        float m_slopeLimit = 45.0f;     // 오를 수 있는 경사 각도 (도)
        float m_minMoveDistance = 0.001f;

        // 물리 속성 (스크립트에서 관리)
        Vector3 m_velocity{ 0.0f, 0.0f, 0.0f };

        // 상태
        bool m_isGrounded = false;
        ControllerCollisionFlags m_lastCollisionFlags = ControllerCollisionFlags::None;

        // 레이어
        uint32_t m_layer = PhysicsLayer::Player;

        // 오프셋 (로컬 위치 조정)
        Vector3 m_center{ 0.0f, 0.0f, 0.0f };

    public:
        CharacterController() = default;
        virtual ~CharacterController();

    public:
        void Initialize() override;
        void Awake() override;
        void OnDestroy() override;

        // ═══════════════════════════════════════
        // 이동
        // ═══════════════════════════════════════

        // 이동 (충돌 처리 포함)
        ControllerCollisionFlags Move(const Vector3& motion, float deltaTime);

        // 위치 직접 설정 (텔레포트)
        void SetPosition(const Vector3& position);
        Vector3 GetPosition() const;

        // 속도 관리 (외부 힘, 중력 등)
        void AddVelocity(const Vector3& velocity) { m_velocity += velocity; }
        void SetVelocity(const Vector3& velocity) { m_velocity = velocity; }
        Vector3 GetVelocity() const { return m_velocity; }

        // ═══════════════════════════════════════
        // 상태
        // ═══════════════════════════════════════

        bool IsGrounded() const { return m_isGrounded; }
        ControllerCollisionFlags GetLastCollisionFlags() const { return m_lastCollisionFlags; }

        // ═══════════════════════════════════════
        // 형태 설정
        // ═══════════════════════════════════════

        float GetHeight() const { return m_height; }
        void SetHeight(float height);

        float GetRadius() const { return m_radius; }
        void SetRadius(float radius);

        float GetSkinWidth() const { return m_skinWidth; }
        void SetSkinWidth(float width);

        float GetStepOffset() const { return m_stepOffset; }
        void SetStepOffset(float offset);

        float GetSlopeLimit() const { return m_slopeLimit; }
        void SetSlopeLimit(float limitDegrees);

        const Vector3& GetCenter() const { return m_center; }
        void SetCenter(const Vector3& center);

        // ═══════════════════════════════════════
        // 레이어
        // ═══════════════════════════════════════

        uint32_t GetLayer() const { return m_layer; }
        void SetLayer(uint32_t layer);

        // ═══════════════════════════════════════
        // PhysX 접근
        // ═══════════════════════════════════════

        physx::PxController* GetPxController() const { return m_controller; }

        // ═══════════════════════════════════════
        // 직렬화
        // ═══════════════════════════════════════

        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;

    private:
        void CreateController();
        void SyncTransformFromController();
    };
}
