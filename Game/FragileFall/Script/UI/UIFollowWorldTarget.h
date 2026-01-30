#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/RectTransform.h>

namespace engine
{
    class UIImage;
    class Camera;
}

namespace game
{
    class UIFollowWorldTarget :
        public engine::Script<UIFollowWorldTarget>
    {
        REGISTER_SCRIPT(UIFollowWorldTarget, Script)

    public:
        void Awake() override;
        //void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        std::string                 m_targetName;
        engine::Vector3             m_offset{ -5.0f, 0.0f, 0.0f };
        bool                        m_hideWhenOffscreen = true;
        bool                        m_visible = true;

        engine::GameObject*         m_target = nullptr;
        engine::RectTransform*      m_rt = nullptr;
        engine::UIImage*            m_img = nullptr;
        engine::Camera*             m_camera = nullptr;

        engine::RectTransform*      m_parentRT = nullptr;
        engine::UIRect              m_cachedParentRect{};
        float                       m_cachedVpW = -1.f;
        float                       m_cachedVpH = -1.f;
        bool                        m_cachedVisible = true;

        bool                        m_cameraCached = false;
        bool                        m_anchorsPrepared = false;
        std::string                 m_lastBoundName;

    private:
        void RebindTarget();
        void PrepareAnchorOnce();
        void SetVisible(bool v);
    };
}