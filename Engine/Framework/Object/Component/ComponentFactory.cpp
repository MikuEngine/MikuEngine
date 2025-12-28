#include "pch.h"
#include "ComponentFactory.h"

#include "Framework/Object/Component/Component.h"

namespace engine
{
    void ComponentFactory::Register(const std::string& name, Creator creator)
    {
        m_registry[name] = creator;
    }

    std::unique_ptr<Component> ComponentFactory::Create(const std::string& name)
    {
        if (auto iter = m_registry.find(name); iter != m_registry.end())
        {
            return std::invoke(iter->second);
        }

        return nullptr;
    }

    const std::map<std::string, ComponentFactory::Creator>& ComponentFactory::GetRegistry() const
    {
        return m_registry;
    }
}