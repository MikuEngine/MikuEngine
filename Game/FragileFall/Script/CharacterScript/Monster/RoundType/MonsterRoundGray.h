#pragma once

#include "MonsterRoundType.h"

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterRoundGray - Gray 등급 동글 몬스터
    // 
    // 특징:
    //   - MonsterRoundType 상속
    //   - MonsterTier::Gray 고정
    //   - 기본 이동 및 공격 패턴 (추후 구현)
    // ═══════════════════════════════════════════════════════════════

    class MonsterRoundGray : public MonsterRoundType
    {
        REGISTER_SCRIPT(MonsterRoundGray, MonsterRoundType)

    public:
        void Awake() override;
        void Start() override;

    protected:
        // ─────────────────────────────────────────────
        // 오버라이드 (Gray 전용 로직)
        // ─────────────────────────────────────────────
        void InitializeBullet() override;
        void Attack(float deltaTime) override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
