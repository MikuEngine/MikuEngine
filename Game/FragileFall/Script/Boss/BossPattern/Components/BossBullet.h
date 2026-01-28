#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class StaticMeshRenderer;
}

namespace game
{
    class PlayerControllerScript;

    class BossBullet :
        public engine::Script<BossBullet>
    {
        REGISTER_SCRIPT(BossBullet, Script)

    private:
        engine::Vector3 m_direction;  // 이동 방향 (정규화됨)
        float m_speed = 10.0f;  // 이동 속도
        float m_damage = 20.0f;  // 데미지
        float m_lifetime = 5.0f;  // 수명 (초)
        float m_elapsedTime = 0.0f;  // 경과 시간

        bool m_isDestroyed = false;

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

        void Setup(const engine::Vector3& direction, float speed, float damage, float lifetime);

        void OnTriggerEnter(const engine::CollisionInfo& info) override;

        bool IsDestroyed() const { return m_isDestroyed; }
        float GetDamage() const { return m_damage; }

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}
