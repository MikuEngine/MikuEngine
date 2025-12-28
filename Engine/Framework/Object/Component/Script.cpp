#include "pch.h"
#include "Script.h"

#include "Framework/System/SystemManager.h"
#include "Framework/System/ScriptSystem.h"

namespace engine
{
    ScriptBase::ScriptBase()
    {
        m_systemIndices.fill(-1);
    }

    ScriptBase::~ScriptBase()
    {
        SystemManager::Get().GetScriptSystem().Unregister(this);
    }

    void ScriptBase::RegisterScript(std::uint32_t eventFlags)
    {
        SystemManager::Get().GetScriptSystem().Register(this, eventFlags);
    }

    void ScriptBase::Initialize()
    {
        // game 프로젝트에서 구현
    }

    void ScriptBase::Start()
    {
        // game 프로젝트에서 구현
    }

    void ScriptBase::Update()
    {
        // game 프로젝트에서 구현
    }
}