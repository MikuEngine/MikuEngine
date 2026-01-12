#include "EnginePCH.h"
#include "ScriptSystem.h"

namespace engine
{
    void ScriptSystem::Register(ScriptBase* script, std::uint32_t eventFlags)
    {
        if (eventFlags & (1U << static_cast<int>(ScriptEvent::Start)))
        {
            AddScript(m_startScripts, script, ScriptEvent::Start);
        }

        if (eventFlags & (1U << static_cast<int>(ScriptEvent::Update)))
        {
            AddScript(m_updateScripts, script, ScriptEvent::Update);
        }
    }

    void ScriptSystem::Unregister(ScriptBase* script)
    {
        RemoveScript(m_startScripts, script, ScriptEvent::Start);
        RemoveScript(m_updateScripts, script, ScriptEvent::Update);
    }

    void ScriptSystem::CallStart()
    {
        for (size_t i = 0; i < m_startScripts.size(); )
        {
            auto script = m_startScripts[i];

            if (script->IsActive())
            {
                script->Start();

                RemoveScript(m_startScripts, script, ScriptEvent::Start);
            }
            else
            {
                ++i;
            }
        }
    }

    void ScriptSystem::CallUpdate()
    {
        for (auto& script : m_updateScripts)
        {
            if (!script->IsActive())
            {
                continue;
            }

            if (script->m_systemIndices[static_cast<size_t>(ScriptEvent::Start)] != -1)
            {
                continue;
            }

            script->Update();
        }
    }

    void ScriptSystem::AddScript(std::vector<ScriptBase*>& v, ScriptBase* script, ScriptEvent type)
    {
        if (script->m_systemIndices[static_cast<size_t>(type)] != -1)
        {
            return; // 이미 등록됨
        }

        script->m_systemIndices[static_cast<size_t>(type)] = static_cast<std::int32_t>(v.size());
        v.push_back(script);
    }

    void ScriptSystem::RemoveScript(std::vector<ScriptBase*>& v, ScriptBase* script, ScriptEvent type)
    {
        if (v.empty())
        {
            return;
        }

        std::int32_t index = script->m_systemIndices[static_cast<size_t>(type)];
        if (index < 0)
        {
            return; // 등록 안됨
        }
        
        ScriptBase* back = v.back();
        v[index] = back;
        v.pop_back();
        
        back->m_systemIndices[static_cast<size_t>(type)] = index;
        script->m_systemIndices[static_cast<size_t>(type)] = -1;
    }
}