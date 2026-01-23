#pragma once

#include <Framework/Object/Component/Script.h>

namespace engine
{
    class UIImage;
    class Camera;
}

namespace game
{
    class UIFollowTarget :
        public engine::Script<UIFollowTarget>
    {
        REGISTER_COMPONENT(UIFollowTarget, Script)

    public:
        void Awake() override;
        //void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        std::string m_targetName;
        engine::Vector3 m_offset{ -5.0f, 0.0f, 0.0f };
        bool m_hideWhenOffscreen = true;

        bool m_visible = true;

        engine::GameObject* m_target = nullptr;
        engine::RectTransform* m_rt = nullptr;
        engine::UIImage* m_img = nullptr;

        engine::Camera* m_camera = nullptr;
        engine::Matrix m_view;
        engine::Matrix m_proj;
        bool m_cameraCached = false;

    private:
        void RebindTarget();
        void PrepareAnchorOnce();
        void SetVisible(bool v);

    private:
        bool m_anchorsPrepared = false;
        std::string m_lastBoundName = "";
    };
}