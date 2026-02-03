#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class AnimFSM;
    class LogicFSM;
}

namespace game
{
    class AimPointer;

    // ═══════════════════════════════════════════════════════════════
    // PlayerAimMeshController
    // 
    // 플레이어 Transform 위치를 따라가며, AimPointerMesh 방향으로 자전하는 스크립트
    // - 위치: 플레이어 Transform의 XZ 좌표를 그대로 따라감 (Y는 고정값)
    // - 회전: +Z 방향이 AimPointerMesh를 향하도록 Y축 자전
    // - 애니메이션: Player의 LogicFSM 상태에 따라 AnimFSM 제어
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
        engine::AnimFSM* m_animFSM = nullptr;
        engine::LogicFSM* m_logicFSM = nullptr;  // Player의 LogicFSM
        AimPointer* m_aimPointer = nullptr;

        // ─────────────────────────────────────────────
        // 위치/회전 설정
        // ─────────────────────────────────────────────
        std::string m_playerObjectName = "Player";
        std::string m_aimPointerMeshObjectName = "AimPointerMesh";
        float m_fixedY = 0.0f;

        // ─────────────────────────────────────────────
        // 애니메이션 설정
        // ─────────────────────────────────────────────
        std::string m_animName_Idle = "Idle";
        std::string m_animName_WalkForward = "WalkForward";
        std::string m_animName_WalkBackward = "WalkBackward";
        std::string m_animName_Fire = "Fire";

        // ─────────────────────────────────────────────
        // Forward/Backward 판정 설정
        // ─────────────────────────────────────────────
        float m_backwardThreshold = -0.1f;   // Forward → Backward 전환 (약 96도)
        float m_forwardThreshold = 0.1f;     // Backward → Forward 전환 (약 84도)

        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        bool m_isBackward = false;
        bool m_animFSMInitialized = false;

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
        
        // ─────────────────────────────────────────────
        // 애니메이션 관련
        // ─────────────────────────────────────────────
        void InitializeAnimFSM();
        void UpdateAnimation();
        
        // ─────────────────────────────────────────────
        // Forward/Backward 판정
        // ─────────────────────────────────────────────
        engine::Vector3 GetMoveInputDirection() const;
        void UpdateForwardBackward(const engine::Vector3& aimDir);
    };
}
