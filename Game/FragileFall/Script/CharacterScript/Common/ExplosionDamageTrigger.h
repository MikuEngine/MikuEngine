#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class SphereCollider;
}

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // ExplosionDamageTrigger - 폭발 데미지 트리거
    // 
    // 포물선 탄환 착탄 시 생성되어 플레이어에게 데미지를 줌
    // - 생성 후 0.3초간 스케일/콜라이더 확장
    // - 일정 시간 후 자동 파괴
    // - 플레이어와 충돌 시 데미지 적용
    // ═══════════════════════════════════════════════════════════════
    class ExplosionDamageTrigger :
        public engine::Script<ExplosionDamageTrigger>
    {
        REGISTER_SCRIPT(ExplosionDamageTrigger, Script)

    private:
        // ─────────────────────────────────────────────
        // 생존 시간
        // ─────────────────────────────────────────────
        float m_lifetime = 1.5f;
        float m_elapsedTime = 0.0f;
        
        // ─────────────────────────────────────────────
        // 데미지
        // ─────────────────────────────────────────────
        float m_damage = 10;
        bool m_hasDamaged = false;
        
        // ─────────────────────────────────────────────
        // 확장 애니메이션 (0.3초간)
        // ─────────────────────────────────────────────
        float m_expandDuration = 0.3f;
        float m_startScale = 1.4f;
        float m_endScale = 5.0f;
        float m_startRadius = 2.66f;
        float m_endRadius = 2.66f;
        
        // 콜라이더 캐시
        engine::SphereCollider* m_sphereCollider = nullptr;

    public:
        // 초기화 (BulletMonster에서 호출)
        void Setup(float damage, float lifetime = 1.5f);

        // 생명주기
        void Start() override;
        void Update() override;

        // 충돌 콜백
        void OnTriggerEnter(const engine::CollisionInfo& info) override;
        
    private:
        void UpdateExpansion();
    };
}
