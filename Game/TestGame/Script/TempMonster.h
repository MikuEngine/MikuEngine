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
        REGISTER_COMPONENT(TempMonster)

    private:
        engine::SpriteRenderer* m_spriteRenderer = nullptr;
        int m_hitCount = 0;

    public:
        void Start() override;
        
        // 총알에 맞았을 때 호출
        void OnHit();

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        std::string GetType() const override;

    private:
        void ToggleHitIndicator();
    };
}
