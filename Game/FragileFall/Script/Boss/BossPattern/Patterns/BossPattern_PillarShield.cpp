#include "GamePCH.h"
#include "BossPattern_PillarShield.h"
#include "../../BossScript.h"
#include "../Components/BossPillar.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/Component/Transform.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 패턴 실행
    // ═══════════════════════════════════════════════════════════════
    void BossPattern_PillarShield::Start(BossScript* boss)
    {
        if (!boss) return;

        m_isActive = true;
        m_intervalTimer = 0.0f;

        // 시작 시 즉시 기둥 생성
        SpawnPillars(boss);
    }

    void BossPattern_PillarShield::Update(BossScript* boss, float deltaTime)
    {
        if (!boss || !m_isActive) return;

        // 간격 타이머 업데이트
        m_intervalTimer += deltaTime;

        // 간격이 지나면 기둥 재생성
        if (m_intervalTimer >= m_interval)
        {
            // 기존 기둥들 정리
            CheckPillarsStatus(boss);

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

    // ═══════════════════════════════════════════════════════════════
    // 기둥 생성
    // ═══════════════════════════════════════════════════════════════
    void BossPattern_PillarShield::SpawnPillars(BossScript* boss)
    {
        if (!boss) return;

        auto* scene = engine::SceneManager::Get().GetScene();
        if (!scene) return;

        auto* bossTransform = boss->GetGameObject()->GetTransform();
        if (!bossTransform) return;

        engine::Vector3 bossPos = bossTransform->GetWorldPosition();

        // 기존 기둥들 정리
        for (const auto& pillar : m_spawnedPillars)
        {
            if (pillar && pillar->GetGameObject())
            {
                // TODO: GameObject 파괴 또는 비활성화
                // pillar->GetGameObject()->Destroy();
            }
        }
        m_spawnedPillars.clear();

        // 새 기둥들 생성
        for (int i = 0; i < m_pillarCount; ++i)
        {
            float angle = static_cast<float>(i) * (360.0f / m_pillarCount) * 3.14159f / 180.0f;
            engine::Vector3 offset(
                cosf(angle) * m_pillarSpawnRadius,
                0.0f,
                sinf(angle) * m_pillarSpawnRadius
            );

            engine::Vector3 pillarPos = bossPos + offset;

            // 기둥 GameObject 생성
            std::string pillarName = "BossPillar_" + std::to_string(i);
            auto* pillarGO = scene->CreateGameObject(pillarName);
            if (!pillarGO) continue;

            // Transform 설정
            auto* pillarTransform = pillarGO->GetTransform();
            if (pillarTransform)
            {
                pillarTransform->SetLocalPosition(pillarPos);
            }

            // BossPillar 스크립트 추가
            auto* pillarScript = pillarGO->AddComponent<BossPillar>();
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

    // ═══════════════════════════════════════════════════════════════
    // 기둥 상태 체크
    // ═══════════════════════════════════════════════════════════════
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
