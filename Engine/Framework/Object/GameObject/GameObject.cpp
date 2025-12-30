#include "pch.h"
#include "GameObject.h"

#include "Common/Utility/JsonHelper.h"
#include "Common/Utility/StaticMemoryPool.h"
#include "Framework/Object/Component/Component.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/Component/ComponentFactory.h"
#include "Framework/Scene/Scene.h"

namespace engine
{
    namespace
    {
        StaticMemoryPool<GameObject, 4096> g_gameObjectPool;
    }

    GameObject::GameObject()
    {
        m_components.reserve(8);

        m_transform = AddComponent<Transform>();
    }

    void* GameObject::operator new(size_t size)
    {
        return g_gameObjectPool.Allocate(size);
    }

    void GameObject::operator delete(void* ptr)
    {
        g_gameObjectPool.Deallocate(ptr);
    }

    Transform* GameObject::GetTransform() const
    {
        return m_transform;
    }

    const std::string& GameObject::GetName() const
    {
        return m_name;
    }

    void GameObject::SetName(const std::string& name)
    {
        m_name = name;
    }

    void GameObject::Destroy()
    {
        if (m_isPendingKill)
        {
            return;
        }

        for (auto childTr : m_transform->GetChildren())
        {
            childTr->GetGameObject()->Destroy();
        }

        m_isPendingKill = true;

        SceneManager::Get().GetScene()->RegisterPendingKill(this);
    }

    bool GameObject::IsPendingKill() const
    {
        return m_isPendingKill;
    }

    void GameObject::RemoveComponentFast(Component* component)
    {
        if (component == nullptr)
        {
            return;
        }

        std::int32_t index = component->m_gameObjectIndex;
        std::int32_t lastIndex = static_cast<std::int32_t>(m_components.size() - 1);

        if (index < 0 || index >= m_components.size() || m_components[index].get() != component)
        {
            return;
        }

        if (index != lastIndex)
        {
            m_components[index] = std::move(m_components[lastIndex]);
            m_components[index]->m_gameObjectIndex = index;
        }

        m_components.pop_back();
    }

    void GameObject::BroadcastOnDestroy()
    {
        for (auto& comp : m_components)
        {
            comp->OnDestroy();
        }
    }

    Component* GameObject::AddComponent(std::unique_ptr<Component>&& component)
    {
        Component* ptr = component.get();
        m_components.push_back(std::move(component));

        ptr->m_owner = this;
        ptr->m_gameObjectIndex = static_cast<std::int32_t>(m_components.size() - 1);

        SceneManager::Get().GetScene()->RegisterPendingAdd(ptr);


        return ptr;
    }

    const std::vector<std::unique_ptr<Component>>& GameObject::GetComponents() const
    {
        return m_components;
    }

    void GameObject::RemoveComponent(size_t index)
    {
        if (index >= m_components.size())
        {
            return;
        }

        // 인덱스로 지움 OnDestroy 호출 안함

        m_components.erase(m_components.begin() + index);

        for (size_t i = index; i < m_components.size(); ++i)
        {
            m_components[i]->m_gameObjectIndex = static_cast<int32_t>(i);
        }
    }

    void GameObject::Save(json& j) const
    {
        j["Name"] = m_name;
        j["Components"] = json::array();
        
        for (const auto& comp : m_components)
        {
            json compJson;
            comp->Save(compJson);

            if (!compJson.empty())
            {
                j["Components"].push_back(compJson);
            }
        }
    }

    void GameObject::Load(const json& j)
    {
        JsonGet(j, "Name", m_name);

        JsonArrayForEach(j, "Components", [&](const json& compJson)
            {
                std::string type = compJson.value("Type", "");

                if (type.empty())
                {
                    return;
                }

                if (type == "Transform")
                {
                    GetTransform()->Load(compJson);
                }
                else
                {
                    auto newComp = ComponentFactory::Get().Create(type);
                    if (newComp)
                    {
                        newComp->Load(compJson);
                        AddComponent(std::move(newComp));
                    }
                }
            });
    }

    std::string GameObject::GetType() const
    {
        return "GameObject";
    }
}