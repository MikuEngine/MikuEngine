#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // PlayerAimMeshController
    // 
    // 플레이어 Transform 위치를 따라가며, AimPointerMesh 방향으로 자전하는 스크립트
    // - 위치: 플레이어 Transform의 XZ 좌표를 그대로 따라감 (Y는 고정값)
    // - 회전: +Z 방향이 AimPointerMesh를 향하도록 Y축 자전
    // - 물리(Rigidbody, Collider) 없음
    // - 스케일은 건드리지 않음
    // ═══════════════════════════════════════════════════════════════
    class PlayerAimMeshController :
        public engine::Script<PlayerAimMeshController>
    {
        REGISTER_SCRIPT(PlayerAimMeshController, Script)

    private:
        // ─────────────────────────────────────────────
        // 참조
        // ─────────────────────────────────────────────
        engine::GameObject* m_playerObject = nullptr;
        engine::GameObject* m_aimPointerMeshObject = nullptr;

        // ─────────────────────────────────────────────
        // 설정
        // ─────────────────────────────────────────────
        std::string m_playerObjectName = "Player";              // 플레이어 오브젝트 이름
        std::string m_aimPointerMeshObjectName = "AimPointerMesh";  // AimPointerMesh 오브젝트 이름
        float m_fixedY = 0.0f;                                  // 고정 Y 좌표

    public:
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void CacheReferences();
        void UpdatePositionAndRotation();
    };
}
