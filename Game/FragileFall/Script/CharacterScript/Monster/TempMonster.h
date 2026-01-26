#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class SpriteRenderer;
}

namespace game
{
    class TempMonster :
        public engine::Script<TempMonster>
    {
        REGISTER_SCRIPT(TempMonster, Script)

    private:
        engine::SpriteRenderer* m_spriteRenderer = nullptr;
        int m_hitCount = 0;
        bool m_isCrystalized = false;

    public:
        void Start() override;
        
        // 총알에 맞았을 때 호출 (외부에서 직접 호출 가능)
        void OnHit();
        
        // Push 방식 충돌 콜백 (CollisionSystem에서 자동 호출)
        void OnTriggerEnter(const engine::CollisionInfo& info) override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void ToggleHitIndicator();
    };
}
