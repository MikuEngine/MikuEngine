#pragma once

#include "MonsterRoundType.h"

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // MonsterRoundBlue - Blue 등급 동글 몬스터
    // 
    // 특징:
    //   - MonsterRoundType 상속
    //   - MonsterTier::Blue 고정
    //   - 특수 이동 및 공격 패턴 (추후 구현)
    // ═══════════════════════════════════════════════════════════════

    class MonsterRoundBlue : public MonsterRoundType
    {
        REGISTER_SCRIPT(MonsterRoundBlue, MonsterRoundType)

    public:
        void Awake() override;
        void Start() override;

    protected:
        // ─────────────────────────────────────────────
        // 오버라이드 (Blue 전용 로직)
        // ─────────────────────────────────────────────
        void InitializeBullet() override;
        void Attack(float deltaTime) override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
