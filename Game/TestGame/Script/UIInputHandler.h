#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class UIInputHandler :
        public engine::Script<UIInputHandler>
    {
        REGISTER_COMPONENT(UIInputHandler)
    private:
        bool m_isMenuOpened = false;
        engine::GameObject* m_menuObj = nullptr;

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;
        std::string GetType() const override;
    };
}