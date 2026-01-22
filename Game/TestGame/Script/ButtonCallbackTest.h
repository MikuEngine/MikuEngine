#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/UI/UIButton.h>

namespace game
{
    class ButtonCallbackTest :
        public engine::Script<ButtonCallbackTest>
    {
        REGISTER_COMPONENT(ButtonCallbackTest, Script)

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
        
        void CallThis();

        void OnStart();
        void OnOption();
        void OnCredit();
        void OnQuit();

        bool m_bound = false;
    };
}