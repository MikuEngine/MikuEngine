#pragma once

#include <Framework/Object/Component/Script.h>

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

        // ─────────────────────────────────────────────
        // 설정
        // ─────────────────────────────────────────────
        std::string m_aimPointerObjectName = "Player";  // AimPointer 컴포넌트가 붙은 오브젝트 이름
        float m_fixedY = 2.2f;                          // 고정 Y 좌표

    public:
        void Start() override;
        void Update() override;

    public:
        // 현재 위치 반환 (다른 스크립트에서 참조용)
        engine::Vector3 GetWorldPosition() const;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void CacheAimPointer();
    };
}
