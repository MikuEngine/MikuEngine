#pragma once

#include <Framework/Object/Component/Script.h>
#include "Script/UI/UIToastAnimator.h"
#include <deque>
#include <vector>

namespace game
{
    class UIToastAnimator;
    class MessageCatalog;

    enum class UIMessageChannel
    {
        Tutorial = 1,
        TutorialLobby = 2,
        System = 3,

        Kill = 0,
    };

    class UIMessageQueue :
        public engine::Script<UIMessageQueue>
    {
        REGISTER_SCRIPT(UIMessageQueue, Script)
    private:

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
        void SetCatalog(MessageCatalog* catalog) { m_catalog = catalog; }
        void PushMessageKey(const std::string& key);
        void CollectCatalogKeys(UIMessageChannel ch, const std::string& prefix, std::vector<std::string>& outKeys) const;




        void PushMessage(UIMessageChannel ch, const std::string& text, const std::string& iconKey = "");
        void Advance(UIMessageChannel ch, float fadeOutOverride = -1.0f);
        void SetSingle(UIMessageChannel ch, const std::string& text, bool playEnter = true);
        void ClearChannel(UIMessageChannel ch, float fadeOutOverride = 0.15f);

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
        std::deque<Item> m_itemsTutorialLobby;

        ChannelConfig m_kill;
        ChannelConfig m_tutorial;
        ChannelConfig m_tutorialLobby;

        // 직렬화 대상: Kill 토스트 프리팹 이름만 인스펙터에서 수정 가능
        std::string m_killPrefabKey = "UIToastPopUp";

        MessageCatalog* m_catalog = nullptr;

        void TryStartExitVisibleBatch(std::deque<Item>& q, const ChannelConfig& cfg);
        void CleanupFinishedVisible(std::deque<Item>& q, const ChannelConfig& cfg);
        void TrySpawnNextBatchIfEmpty(std::deque<Item>& q, const ChannelConfig& cfg);
        bool HasHiddenItems(const std::deque<Item>& q) const;

        void ReflowVisible(std::deque<Item>& q, const ChannelConfig& cfg, bool instant);
        engine::Vector2 CalcTargetPos(const ChannelConfig& cfg, size_t visibleIndex) const;

        void UpdateChannel(std::deque<Item>& q, ChannelConfig& cfg);
    };
}