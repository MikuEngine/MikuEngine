#pragma once

#include <Framework/Object/Component/Script.h>
#include <Framework/Object/Component/UI/UIButton.h>
#include <Framework/Object/Component/UI/UISlider.h>

namespace game
{
    class SceneController_Main :
        public engine::Script<SceneController_Main>
    {
        REGISTER_SCRIPT(SceneController_Main, Script)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        // Bind
        void BindButton(const std::string& name, engine::UIButton::ClickCallback cb);
        void BindSlider(const std::string& name, engine::UISlider::ValueChangedCallback cb);

        // Button
        void StartGame();
        void OpenOption();
        void OpenCredit();
        void QuitGame();
        void Back();

        // Slider
        //void OnMasterChanged(float v);    (미구현)
        void OnBGMChanged(float v);
        void OnSFXChanged(float v);
        void SetSensitivity(float v);

        // Execute
        void SetOptionOpen(bool open);
        void SetCreditOpen(bool open);
        void UpdateBlocker();

    private:
        // GameObject
        engine::GameObject* m_optionPopUp = nullptr;
        engine::GameObject* m_creditPopUp = nullptr;
        engine::GameObject* m_blocker = nullptr;

        bool m_bound = false;
        bool m_isOptionOpen = false;
        bool m_isCreditOpen = false;
    };
}