#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class AimPointerMeshScript;
    class PlayerControllerScript;

    // ═══════════════════════════════════════════════════════════════
    // OrbitScript
    // 
    // 플레이어 Transform 위치를 중심으로, AimPointerMesh 방향을 향해 공전하는 스크립트
    // - Y 좌표는 고정값 사용
    // - 물리(Rigidbody, Collider) 없음
    // - +Z 방향이 AimPointerMesh를 향하도록 Y축 회전
    // - 즉시(Snap) 회전, 보간 없음
    // - 발사 시 반동 효과 (위치/회전 offset)
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
        PlayerControllerScript* m_playerController = nullptr;

        // ─────────────────────────────────────────────
        // 공전 설정
        // ─────────────────────────────────────────────
        std::string m_playerObjectName = "Player";
        std::string m_aimPointerMeshObjectName = "AimPointerMesh";
        float m_fixedY = 2.2f;
        float m_orbitRadius = 2.0f;

        // ─────────────────────────────────────────────
        // 반동 설정
        // ─────────────────────────────────────────────
        float m_recoilOffsetZ = -0.2f;      // 뒤로 밀림 (-Z)
        float m_recoilOffsetY = 0.1f;       // 위로 밀림 (+Y)
        float m_recoilRotationX = 25.0f;    // 총구 들림 (X축 회전, 도)

        // ─────────────────────────────────────────────
        // 반동 런타임 상태
        // ─────────────────────────────────────────────
        bool m_isRecoiling = false;
        float m_recoilTimer = 0.0f;
        float m_recoilDuration = 0.2f;      // PCS의 fireRate / 2로 자동 계산됨

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
        
        // ─────────────────────────────────────────────
        // 반동 시스템
        // ─────────────────────────────────────────────
        void OnFired();                     // 발사 콜백
        void UpdateRecoil(float deltaTime);
        float GetRecoilScale() const;       // 0~1~0 보간값
    };
}
