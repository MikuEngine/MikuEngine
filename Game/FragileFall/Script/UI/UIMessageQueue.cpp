#include "GamePCH.h"
#include "UIMessageQueue.h"
#include <Framework/Asset/Prefab.h>
#include <Framework/Object/Component/RectTransform.h>

#include "Manager/MessageCatalog.h"

namespace game
{
    void UIMessageQueue::Awake()
    {
        m_kill.canvas = engine::GameObject::Find("Canvas_KillPopUp");
        m_kill.spawnPos = { 660.0f, -400.0f };
        m_kill.maxVisible = 3;
        m_kill.lifeTime = 3.0f;
        m_kill.prefabKey = "UIToastPopUp";

        m_tutorial.canvas = engine::GameObject::Find("Canvas_Message"); // 튜토리얼
        m_tutorial.spawnPos = { 510.0f, -300.0f };  // 원하는 위치
        m_tutorial.maxVisible = 3;                 
        m_tutorial.lifeTime = 9999.0f;         
        m_tutorial.spacing = 160.0f;
        m_tutorial.prefabKey = "UIToastPopUp_Tutorial";
    }

    void UIMessageQueue::Start()
    {
        
    }

    void UIMessageQueue::Update()
    {
        // 메시지 흐름
        // if문으로 제어
        if (engine::Input::IsKeyPressed(engine::Keys::K))
        {
            PushMessage(UIMessageChannel::Kill, "asdf", "");
        }

        UpdateChannel(m_itemsKill, m_kill);
        UpdateChannel(m_itemsTutorial, m_tutorial);
    }

    void UIMessageQueue::OnGui()
    {

    }

    void UIMessageQueue::Save(engine::json& j) const
    {
        Object::Save(j);

    }

    void UIMessageQueue::Load(const engine::json& j)
    {
        Object::Load(j);

    }

    void UIMessageQueue::PushMessageKey(const std::string& key)
    {
        if (!m_catalog)
            return;

        UIMessageChannel ch{};
        std::string text;
        std::string iconKey;

        if (!m_catalog->TryGet(key, ch, text, iconKey))
        {
            // 못찾았을 시 로그
            LOG_PRINT("[UIMessageQueue] key not found: {}", key);
            return;
        }

        LOG_PRINT("[Msg] text={}", text);

        PushMessage(ch, text, iconKey);
    }

    void UIMessageQueue::PushMessage(UIMessageChannel ch, const std::string& text, const std::string& iconKey)
    {
        auto& q = (ch == UIMessageChannel::Kill) ? m_itemsKill : m_itemsTutorial;
        auto& cfg = (ch == UIMessageChannel::Kill) ? m_kill : m_tutorial;

        if (!cfg.canvas) return;

        // 프리팹 생성
        engine::Ptr<engine::GameObject> go = engine::Prefab::Instantiate(cfg.prefabKey);
        if (!go) return;

        // UI는 캔버스의 자식으로 추가
        go->GetTransform()->SetParent(cfg.canvas->GetTransform());

        auto* rt = go->GetComponent<engine::RectTransform>();
        auto* anim = go->GetComponent<game::UIToastAnimator>();

        if (!rt || !anim)
        {
            go->Destroy();
            return;
        }

        anim->SetText(text);

        int visibleCount = 0;
        for (auto& it : q)
            if (it.visible && it.go) visibleCount++;

        Item item;
        item.go = go;
        item.born = engine::Time::GetTimestamp();
        item.exiting = false;

        if (visibleCount >= cfg.maxVisible)
        {
            item.visible = false;
            go->SetActive(false);
            q.push_back(item);
            return;
        }

        item.visible = true;
        q.push_back(item);

        ReflowVisible(q, cfg, false);

        const engine::Vector2 target = CalcTargetPos(cfg, (size_t)visibleCount);
        anim->PlayEnter(target);
    }

    void UIMessageQueue::Advance(UIMessageChannel ch, float fadeOutOverride)
    {
        auto& q = (ch == UIMessageChannel::Kill) ? m_itemsKill : m_itemsTutorial;
        auto& cfg = (ch == UIMessageChannel::Kill) ? m_kill : m_tutorial;

        // visible 중 "첫 번째(=현재)" 하나만 페이드아웃
        for (auto& it : q)
        {
            if (!it.go || !it.visible) continue;
            if (it.exiting) return; // 이미 나가는 중이면 중복 방지

            it.exiting = true;

            if (auto* anim = it.go->GetComponent<game::UIToastAnimator>())
                anim->FadeOut(fadeOutOverride);  // UIToastAnimator에 override 지원하셨으니 그대로
            else
                it.go->Destroy();

            return;
        }
    }

    void UIMessageQueue::SetSingle(UIMessageChannel ch, const std::string& text, bool playEnter)
    {
        auto& q = (ch == UIMessageChannel::Kill) ? m_itemsKill : m_itemsTutorial;

        // 1) 이미 보이는 게 있으면 (단, 나가는 중이 아닌 것만)
        for (auto& it : q)
        {
            // exiting이 true인 것은 곧 파괴될 것이므로 무시합니다.
            if (!it.go || !it.visible || it.exiting) continue;

            if (auto* anim = it.go->GetComponent<game::UIToastAnimator>())
            {
                anim->SetText(text);
                return;
            }
        }

        // 2) 유효하게 보이는 게 없으면 새로 생성
        PushMessage(ch, text, "");
    }

    void UIMessageQueue::ClearChannel(UIMessageChannel ch, float fadeOutOverride)
    {
        auto& q = (ch == UIMessageChannel::Kill) ? m_itemsKill : m_itemsTutorial;

        for (auto& it : q)
        {
            if (!it.go || !it.visible) continue;
            if (it.exiting) continue;

            it.exiting = true;
            if (auto* anim = it.go->GetComponent<game::UIToastAnimator>())
                anim->FadeOut(fadeOutOverride);
            else
                it.go->Destroy();
        }
        q.clear();
    }

    void UIMessageQueue::TryStartExitVisibleBatch(std::deque<Item>& q, const ChannelConfig& cfg)
    {
        int visibleAlive = 0;
        engine::TimePoint oldestBorn{};
        bool oldestSet = false;

        for (auto& it : q)
        {
            if (!it.visible || !it.go) continue;

            visibleAlive++;

            if (!oldestSet || it.born < oldestBorn)
            {
                oldestBorn = it.born;
                oldestSet = true;
            }
        }

        if (visibleAlive <= 0) return;

        // 배치 기준 시간 = visible 중 가장 오래된 born
        const float age = engine::Time::GetElapsedSeconds(oldestBorn);
        if (age < cfg.lifeTime) return;

        // visible 전부 FadeOut 시작
        for (auto& it : q)
        {
            if (!it.visible || !it.go) continue;
            if (it.exiting) continue;

            if (auto* anim = it.go->GetComponent<game::UIToastAnimator>())
            {
                it.exiting = true;
                anim->FadeOut();
            }
            else
            {
                it.exiting = true;
            }
        }
    }

    void UIMessageQueue::CleanupFinishedVisible(std::deque<Item>& q, const ChannelConfig& cfg)
    {
        for (auto& it : q)
        {
            if (!it.visible || !it.go) continue;
            if (!it.exiting) continue;

            auto* anim = it.go->GetComponent<game::UIToastAnimator>();
            if (!anim || anim->IsFinished())
            {
                it.go->Destroy();
                it.go = nullptr;
            }
        }

        while (!q.empty() && !q.front().go)
            q.pop_front();

        ReflowVisible(q, cfg, false);
    }

    void UIMessageQueue::TrySpawnNextBatchIfEmpty(std::deque<Item>& q, const ChannelConfig& cfg)
    {
        int visibleAlive = 0;
        for (auto& it : q)
            if (it.visible && it.go) visibleAlive++;

        if (visibleAlive > 0) return;

        int spawned = 0;
        for (auto& it : q)
        {
            if (spawned >= cfg.maxVisible) break;
            if (!it.go) continue;
            if (it.visible) continue; // hidden만

            it.visible = true;
            it.exiting = false;
            it.born = engine::Time::GetTimestamp();

            it.go->SetActive(true);

            if (auto* anim = it.go->GetComponent<game::UIToastAnimator>())
            {
                const engine::Vector2 target = CalcTargetPos(cfg, (size_t)spawned);
                anim->PlayEnter(target);
            }
            spawned++;
        }

        ReflowVisible(q, cfg, false);
    }

    bool UIMessageQueue::HasHiddenItems(const std::deque<Item>& q) const
    {
        for (auto& it : q)
            if (it.go && !it.visible) return true;
        return false;
    }

    void UIMessageQueue::ReflowVisible(std::deque<Item>& q, const ChannelConfig& cfg, bool instant)
    {
        int idx = 0;
        for (auto& it : q)
        {
            if (!it.go || !it.visible) continue;

            auto* rt = it.go->GetComponent<engine::RectTransform>();
            auto* anim = it.go->GetComponent<game::UIToastAnimator>();
            if (!rt || !anim) continue;

            engine::Vector2 target = CalcTargetPos(cfg, (size_t)idx);

            if (instant) rt->SetAnchoredPosition(target);
            else         anim->MoveTo(target);

            idx++;
        }
    }

    engine::Vector2 UIMessageQueue::CalcTargetPos(const ChannelConfig& cfg, size_t visibleIndex) const
    {
        return { cfg.spawnPos.x, cfg.spawnPos.y + (float)visibleIndex * cfg.spacing };
    }

    void UIMessageQueue::UpdateChannel(std::deque<Item>& q, ChannelConfig& cfg)
    {
        TryStartExitVisibleBatch(q, cfg);
        CleanupFinishedVisible(q, cfg);
        TrySpawnNextBatchIfEmpty(q, cfg);
    }
}