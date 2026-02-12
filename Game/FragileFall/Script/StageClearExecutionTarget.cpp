#include "GamePCH.h"
#include "StageClearExecutionTarget.h"

#include <Framework/Asset/Prefab.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/System/SoundSystem.h>

#include "Manager/StageManager.h"
#include "Script/CrystalIceFillControllerScript.h"

namespace game
{
    namespace
    {
        CrystalIceFillControllerScript* FindIceFillInChildren(engine::Transform* root)
        {
            if (!root)
                return nullptr;

            for (engine::Transform* child : root->GetChildren())
            {
                if (!child) continue;
                engine::GameObject* go = child->GetGameObject();
                if (!go) continue;

                if (auto* fill = go->GetComponent<CrystalIceFillControllerScript>())
                    return fill;

                if (auto* nested = FindIceFillInChildren(child))
                    return nested;
            }
            return nullptr;
        }

        std::string GetTargetColorFromName(const std::string& targetName)
        {
            if (targetName.find("_Gray") != std::string::npos) return "Gray";
            if (targetName.find("_Blue") != std::string::npos) return "Blue";
            if (targetName.find("_Red") != std::string::npos) return "Red";
            if (targetName.find("_Green") != std::string::npos) return "Green";
            if (targetName.find("_Purple") != std::string::npos) return "Purple";
            if (targetName.find("_White") != std::string::npos) return "White";
            return "";
        }
    }

    void StageClearExecutionTarget::Start()
    {
        // 프리팹 내부 CrystalIceFillControllerScript를 찾아 StageClear용 속도로 맞춘다.
        // (사용자 요청: 프리팹 인스턴스 후 내부 스크립트 값을 직접 조절)
        if (auto* fill = FindIceFillInChildren(GetTransform()))
        {
            fill->SetDuration(0.4f);
            fill->SetStepCount(20);
        }

        engine::SoundSystem::Get().Play("Stage_Clear", "SFX/Monster");
    }

    void StageClearExecutionTarget::Execute()
    {
        if (m_executed)
        {
            return;
        }

        // StageClearExecutionTarget 프리팹 이름 suffix(_Gray/_Red/...) 기준으로 분기
        std::string colorSuffix;
        if (GetGameObject())
        {
            colorSuffix = GetTargetColorFromName(GetGameObject()->GetName());
        }

        // suffix가 없으면 맵 프리팹 기반 fallback
        if (colorSuffix.empty())
        {
            colorSuffix = StageManager::GetMapColorFromPrefabName(
                StageManager::Get().GetCurrentMapPrefabName());
        }

        // Effect 프리팹은 white만 소문자 네이밍
        if (colorSuffix == "White")
            colorSuffix = "white";

        std::string effectPrefab = "Effect_Break_V1.01_big_" + colorSuffix;
        auto effect = engine::Prefab::Instantiate(effectPrefab);
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
