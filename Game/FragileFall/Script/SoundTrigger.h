#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class SoundTrigger :
        public engine::Script<SoundTrigger>
    {
        REGISTER_SCRIPT(SoundTrigger, Script)

    private:
        std::string m_soundKey;
        std::string m_soundOption;
        bool m_isActive = true;

		float m_radius = 8.0f;

    public:
        //void Awake() override;
        void Start() override;
        //void Update() override;

        void SetActivateSound(bool active);

        void OnTriggerEnter(const engine::CollisionInfo& info) override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
    };
}