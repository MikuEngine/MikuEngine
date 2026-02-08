#include "GamePCH.h"
#include "BossPillar.h"

#include <Framework/Asset/Prefab.h>

#include "Script/Boss/BossScript.h"

namespace game
{
    void BossPillar::Update()
    {
        if (m_hp <= 0 && !m_isCrystalized)
        {
            m_isCrystalized = true;

            OnCrystalized();
        }
    }

    void BossPillar::TakeDamage(float damage, bool isShieldPierce)
    {
        m_hp -= damage;
        if (m_hp < 0)
        {
            m_hp = 0;
        }
    }

    void BossPillar::Execute()
    {
        for (auto& e : m_pillarCrystalizedPieces)
        {
            e->Destroy();
        }

        // 보스에 기둥 파괴 알림
        if (m_boss)
        {
            m_boss->OnPillarDestroyed(engine::Ptr<BossPillar>(this));

            GetGameObject()->Destroy();
        }
    }

    bool BossPillar::IsCrystalized()
    {
        return m_isCrystalized;
    }

    void BossPillar::OnCrystalized()
    {
        for (int i = 0; i < 10; ++i)
        {
            auto go = engine::Prefab::Instantiate("PillarCrystalizedPiece");

            auto pos = GetTransform()->GetLocalPosition();

            pos.y += engine::Random::Float(-2.0f, 2.0f);

            float rotY = engine::Random::Float(0.0f, 360.f);
            float rotX = engine::Random::Float(30.0f, 60.0f);

            go->GetTransform()->SetLocalPosition(pos);
            go->GetTransform()->SetLocalRotation(engine::Vector3(rotX, rotY, 0.0f));

            m_pillarCrystalizedPieces.push_back(go);
        }
    }
}
