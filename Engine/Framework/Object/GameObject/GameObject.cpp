#include "pch.h"
#include "GameObject.h"

#include "Common/Utility/JsonHelper.h"
#include "Framework/Object/Component/Component.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/Component/ComponentFactory.h"

namespace engine
{
    GameObject::GameObject(const std::string& name)
        : m_name{ name }
    {
        m_transform = AddComponent<Transform>();
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

    Component* GameObject::AddComponent(std::unique_ptr<Component>&& component)
    {
        Component* ptr = component.get();
        component->m_owner = this;
        m_components.push_back(std::move(component));

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

        m_components.erase(m_components.begin() + index);
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