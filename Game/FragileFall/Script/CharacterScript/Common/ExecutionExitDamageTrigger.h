#pragma once

#include <Framework/Object/Component/Script.h>
#include <unordered_set>

namespace engine
{
    class SphereCollider;
}

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // ExecutionExitDamageTrigger - 처형 종료 데미지 트리거
    // 
    // 플레이어 처형 종료 시 생성되어 주변 적에게 데미지를 줌
    // - 생성 위치: 플레이어 XZ, Y=0 (프리팹 자체 높이 1.5)
    // - 범위: 스케일로 조정 가능
    // - 대상: Enemy, JumpingEnemy, SplittingEnemy, BossBigProjectile, Boss
    // - 각 적에게 1번만 데미지 적용
    // - 일정 시간 후 자동 파괴
    // ═══════════════════════════════════════════════════════════════
    class ExecutionExitDamageTrigger :
        public engine::Script<ExecutionExitDamageTrigger>
    {
        REGISTER_SCRIPT(ExecutionExitDamageTrigger, Script)

    private:
        // ─────────────────────────────────────────────
        // 생존 시간
        // ─────────────────────────────────────────────
        float m_lifetime = 1.0f;
        float m_elapsedTime = 0.0f;
        
        // ─────────────────────────────────────────────
        // 데미지
        // ─────────────────────────────────────────────
        float m_damage = 15.0f;
        
        // ─────────────────────────────────────────────
        // 범위 스케일 (반지름)
        // ─────────────────────────────────────────────
        float m_radiusScale = 5.0f;
        
        // ─────────────────────────────────────────────
        // 데미지를 받은 오브젝트 추적 (중복 방지)
        // Ptr<GameObject>의 raw pointer를 저장
        // ─────────────────────────────────────────────
        std::unordered_set<void*> m_damagedObjects;
        
        // 콜라이더 캐시
        engine::SphereCollider* m_sphereCollider = nullptr;

    public:
        // 초기화 (PlayerControllerScript에서 호출)
        void Setup(float damage, float radiusScale, float lifetime);

        // 생명주기
        void Start() override;
        void Update() override;

        // 충돌 콜백
        void OnTriggerEnter(const engine::CollisionInfo& info) override;
    };
}
