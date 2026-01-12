#include "EnginePCH.h"
#include "Script.h"

#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"
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

    GameObject* ScriptBase::CreateGameObject(const std::string& name)
    {
        return SceneManager::Get().GetScene()->CreateGameObject(name);
    }
}