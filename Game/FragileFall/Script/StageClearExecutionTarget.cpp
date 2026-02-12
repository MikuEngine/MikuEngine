#include "GamePCH.h"
#include "StageClearExecutionTarget.h"

#include "Manager/StageManager.h"

namespace game
{
    void StageClearExecutionTarget::Execute()
    {
        if (m_executed)
        {
            return;
        }

        m_executed = true;
        StageManager::Get().OnStageClearExecutionTargetExecuted();

        if (GetGameObject())
        {
            GetGameObject()->Destroy();
        }
    }
}
