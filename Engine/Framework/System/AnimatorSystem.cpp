#include "pch.h"
#include "AnimatorSystem.h"

namespace engine
{
    void AnimatorSystem::Update()
    {
        for (auto animator : m_components)
        {
            animator->Update();
        }
    }
}