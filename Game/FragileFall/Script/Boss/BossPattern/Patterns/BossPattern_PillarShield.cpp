#include "GamePCH.h"
#include "BossPattern_PillarShield.h"

#include <Framework/Object/Component/Transform.h>
#include <Framework/Asset/Prefab.h>

#include "Script/Boss/BossScript.h"
#include "Script/Boss/BossPattern/Components/BossPillar.h"

namespace game
{
    namespace
    {
        constexpr engine::Vector3 g_twoPillarPositions[2]{
            engine::Vector3(-4.0f, 0.0f, 0.0f),
            engine::Vector3(4.0f, 0.0f, 0.0f)
        };
    }

    void BossPattern_PillarShield::Start(BossScript* boss)
    {
        if (!boss)
        {
            return;
        }

        m_isActive = true;
        m_intervalTimer = 0.0f;

        // 시작 시 즉시 기둥 생성
        SpawnPillars(boss);
    }

    void BossPattern_PillarShield::Update(BossScript* boss, float deltaTime)
    {
        if (!boss || !m_isActive) return;

        if (m_spawnedPillars.empty())
        {
            m_intervalTimer += deltaTime;
        }

        // 간격이 지나면 기둥 재생성
        if (m_intervalTimer >= m_interval)
        {
            // 새 기둥 생성
            SpawnPillars(boss);
            m_intervalTimer = 0.0f;
        }
        else
        {
            // 기둥 상태 체크
            CheckPillarsStatus(boss);
        }
    }

    void BossPattern_PillarShield::End(BossScript* boss)
    {
        m_isActive = false;
        m_intervalTimer = 0.0f;
    }

    void BossPattern_PillarShield::SpawnPillars(BossScript* boss)
    {
        if (!boss)
        {
            return;
        }

        // 기존 기둥들 정리
        for (const auto& pillar : m_spawnedPillars)
        {
            if (pillar && pillar->GetGameObject())
            {
                pillar->GetGameObject()->Destroy();
            }
        }
        m_spawnedPillars.clear();

        // 새 기둥들 생성
        for (int i = 0; i < m_pillarCount; ++i)
        {
            engine::Vector3 pillarPos;
            if (m_pillarCount == 2)
            {
                pillarPos = g_twoPillarPositions[i];
            }

            // 기둥 GameObject 생성
            std::string pillarName = "BossPillar_" + std::to_string(i);
            auto go = engine::Prefab::Instantiate("BossPillar");

            go->GetTransform()->SetLocalPosition(pillarPos);

            auto pillarScript = go->GetComponent<BossPillar>();
            if (pillarScript)
            {
                engine::Ptr<BossPillar> pillarPtr(pillarScript);
                engine::Ptr<BossScript> bossPtr(boss);

                pillarScript->SetBoss(bossPtr);
                pillarScript->SetHP(100);  // TODO: 설정 가능하도록 변경

                m_spawnedPillars.push_back(pillarPtr);
                boss->OnPillarCreated(pillarPtr);
            }
        }
    }

    void BossPattern_PillarShield::CheckPillarsStatus(BossScript* boss)
    {
        if (!boss) return;

        // 파괴된 기둥 제거
        m_spawnedPillars.erase(
            std::remove_if(m_spawnedPillars.begin(), m_spawnedPillars.end(),
                [](const engine::Ptr<BossPillar>& pillar) {
                    if (!pillar) return true;
                    return pillar->IsDestroyed();
                }),
            m_spawnedPillars.end()
        );
    }
}
