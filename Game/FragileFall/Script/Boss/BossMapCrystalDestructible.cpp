#include "GamePCH.h"
#include "BossMapCrystalDestructible.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/System/SoundSystem.h>

#include "BossScript.h"

namespace game
{
    void BossMapCrystalDestructible::Start()
    {
        auto* bossGO = engine::GameObject::Find("z_Dev_Boss");
        if (!bossGO)
            return;

        auto* boss = bossGO->GetComponent<BossScript>();
        if (!boss)
            return;

        boss->RegisterMapCrystalForDeath(GetGameObject());
        m_registeredBoss = boss;
    }

    void BossMapCrystalDestructible::OnDestroy()
    {
        if (m_registeredBoss)
        {
            m_registeredBoss->UnregisterMapCrystalForDeath(GetGameObject());
            m_registeredBoss = nullptr;
        }
    }
}
