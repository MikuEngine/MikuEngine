#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class TempBullet :
        public engine::Script<TempBullet>
    {
        REGISTER_COMPONENT(TempBullet)

    private:
        engine::Vector3 m_direction;
        float m_speed = 1.0f;
        float m_lifetime = 3.0f;
        float m_elapsedTime = 0.0f;  // 생존 시간 (누적)
        
        bool m_isDying = false;      // 충돌 후 죽는 중
        float m_deathDelay = 0.05f;  // 죽기 전 대기 시간
        float m_deathTimer = 0.0f;   // 죽기 전 타이머

    public:
        void Initialize(const engine::Vector3& direction, float speed, float lifetime);
        
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        std::string GetType() const override;
    };
}
