#include "EnginePCH.h"
#include "ComponentFactory.h"

#include "Framework/Object/Component/Component.h"

namespace engine
{
    void ComponentFactory::Register(const std::string& name, Creator creator)
    {
        m_registry[name] = creator;
    }

    void ComponentFactory::RegisterScript(const std::string& name, Creator creator)
    {
        Register(name, std::move(creator));
        m_scriptNames.insert(name);
    }

    bool ComponentFactory::IsScript(const std::string& name) const
    {
        return m_scriptNames.find(name) != m_scriptNames.end();
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