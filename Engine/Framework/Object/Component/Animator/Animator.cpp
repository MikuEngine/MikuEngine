#include "EnginePCH.h"
#include "Animator.h"

#include "Framework/System/SystemManager.h"
#include "Framework/System/AnimatorSystem.h"

namespace engine
{
    Animator::~Animator()
    {
        SystemManager::Get().GetAnimatorSystem().Unregister(this);
    }

    void Animator::Initialize()
    {
        SystemManager::Get().GetAnimatorSystem().Register(this);
    }
}