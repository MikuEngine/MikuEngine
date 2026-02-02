#pragma once

#include <Framework/Object/Component/Script.h>
#include "Script/UI/UIToastAnimator.h"
#include <deque>

namespace game
{
    class UIToastAnimator;

    class UIMessageQueue :
        public engine::Script<UIMessageQueue>
    {
        REGISTER_SCRIPT(UIMessageQueue, Script)

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    public:
        void PushMessage(const std::string& text, const std::string& iconKey = "");

    private:
        struct Item
        {
            engine::Ptr<engine::GameObject> go;
            engine::TimePoint born;

            bool exiting = false;
            bool visible = true;
        };

        std::deque<Item> m_items;

        void TryStartExitVisibleBatch();
        void CleanupFinishedVisible();
        void TrySpawnNextBatchIfEmpty();
        bool HasHiddenItems() const;

        void ReflowVisible(bool instant);
        engine::Vector2 CalcTargetPos(size_t visibleIndex) const;


    private:
        engine::GameObject* m_canvas = nullptr;

        std::string m_prefabKey = "UIToastPopUp";
        engine::Vector2 m_spawnPos = { 660.0f, -400.0f };

        float m_spacing = 110.0f;
        float m_lifeTime = 3.0f;
        int m_maxVisible = 3;
    };
}