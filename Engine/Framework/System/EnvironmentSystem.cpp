#include "EnginePCH.h"
#include "EnvironmentSystem.h"

namespace engine
{
    EnvironmentSettings* EnvironmentSystem::GetEnvironmentSettings() const
    {
        for (auto* settings : m_components)
        {
            if (settings->IsActive())
            {
                return settings;
            }
        }
        return nullptr;
    }
}
