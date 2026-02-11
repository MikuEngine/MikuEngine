#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Ptr.h>

namespace engine
{
    class Camera;
    class Canvas;
    class UIText;
    class RectTransform;
    class GameObject;
}

namespace game
{
    class PlayerControllerScript;
    class AimModeController;

    // Shows two debug vectors above player:
    // 1) Last fired bullet XZ direction
    // 2) Player->cursor world XZ direction
    class PlayerAimDebugTextRenderer :
        public engine::Script<PlayerAimDebugTextRenderer>
    {
        REGISTER_SCRIPT(PlayerAimDebugTextRenderer, Script)

    private:
        engine::Ptr<engine::Camera> m_mainCamera;
        engine::Ptr<engine::Canvas> m_canvas;
        engine::Ptr<engine::RectTransform> m_parentRT;

        engine::Ptr<PlayerControllerScript> m_player;
        engine::Ptr<AimModeController> m_aim;

        engine::Ptr<engine::GameObject> m_textObject;
        engine::Ptr<engine::UIText> m_uiText;
        engine::Ptr<engine::RectTransform> m_textRect;

        std::string m_prefabName = "StateText";
        std::string m_playerObjectName = "Player";
        engine::Vector3 m_worldOffset{ 0.0f, 2.5f, 0.0f };
        bool m_hideWhenOffscreen = true;
        int m_fontSize = 28;

        float m_cachedVpW = -1.0f;
        float m_cachedVpH = -1.0f;
        float m_cachedParentRectX = 0.0f;
        float m_cachedParentRectY = 0.0f;

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void ResolveCanvas();
        void ResolvePlayerRefs();
        bool EnsureTextObject();
        void SetTextVisible(bool visible);
        bool WorldToScreen(const engine::Vector3& worldPos, engine::Vector2& screenPos);
    };
}
