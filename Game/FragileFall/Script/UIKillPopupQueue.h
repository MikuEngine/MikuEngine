#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class UIKillPopupQueue :
        public engine::Script<UIKillPopupQueue>
    {
        REGISTER_SCRIPT(UIKillPopupQueue, Script)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    private:
        //void PushKill(const std::string& text);

    private:
        engine::GameObject* m_container = nullptr;
        engine::GameObject* m_prefab = nullptr;
        engine::GameObject* m_canvas = nullptr;

        float m_spacing = 6.0f;
        float m_lifeTime = 1.2f;
        int m_maxQueue = 6;
    };
}