#pragma once

#include <Framework/Object/Component/Script.h>
#include "Script/UI/UIToastAnimator.h"
#include <deque>

namespace game
{
    class UIToastAnimator;

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
        struct Item
        {
            engine::Ptr<engine::GameObject> go;
            engine::TimePoint born;
            bool exiting = false;
            bool visible = true;
        };

        std::deque<Item> m_items;

    private:
        void PushKill(const std::string& text, const std::string& iconKey);

        void Reflow(bool instant);
        engine::Vector2 CalcTargetPos(size_t index) const;
        
        void TryStartExitTop();
        void TryUnlockAndShow();
        void CleanupTopIfFinished();

        bool HasHiddenItems();

    private:
        engine::GameObject* m_canvas = nullptr;

        std::string m_prefabKey = "UIToastPopUp";

        engine::Vector2 m_spawnPos = { 660.0f, -400.0f };

        float m_spacing = 110.0f;
        float m_lifeTime = 3.0f;
        int m_maxQueue = 3;

        bool m_locked = false;
    };
}