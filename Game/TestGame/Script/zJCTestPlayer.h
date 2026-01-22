#pragma once

#include "BaseControllerScript.h"

namespace engine
{
    class Rigidbody;
}

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // zJCTestPlayer - 4방향 이동 테스트용 플레이어 스크립트
    // 
    // 기능:
    //   - WASD로 상하좌우 이동
    //   - 5개 스테이트: Idle, MoveUp, MoveDown, MoveLeft, MoveRight
    //   - 스테이트 진입/나갈 때 디버그 로그 출력
    // ═══════════════════════════════════════════════════════════════
    class zJCTestPlayer :
        public BaseControllerScript
    {
        REGISTER_COMPONENT(zJCTestPlayer, BaseControllerScript)

    protected:
        // 추가 컴포넌트 참조
        engine::Rigidbody* m_rigidbody = nullptr;

        // 이동 속도
        float m_moveSpeed = 5.0f;

        // FSM 초기화 플래그
        bool m_fsmInitialized = false;

        // 씬 파일에서 로드된 FSM 정보 (에디터 표시용)
        struct LoadedFSMInfo
        {
            std::string currentState;
            bool hasStates = false;
        };
        LoadedFSMInfo m_loadedFSMInfo;

    public:
        void Awake() override;
        void Start() override;
        void Update() override;
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    protected:
        // ─────────────────────────────────────────────
        // 입력 처리 및 게임 로직 (BaseControllerScript 오버라이드)
        // ─────────────────────────────────────────────
        void ProcessInput() override;
        void UpdateGameLogic() override;

        // ─────────────────────────────────────────────
        // 상태 변화 콜백 (BaseControllerScript 오버라이드)
        // ─────────────────────────────────────────────
        void OnStateEntered(const std::string& state) override;
        void OnStateExited(const std::string& state) override;

    protected:
        // ─────────────────────────────────────────────
        // 컴포넌트 캐싱 (BaseControllerScript 오버라이드)
        // ─────────────────────────────────────────────
        void CacheComponents() override;

    private:
        // FSM 초기화 (스테이트 및 전이 설정)
        void InitializeFSM();
    };
}