#pragma once

#include "BaseControllerScript.h"

namespace engine
{
    class Rigidbody;
}

namespace game
{
    class AimPointer;
    class TempBulletFactory;

    // ═══════════════════════════════════════════════════════════════
    // TestShootingPlayer - BaseControllerScript를 상속받은 슈팅 플레이어
    // 
    // 기능:
    //   - WASD 이동
    //   - 마우스 클릭으로 총알 발사
    //   - AimPointer를 향해 발사
    //   - 상체 조준 (선택적)
    // ═══════════════════════════════════════════════════════════════
    class TestShootingPlayer : 
        public BaseControllerScript
    {
        REGISTER_COMPONENT(TestShootingPlayer, BaseControllerScript)

    protected:
        // 추가 컴포넌트 참조
        engine::Rigidbody* m_rigidbody = nullptr;
        AimPointer* m_aimPointer = nullptr;
        TempBulletFactory* m_bulletFactory = nullptr;

        // 이동 속도
        float m_moveSpeed = 5.0f;

        // 상체 조준 설정
        bool m_enableUpperBodyAim = true;

        // FSM 초기화 플래그
        bool m_fsmInitialized = false;

        // 연사 속도 관리
        float m_fireRate = 0.2f;  // 초당 발사 횟수 (0.2초 = 5발/초)
        float m_fireTimer = 0.0f;  // 발사 타이머

    public:
        void Awake() override;
        void Start() override;

    protected:
        // ─────────────────────────────────────────────
        // 컴포넌트 캐싱 (BaseControllerScript 오버라이드)
        // ─────────────────────────────────────────────
        void CacheComponents() override;

        // ─────────────────────────────────────────────
        // 입력 처리 및 게임 로직 (BaseControllerScript 오버라이드)
        // ─────────────────────────────────────────────
        void ProcessInput() override;
        void UpdateGameLogic() override;

        // ─────────────────────────────────────────────
        // 상태 변화 콜백 (BaseControllerScript 오버라이드)
        // ─────────────────────────────────────────────
        void OnStateEntered(const std::string& state) override;

    private:
        // FSM 초기화 (스테이트 및 전이 설정)
        void InitializeFSM();

        // 플레이어 전용 입력 함수
        engine::Vector3 GetMoveInputDirection() const;

        // 플레이어 전용 액션
        void HandleShooting();
        void UpdateUpperBodyAim();
        float CalculateAimYaw() const;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}