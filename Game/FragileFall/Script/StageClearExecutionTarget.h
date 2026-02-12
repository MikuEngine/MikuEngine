#pragma once

#include <Framework/Object/Component/Script.h>

namespace game
{
    class StageClearExecutionTarget :
        public engine::Script<StageClearExecutionTarget>
    {
        REGISTER_SCRIPT(StageClearExecutionTarget, Script)

    private:
        bool m_executed = false;

    public:
        bool IsExecutable() const { return !m_executed; }
        void Execute();
    };
}
