#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Ptr.h>
#include <vector>

namespace engine
{
    class Camera;
    class Collider;
}

namespace game
{
    class AimModeController;

    // ═══════════════════════════════════════════════════════════════
    // AimPointerMeshScript
    // 
    // AimPointer가 가리키는 월드 좌표로 매 프레임 순간이동하는 스크립트
    // - Y 좌표는 고정값 사용
    // - 물리(Rigidbody, Collider) 없음
    // - 속도 기반 이동이 아닌 Transform 직접 설정
    // ═══════════════════════════════════════════════════════════════
    class AimPointerMeshScript :
        public engine::Script<AimPointerMeshScript>
    {
        REGISTER_SCRIPT(AimPointerMeshScript, Script)

    private:
        // ─────────────────────────────────────────────
        // 참조
        // ─────────────────────────────────────────────
        AimModeController* m_aimPointer = nullptr;
        engine::Camera* m_mainCamera = nullptr;
        engine::Collider* m_collider = nullptr;

        // ─────────────────────────────────────────────
        // 설정
        // ─────────────────────────────────────────────
        std::string m_aimPointerObjectName = "Player";  // AimPointer 컴포넌트가 붙은 오브젝트 이름
        float m_fixedY = 0.0f;                          // [하위호환용] 기존 고정 Y 값 (현재 동작에서는 미사용)
        float m_scaleAtNearest = 0.8f;                  // 카메라 기준 최단 거리에서의 기본 스케일
        float m_scaleRefDistance = 1.0f;                // 런타임 기준 거리(시작 시 커서 위치 기준)
        float m_scaleMin = 0.05f;
        float m_scaleMax = 5.0f;
        std::vector<engine::Ptr<engine::GameObject>> m_overlapTargets; // 현재 트리거 중인 유효 대상
        bool m_lastOnEnemy = false;

    public:
        void Start() override;
        void Update() override;
        void OnTriggerEnter(const engine::CollisionInfo& info) override;
        void OnTriggerExit(const engine::CollisionInfo& info) override;

    public:
        // 현재 위치 반환 (다른 스크립트에서 참조용)
        engine::Vector3 GetWorldPosition() const;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void CacheAimPointer();
        void CacheCameraAndCollider();
        void UpdateDistanceBasedScale();
        void RefreshOnEnemyState();
        bool IsValidOnEnemyTarget(const engine::Ptr<engine::GameObject>& other) const;
    };
}
