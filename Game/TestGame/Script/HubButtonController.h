#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/UI/UIButton.h>

namespace game
{
    class HubButtonController :
        public engine::Script<HubButtonController>
    {
        REGISTER_COMPONENT(HubButtonController, Script)
    public:
        void Awake() override;
        //void Start() override;
        //void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        void BindButton(const std::string& name, engine::UIButton::ClickCallback cb);
        void BindButton(const std::string& name, engine::UIButton::HoverCallback cb);

        // OnClick
        void EnterPlay();
        void OpenUpgrade();
        void OpenOption();
        void BackToMain();
        void BackToHub();

        // OnHover
        void SetGuide(const std::string& key);

        bool m_bound = false;
    };
}