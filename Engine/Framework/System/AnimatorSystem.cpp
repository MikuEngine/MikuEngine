#include "EnginePCH.h"
#include "AnimatorSystem.h"

namespace engine
{
    void AnimatorSystem::Update()
    {
        for (auto animator : m_components)
        {
            if (animator->IsActive())
            {
                animator->Update();
            }
        }
    }
}