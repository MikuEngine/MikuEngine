#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/UI/UIButton.h>

namespace game
{
    class TitleButtonController :
        public engine::Script<TitleButtonController>
    {
        REGISTER_COMPONENT(TitleButtonController, Script)

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
        
        void StartGame();
        void OpenOption();
        void OpenCredit();
        void QuitGame();

        void Back();

        void SetOptionOpen(bool open);
        void SetCreditOpen(bool open);
        void UpdateBlocker();

        bool m_bound = false;

        bool m_isOptionOpen = false;
        bool m_isCreditOpen = false;

    private:
        // GameObject
        engine::GameObject* m_optionPopUp = nullptr;
        engine::GameObject* m_creditPopUp = nullptr;
        engine::GameObject* m_blocker = nullptr;
    };
}