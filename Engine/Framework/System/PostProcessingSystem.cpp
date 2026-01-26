#include "EnginePCH.h"
#include "PostProcessingSystem.h"

namespace engine
{
    PostProcessingSettings* PostProcessingSystem::GetPostProcessingSettings() const
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
