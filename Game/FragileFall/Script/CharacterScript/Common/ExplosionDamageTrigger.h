#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // ExplosionDamageTrigger - 폭발 데미지 트리거
    // 
    // 포물선 탄환 착탄 시 생성되어 플레이어에게 데미지를 줌
    // - 일정 시간 후 자동 파괴
    // - 플레이어와 충돌 시 데미지 적용
    // ═══════════════════════════════════════════════════════════════
    class ExplosionDamageTrigger :
        public engine::Script<ExplosionDamageTrigger>
    {
        REGISTER_SCRIPT(ExplosionDamageTrigger, Script)

    private:
        float m_lifetime = 3.0f;          // 생존 시간
        float m_elapsedTime = 0.0f;       // 경과 시간
        int m_damage = 10;                // 데미지
        bool m_hasDamaged = false;        // 이미 데미지를 주었는지 (1회만)

    public:
        // 초기화 (BulletMonster에서 호출)
        void Setup(int damage, float lifetime = 3.0f);

        // 생명주기
        void Start() override;
        void Update() override;

        // 충돌 콜백
        void OnTriggerEnter(const engine::CollisionInfo& info) override;
    };
}
