#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class AimPointerMeshScript;

    // ═══════════════════════════════════════════════════════════════
    // OrbitScript
    // 
    // 플레이어 Transform 위치를 중심으로, AimPointerMesh 방향을 향해 공전하는 스크립트
    // - Y 좌표는 고정값 사용
    // - 물리(Rigidbody, Collider) 없음
    // - +Z 방향이 AimPointerMesh를 향하도록 Y축 회전
    // - 즉시(Snap) 회전, 보간 없음
    // ═══════════════════════════════════════════════════════════════
    class OrbitScript :
        public engine::Script<OrbitScript>
    {
        REGISTER_SCRIPT(OrbitScript, Script)

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
        float m_fixedY = 2.2f;                                  // 고정 Y 좌표
        float m_orbitRadius = 2.0f;                             // 공전 반경

    public:
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void CacheReferences();
        void UpdateOrbit();
    };
}
