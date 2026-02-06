#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class AnimFSM;
    class LogicFSM;
}

namespace game
{
    class AimModeController;
    class PlayerControllerScript;

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
        AimModeController* m_aimPointer = nullptr;
        PlayerControllerScript* m_playerControllerScript = nullptr;

        // ─────────────────────────────────────────────
        // 위치/회전 설정
        // ─────────────────────────────────────────────
        std::string m_playerObjectName = "Player";
        std::string m_aimPointerMeshObjectName = "AimPointerMesh";
        float m_fixedY = 0.0f;
        /** 이동 중일 때만 적용. 에임이 하체 방향과 이 각도(도) 이상 벌어졌을 때만 하체 회전. 0이면 항상 에임 정확히 추적. Idle/가만히 있을 때는 미적용 */
        float m_lowerBodyAimThresholdDeg = 0.0f;
        /** 하체 회전 보간 속도 (1초에 이 비율만큼 목표 방향으로 회전, Slerp 계수). 클수록 빨리 맞춤 */
        float m_lowerBodyTurnSpeed = 8.0f;

        // ─────────────────────────────────────────────
        // 애니메이션 설정
        // ─────────────────────────────────────────────
        /** Walk+발사 시 상체 절차 회전에 더하는 Yaw 오프셋(도). Fire 애니가 비스듬히 서서 쏘는 경우 보정용 (예: 왼쪽 보면 +값) */
        float m_upperBodyAimOffsetDeg = 0.0f;
        /** 상체 Yaw 스케일. 정면은 맞는데 옆 조준 시 손 방향이 어긋나면 조정 (1=기본, 보통 0.8~1.2) */
        float m_upperBodyYawScale = 1.0f;
        std::string m_animName_Idle = "Idle";
        std::string m_animName_WalkForward = "WalkForward";
        std::string m_animName_WalkBackward = "WalkBackward";
        std::string m_animName_Fire = "Fire";
        std::string m_animName_Dash = "Dash";
        std::string m_animName_Execution = "Execution";
        /** 총 메쉬 본 이름 (루트 직계). 비어있지 않으면 손 본을 따라가도록 SetBoneFollowBone 등록 */
        std::string m_gunBoneName;
        /** 손 본 이름 (총 본이 이 본의 pose를 따름). m_gunBoneName과 둘 다 설정 시에만 연동 활성화 */
        std::string m_handBoneName;

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
        
        // ─────────────────────────────────────────────
        // Shooting 상태 (PCS 콜백으로 제어)
        // ─────────────────────────────────────────────
        float m_shootingDuration = 0.5f;   // Shooting 상태 유지 시간
        float m_shootingTimer = 0.0f;      // 남은 Shooting 시간
        bool m_isShooting = false;         // 현재 Shooting 상태

        // ─────────────────────────────────────────────
        // Fire 애니메이션 발사 프레임 동기화
        // ─────────────────────────────────────────────
        /** Fire 애니메이션에서 실제 총알 발사 모션이 나오는 시간 (정규화된 시간, 0.0~1.0) */
        float m_fireAnimShootFrameTime = 0.2f;  // 기본값: 애니메이션의 20% 지점
        /** 이전 프레임 Fire 레이어 정규화 시간 (발사 프레임 "통과" 감지용, -1 = 미사용) */
        float m_prevFireNormalizedTime = -1.0f;
        /** Shoot 상태 누적 시간 (AnimFSM 상태 전환과 무관하게 계속 증가) */
        float m_accumulatedShootTime = 0.0f;

        // ─────────────────────────────────────────────
        // Procedural Yaw: 하체 회전 보정용 (이전 프레임 하체 Yaw)
        // ─────────────────────────────────────────────
        float m_prevLowerBodyYawDeg = 0.0f;

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
        float CalculateAimYaw() const;       // 캐릭터 전방 대비 에임 방향 Yaw (도)
        float GetLowerBodyYawDegrees() const; // 메시(하체) 전방 Yaw (도)
        
        // ─────────────────────────────────────────────
        // Forward/Backward 판정
        // ─────────────────────────────────────────────
        engine::Vector3 GetMoveInputDirection() const;
        void UpdateForwardBackward(const engine::Vector3& aimDir);
        
        // ─────────────────────────────────────────────
        // Shooting 콜백 (PCS에서 호출)
        // ─────────────────────────────────────────────
        void OnPlayerFired();
        void UpdateShootingState();
    };
}
