#include "GamePCH.h"
#include "UIKillPopupQueue.h"

#include <Framework/Scene/Scene.h>

#include <Framework/Asset/Prefab.h>

#include <Framework/Object/GameObject/GameObject.h>

#include <Framework/Object/Component/UI/UIPanel.h>
#include <Framework/Object/Component/UI/UIImage.h>

#include <Framework/Object/Component/RectTransform.h>

namespace game
{
    static engine::Vector2 Lerp(const engine::Vector2& a, const engine::Vector2& b, float t)
    {
        return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
    }

    void UIKillPopupQueue::Awake()
    {
        m_canvas = engine::GameObject::Find("Canvas_KillPopUp");
        if (!m_canvas) return;
    }

    void UIKillPopupQueue::Start()
    {

    }

    void UIKillPopupQueue::Update()
    {
        if (!m_canvas) return;

        if (engine::Input::IsKeyPressed(engine::Keys::K))
        {
            PushKill("토가 17세 사인: 결정화", "");
        }

        TryStartExitTop();
        CleanupTopIfFinished();
        TryUnlockAndShow();
    }

    void UIKillPopupQueue::OnGui()
    {
        ImGui::DragFloat("LifeTime", &m_lifeTime);
        ImGui::DragFloat("Spacing", &m_spacing);
        ImGui::DragFloat2("Origin Offset", &m_spawnPos.x);
        ImGui::DragInt("MaxQueue", &m_maxQueue);
    }

    void UIKillPopupQueue::Save(engine::json& j) const
    {
        Object::Save(j);
        j["LifeTime"] = m_lifeTime;
        j["Spacing"] = m_spacing;
        j["SpawnPos"] = m_spawnPos;
        j["MaxQueue"] = m_maxQueue;
    }

    void UIKillPopupQueue::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "LifeTime", m_lifeTime);
        engine::JsonGet(j, "Spacing", m_spacing);
        engine::JsonGet(j, "SpawnPos", m_spawnPos);
        engine::JsonGet(j, "MaxQueue", m_maxQueue);
    }

    void UIKillPopupQueue::PushKill(const std::string& text, const std::string& iconKey)
    {
        if (!m_canvas) return;

        int visibleCount = 0;
        for (auto& it : m_items)
            if (it.visible && it.go) visibleCount++;

        const bool willOverflow = (visibleCount >= m_maxQueue);

        // 프리팹 생성
        engine::Ptr<engine::GameObject> go = engine::Prefab::Instantiate(m_prefabKey);
        if (!go) return;

        // UI는 캔버스의 자식으로 추가
        go->GetTransform()->SetParent(m_canvas->GetTransform());

        auto* rt = go->GetComponent<engine::RectTransform>();
        auto* anim = go->GetComponent<game::UIToastAnimator>();

        if (!rt || !anim)
        {
            go->Destroy();
            return;
        }

        anim->SetText(text);

        Item item;
        item.go = go;
        item.born = engine::Time::GetTimestamp();
        item.exiting = false;

        if (m_locked || willOverflow)
        {
            m_locked = true;
            item.visible = false;
            go->SetActive(false);
            m_items.push_back(item);
            return;
        }

        item.visible = true;
        m_items.push_back(item);

        Reflow(false);

        const engine::Vector2 target = CalcTargetPos(m_items.size() - 1);
        anim->PlayEnter(target);
    }

    void UIKillPopupQueue::Reflow(bool instant)
    {
        int idx = 0;
        for (auto& it : m_items)
        {
            if (!it.go || !it.visible) continue;

            auto* rt = it.go->GetComponent<engine::RectTransform>();
            auto* anim = it.go->GetComponent<game::UIToastAnimator>();
            if (!rt || !anim) continue;

            engine::Vector2 target = CalcTargetPos((size_t)idx);

            if (instant) rt->SetAnchoredPosition(target);
            else         anim->MoveTo(target);

            idx++;
        }
    }

    engine::Vector2 UIKillPopupQueue::CalcTargetPos(size_t index) const
    {
        return { m_spawnPos.x, m_spawnPos.y + (float)index * m_spacing };
    }

    void UIKillPopupQueue::TryStartExitTop()
    {
        while (!m_items.empty() && !m_items.front().go)
            m_items.pop_front();

        if (m_items.empty()) return;

        Item& top = m_items.front();
        if (!top.go) return;

        auto* anim = top.go->GetComponent<game::UIToastAnimator>();
        if (!anim)
        {
            top.go->Destroy();
            m_items.pop_front();
            Reflow(false);
            return;
        }

        if (!top.exiting)
        {
            const float age = engine::Time::GetElapsedSeconds(top.born);
            if (age >= m_lifeTime)
            {
                top.exiting = true;
                anim->FadeOut();
            }
        }
    }

    void UIKillPopupQueue::TryUnlockAndShow()
    {
        // 화면에 보이는 게 아직 있으면 아무것도 하지 않음
        int visibleAlive = 0;
        for (auto& it : m_items)
            if (it.visible && it.go) visibleAlive++;

        if (visibleAlive > 0) return;

        // 화면이 비었으니, 다음 배치를 꺼낸다(최대 m_maxQueue개)
        int spawned = 0;
        for (size_t i = 0; i < m_items.size() && spawned < m_maxQueue; ++i)
        {
            auto& it = m_items[i];
            if (!it.go) continue;
            if (it.visible) continue; // 숨겨둔 것만

            it.visible = true;
            it.exiting = false;
            it.born = engine::Time::GetTimestamp();

            it.go->SetActive(true);

            if (auto* anim = it.go->GetComponent<game::UIToastAnimator>())
            {
                const engine::Vector2 target = CalcTargetPos((size_t)spawned);
                anim->PlayEnter(target);
            }

            spawned++;
        }

        // 더 남아있으면 잠금 유지, 다 꺼냈으면 잠금 해제
        m_locked = HasHiddenItems();
    }

    void UIKillPopupQueue::CleanupTopIfFinished()
    {
        while (!m_items.empty() && !m_items.front().go)
            m_items.pop_front();

        if (m_items.empty())
            return;

        Item& top = m_items.front();
        if (!top.go)
            return;

        if (!top.exiting)
            return;

        auto* anim = top.go->GetComponent<game::UIToastAnimator>();
        if (!anim)
        {
            top.go->Destroy();
            m_items.pop_front();
            Reflow(false);
            return;
        }

        if (anim->IsFinished())
        {
            top.go->Destroy();
            m_items.pop_front();
            Reflow(false);
        }
    }
    bool UIKillPopupQueue::HasHiddenItems()
    {
        for (auto& it : m_items)
            if (it.go && !it.visible) return true;
        return false;
    }
}