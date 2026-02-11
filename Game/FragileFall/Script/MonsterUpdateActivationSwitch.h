#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/ComponentFactory.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterUpdateActivationSwitch
    // 
    // 씬 시작 시 모든 몬스터의 업데이트를 일정 시간 동안 중지시키는 스위치
    // 
    // 사용법:
    //   1. 씬에 GameObject 생성 후 이 스크립트 부착
    //   2. GameObject 이름을 "MonsterUpdateSwitch"로 설정
    //   3. 인스펙터에서 m_activationDelay 설정 (기본 1초)
    //   4. 씬 시작 후 설정 시간이 지나면 자동으로 업데이트 허용
    // 
    // 재활용:
    //   - 다른 스크립트에서 이 오브젝트를 찾아서
    //   - m_isUpdateAllowed를 false로 설정하면 몬스터 업데이트 중지
    //   - true로 설정하면 다시 재개
    // ═══════════════════════════════════════════════════════════════

    class MonsterUpdateActivationSwitch : public engine::Script<MonsterUpdateActivationSwitch>
    {
        REGISTER_SCRIPT(MonsterUpdateActivationSwitch, Script)

    public:
        // ─────────────────────────────────────────────
        // Public 멤버 (몬스터가 접근)
        // ─────────────────────────────────────────────
        bool m_isUpdateAllowed = false;  // 몬스터 업데이트 허용 여부 (런타임 전용)
        bool m_hasActivated = false;     // 한 번 활성화되었는지 (런타임 전용)

        bool GetIsUpdateAllowed() const { return m_isUpdateAllowed; }
        bool HasActivated() const { return m_hasActivated; }
        bool ShouldWaitAfterIdle() const { return m_waitAfterIdle; }
        void BeginDelayAfterIdle();
        void SetSwitchActivation(bool setparam);
       
    private:
        // ─────────────────────────────────────────────
        // 설정 (인스펙터 직렬화)
        // ─────────────────────────────────────────────
        float m_activationDelay = 1.0f;  // 업데이트 허용까지 대기 시간 (초)
        bool m_waitAfterIdle = true;     // true: Idle 도달 후 대기 시작, false: 씬 시작 즉시 대기 시작

        // ─────────────────────────────────────────────
        // 런타임 상태
        // ─────────────────────────────────────────────
        float m_elapsedTime = 0.0f;      // 경과 시간
        bool m_waitingForDelay = false;  // 실제 대기 카운트 진행 중인지

        void ResetRuntimeState();
       

    public:
        MonsterUpdateActivationSwitch() = default;
        virtual ~MonsterUpdateActivationSwitch() = default;

        // ─────────────────────────────────────────────
        // 생명주기
        // ─────────────────────────────────────────────
        void Awake() override;
        void Start() override;
        void Update() override;         

        // ─────────────────────────────────────────────
        // GUI / 직렬화
        // ─────────────────────────────────────────────
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
