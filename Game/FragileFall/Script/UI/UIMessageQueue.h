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
    private:
        enum class UIMessageChannel
        {
            Kill,
            Tutorial,
            System,
        };

        struct ChannelConfig
        {
            engine::GameObject* canvas = nullptr;
            std::string prefabKey = "UIToastPopUp";
            engine::Vector2 spawnPos{ 0, 0 };
            float spacing = 110.0f;
            float lifeTime = 3.0f;
            int maxVisible = 3;
        };

    public:
        void Awake() override;
        void Start() override;
        void Update() override;

    public:
        void OnGui() override;
        void Save(engine::json& j) const override;
        void Load(const engine::json& j) override;

    public:
        void PushMessage(UIMessageChannel ch, const std::string& text, const std::string& iconKey = "");

    private:
        struct Item
        {
            UIMessageChannel channel = UIMessageChannel::Kill;
            engine::Ptr<engine::GameObject> go;
            engine::TimePoint born{};
            bool exiting = false;
            bool visible = true;
        };

        std::deque<Item> m_itemsKill;
        std::deque<Item> m_itemsTutorial;

        ChannelConfig m_kill;
        ChannelConfig m_tutorial;

        void TryStartExitVisibleBatch(std::deque<Item>& q, const ChannelConfig& cfg);
        void CleanupFinishedVisible(std::deque<Item>& q, const ChannelConfig& cfg);
        void TrySpawnNextBatchIfEmpty(std::deque<Item>& q, const ChannelConfig& cfg);
        bool HasHiddenItems(const std::deque<Item>& q) const;

        void ReflowVisible(std::deque<Item>& q, const ChannelConfig& cfg, bool instant);
        engine::Vector2 CalcTargetPos(const ChannelConfig& cfg, size_t visibleIndex) const;

        void UpdateChannel(std::deque<Item>& q, ChannelConfig& cfg);
            
    private:
        //engine::Vector2 m_spawnPos = { 660.0f, -400.0f };
    };
}