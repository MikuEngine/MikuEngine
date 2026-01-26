#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/UI/UIButton.h>

namespace game
{
    class SceneController_Play :
        public engine::Script<SceneController_Play>
    {
        REGISTER_COMPONENT(SceneController_Play, Script)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:

        void BindButton(const std::string& name, engine::UIButton::ClickCallback cb);

        void ToggleMenu();
        void SetMenuOpen(bool open);

        void OpenMenu();

        void OpenOption();
        void BackToPlay();
        void BackToMain();

        bool m_bound = false;

        bool m_isMenuOpen = false;
        engine::GameObject* m_menu = nullptr;
        engine::GameObject* m_blocker = nullptr;
    };
}