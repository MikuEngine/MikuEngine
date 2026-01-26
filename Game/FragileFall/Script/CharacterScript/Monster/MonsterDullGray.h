#pragma once

#include "MonsterScript.h"

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterDullGray - DullGray 몬스터 구현
    // 
    // 특징:
    //   - 제자리 고정형 적 (이동 불가)
    //   - 플레이어 감지 시 자동 공격
    //   - 사거리 내 진입: Idle → Engage
    //   - 사거리 이탈: Engage → Idle
    // 
    // FSM 상태:
    //   - Idle: 플레이어 미감지, 대기
    //   - Engage: 플레이어 추적 + 회전 + 발사
    //   - Dead: 사망 (모든 행동 정지)
    // ═══════════════════════════════════════════════════════════════
    class MonsterDullGray : public MonsterScript
    {
        REGISTER_SCRIPT(MonsterDullGray, MonsterScript)

    private:
        // ─────────────────────────────────────────────
        // 애니메이션 이름 (Initialize에서 설정)
        // m_animName_Attack은 부모 클래스 MonsterScript에 정의됨
        // ─────────────────────────────────────────────
        std::string m_animName_Idle = "Idle";
        std::string m_animName_Dead = "Dead";

    public:
        void Awake() override;

    protected:
        // ─────────────────────────────────────────────
        // MonsterScript 오버라이드
        // ─────────────────────────────────────────────
        void InitializeFSM() override;
        void InitializeAnimFSM() override;
        void InitializeAnimations() override;
        void InitializeBullet() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
