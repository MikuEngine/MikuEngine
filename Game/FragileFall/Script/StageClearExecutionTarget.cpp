#include "GamePCH.h"
#include "StageClearExecutionTarget.h"

#include <Framework/Asset/Prefab.h>
#include <Framework/System/SoundSystem.h>

#include "Manager/StageManager.h"

namespace game
{
    void StageClearExecutionTarget::Execute()
    {
        if (m_executed)
        {
            return;
        }

        auto effect = engine::Prefab::Instantiate("Effect_Break_V1.01_big_white");
        if (effect && effect->GetTransform())
        {
            effect->GetTransform()->SetWorldMatrix(GetTransform()->GetWorld());
            effect->GetTransform()->SetLocalScale(engine::Vector3(1.2f, 1.2f, 1.2f));
        }
        engine::SoundSystem::Get().Play("Player_Break_Random", "SFX/Player");

        m_executed = true;
        StageManager::Get().OnStageClearExecutionTargetExecuted();

        if (GetGameObject())
        {
            GetGameObject()->Destroy();
        }
    }
}
