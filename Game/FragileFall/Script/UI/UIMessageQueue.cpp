#include "GamePCH.h"
#include "UIMessageQueue.h"
#include <Framework/Asset/Prefab.h>
#include <Framework/Object/Component/RectTransform.h>

namespace game
{
    void UIMessageQueue::Awake()
    {
        m_canvas = engine::GameObject::Find("Canvas_Message");
    }

    void UIMessageQueue::Start()
    {

    }

    void UIMessageQueue::Update()
    {
        if (!m_canvas) return;

        // 메시지 흐름
        // if문으로 제어
        if (false)
        {
            PushMessage("ㅎㅇ");
        }

        TryStartExitVisibleBatch();
        CleanupFinishedVisible();
        TrySpawnNextBatchIfEmpty();
    }

    void UIMessageQueue::OnGui()
    {
        ImGui::DragFloat("LifeTime", &m_lifeTime, 0.01f, 0.0f, 30.0f);
        ImGui::DragFloat("Spacing", &m_spacing, 1.0f, 0.0f, 1000.0f);
        ImGui::DragFloat2("SpawnPos", &m_spawnPos.x, 1.0f);
        ImGui::DragInt("MaxVisible", &m_maxVisible);
    }

    void UIMessageQueue::Save(engine::json& j) const
    {
        Object::Save(j);
        j["LifeTime"] = m_lifeTime;
        j["Spacing"] = m_spacing;
        j["SpawnPos"] = m_spawnPos;
        j["MaxVisible"] = m_maxVisible;
        j["PrefabKey"] = m_prefabKey;
    }

    void UIMessageQueue::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "LifeTime", m_lifeTime);
        engine::JsonGet(j, "Spacing", m_spacing);
        engine::JsonGet(j, "SpawnPos", m_spawnPos);
        engine::JsonGet(j, "MaxVisible", m_maxVisible);
        engine::JsonGet(j, "PrefabKey", m_prefabKey);
    }

    void UIMessageQueue::PushMessage(const std::string& text, const std::string& iconKey)
    {
        if (!m_canvas) return;

        int visibleCount = 0;
        for (auto& it : m_items)
            if (it.visible && it.go) visibleCount++;

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

        for (auto& it : m_items)
            if (it.visible && it.go) visibleCount++;

        Item item;
        item.go = go;
        item.born = engine::Time::GetTimestamp();
        item.exiting = false;

        if (visibleCount >= m_maxVisible)
        {
            item.visible = false;
            go->SetActive(false);
            m_items.push_back(item);
            return;
        }

        item.visible = true;
        m_items.push_back(item);

        ReflowVisible(false);

        const engine::Vector2 target = CalcTargetPos(m_items.size() - 1);
        anim->PlayEnter(target);
    }

    void UIMessageQueue::TryStartExitVisibleBatch()
    {

    }

    void UIMessageQueue::CleanupFinishedVisible()
    {

    }

    void UIMessageQueue::TrySpawnNextBatchIfEmpty()
    {

    }

    bool UIMessageQueue::HasHiddenItems() const
    {
        return false;
    }

    void UIMessageQueue::ReflowVisible(bool instant)
    {

    }

    engine::Vector2 UIMessageQueue::CalcTargetPos(size_t visibleIndex) const
    {
        return engine::Vector2();
    }
}