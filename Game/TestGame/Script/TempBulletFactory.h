#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class TempBulletFactory :
        public engine::Script<TempBulletFactory>
    {
        REGISTER_COMPONENT(TempBulletFactory, Script)

    private:
        float m_bulletSpeed = 15.0f;     // 총알 속도
        float m_fireRate = 0.2f;         // 발사 간격 (초)
        float m_fireCooldown = 0.0f;     // 발사 쿨다운 타이머
        float m_bulletLifetime = 3.0f;   // 총알 수명 (초)

    public:
        void Start() override;
        void Update() override;
        
        // 총알 발사 (위치, 방향)
        void Fire(const engine::Vector3& position, const engine::Vector3& direction);
        
        // 발사 가능 여부 (쿨다운 체크)
        bool CanFire() const;

        // 속성 Getter/Setter
        float GetBulletSpeed() const { return m_bulletSpeed; }
        void SetBulletSpeed(float speed) { m_bulletSpeed = speed; }
        
        float GetFireRate() const { return m_fireRate; }
        void SetFireRate(float rate) { m_fireRate = rate; }

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
