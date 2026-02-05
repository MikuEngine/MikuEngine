#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class UIClickArea;
}

namespace game
{
    class LobbyInteraction :
        public engine::Script<LobbyInteraction>
    {
        REGISTER_SCRIPT(LobbyInteraction, Script)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void Interact(const engine::Vector2& delta);

        float m_yawDeg = 0.0f;
        float m_radPerPixel = 0.5f;

        engine::GameObject* m_player = nullptr;
        engine::UIClickArea* m_clickArea = nullptr;
    };
}