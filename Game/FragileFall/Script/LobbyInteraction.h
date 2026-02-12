#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class UIClickArea;
    class UIText;
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

        void SetInteractionActive(bool active);

    private:
        void Interact(const engine::Vector2& delta);
        void HandleZoom(float wheelDelta);

        float m_yawDeg = 0.0f;
        float m_radPerPixel = 0.5f;

        float m_currentDistance = 0.0f;
        float m_wheelDelta = 0.0f;

        bool m_isActive = true;

        engine::GameObject* m_camera = nullptr;
        engine::GameObject* m_player = nullptr;
        engine::UIClickArea* m_clickArea = nullptr;
        engine::UIText* m_guideText = nullptr;

        engine::Vector3 m_cameraPos = {};
        engine::Vector3 m_dirFromTarget = {};
    };
}