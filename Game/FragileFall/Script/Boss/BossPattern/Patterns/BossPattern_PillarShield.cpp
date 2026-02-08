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
        // 베이직 패턴 기둥 위치 (좌우 고정)
        constexpr engine::Vector3 g_basicPillarPositions[2]{
            engine::Vector3(-4.0f, 1.0f, 0.0f),
            engine::Vector3(4.0f, 1.0f, 0.0f)
        };
    }

    // ═══════════════════════════════════════════════════════════════
    // 패턴 생명주기
    // ═══════════════════════════════════════════════════════════════
    void BossPattern_PillarShield::Start(BossScript* boss)
    {
        if (!boss) return;

        m_isActive = true;
        m_respawnTimer = 0.0f;

        // 시작 시 항상 좌우 2개 고정 (Use Basic 무시)
        SpawnBasicPillars(boss);
    }

    void BossPattern_PillarShield::Update(BossScript* boss, float deltaTime)
    {
        if (!boss || !m_isActive) return;

        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // 파괴된 기둥 정리 (매 프레임)
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        m_spawnedPillars.erase(
            std::remove_if(m_spawnedPillars.begin(), m_spawnedPillars.end(),
                [](const engine::Ptr<BossPillar>& pillar) {
                    // null이거나 GameObject가 파괴되었으면 제거
                    if (!pillar) return true;
                    if (!pillar->GetGameObject()) return true;
                    return false;
                }),
            m_spawnedPillars.end()
        );

        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // 모든 기둥이 파괴된 경우
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        if (m_spawnedPillars.empty())
        {
            // 대기 타이머 증가
            m_respawnTimer += deltaTime;

            // 쉴드 이펙트 제거 (1회만)
            if (!m_shieldEffects.empty())
            {
                for (auto e : m_shieldEffects)
                {
                    if (e)
                    {
                        e->Destroy();
                    }
                }
                m_shieldEffects.clear();
            }

            // 대기 시간 경과 체크
            if (m_respawnTimer >= boss->GetPillarRespawnDelay())
            {
                // Use Basic 패턴 체크
                if (boss->GetPillarUseBasicPattern())
                {
                    SpawnBasicPillars(boss);  // 좌우 2개 고정
                }
                else
                {
                    SpawnRandomPillars(boss);  // 랜덤 + 밀집도
                }

                m_respawnTimer = 0.0f;
            }
        }
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        // 기둥이 남아있는 경우
        // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
        else
        {
            // 아무것도 안 함 (타이머 증가 X)
        }
    }

    void BossPattern_PillarShield::End(BossScript* boss)
    {
        m_isActive = false;
        m_respawnTimer = 0.0f;

        // 모든 기둥 정리
        for (const auto& pillar : m_spawnedPillars)
        {
            if (pillar && pillar->GetGameObject())
            {
                pillar->GetGameObject()->Destroy();
            }
        }
        m_spawnedPillars.clear();

        // 쉴드 이펙트 정리
        for (auto e : m_shieldEffects)
        {
            if (e)
            {
                e->Destroy();
            }
        }
        m_shieldEffects.clear();
    }

    // ═══════════════════════════════════════════════════════════════
    // 기둥 생성 (베이직 패턴)
    // ═══════════════════════════════════════════════════════════════
    void BossPattern_PillarShield::SpawnBasicPillars(BossScript* boss)
    {
        if (!boss) return;

        // 좌우 2개 고정 위치
        for (int i = 0; i < 2; ++i)
        {
            engine::Vector3 pillarPos = g_basicPillarPositions[i];

            // 프리팹 생성
            auto go = engine::Prefab::Instantiate("BossPillar");
            if (!go) continue;

            go->GetTransform()->SetLocalPosition(pillarPos);

            // BossPillar 스크립트 설정
            auto pillarScript = go->GetComponent<BossPillar>();
            if (pillarScript)
            {
                engine::Ptr<BossPillar> pillarPtr(pillarScript);
                engine::Ptr<BossScript> bossPtr(boss);

                pillarScript->SetBoss(bossPtr);
                pillarScript->SetHP(boss->GetPillarHP());

                m_spawnedPillars.push_back(pillarPtr);
                boss->OnPillarCreated(pillarPtr);
            }
        }

        // 쉴드 이펙트 생성
        SpawnShieldEffects(boss);
    }

    // ═══════════════════════════════════════════════════════════════
    // 기둥 생성 (랜덤 + 밀집도)
    // ═══════════════════════════════════════════════════════════════
    void BossPattern_PillarShield::SpawnRandomPillars(BossScript* boss)
    {
        if (!boss) return;

        int pillarCount = boss->GetPillarSpawnCount();
        float centerX = boss->GetPillarSpawnCenterX();
        float centerZ = boss->GetPillarSpawnCenterZ();
        float rangeX = boss->GetPillarSpawnRangeX();
        float rangeZ = boss->GetPillarSpawnRangeZ();
        float halfRangeX = rangeX * 0.5f;
        float halfRangeZ = rangeZ * 0.5f;
        float spawnY = boss->GetPillarSpawnY();
        float clustering = boss->GetPillarClusteringStrength();
        float overlapRadius = boss->GetPillarOverlapRadius();
        int maxAttempts = boss->GetPillarMaxSpawnAttempts();

        engine::Vector3 firstPillarPos = engine::Vector3::Zero;
        bool hasFirstPillar = false;

        for (int i = 0; i < pillarCount; ++i)
        {
            engine::Vector3 pillarPos;
            bool foundValidPos = false;
            int attempts = 0;

            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            // 위치 찾기 (겹침 체크 + 범위 체크)
            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            while (attempts < maxAttempts)
            {
                // ─────────────────────────────────────────────
                // 첫 번째 기둥
                // ─────────────────────────────────────────────
                if (i == 0)
                {
                    // 직사각형 영역 내 완전 랜덤 (중점 + 범위/2)
                    pillarPos.x = centerX + engine::Random::Float(-halfRangeX, halfRangeX);
                    pillarPos.z = centerZ + engine::Random::Float(-halfRangeZ, halfRangeZ);
                    pillarPos.y = spawnY;

                    // 첫 번째는 겹침 체크 불필요
                    foundValidPos = true;
                    firstPillarPos = pillarPos;
                    hasFirstPillar = true;
                    break;
                }
                // ─────────────────────────────────────────────
                // 두 번째 이후 기둥 (밀집도 적용)
                // ─────────────────────────────────────────────
                else
                {
                    // 1. 완전 랜덤 위치 (중점 + 범위/2)
                    engine::Vector3 randomPos;
                    randomPos.x = centerX + engine::Random::Float(-halfRangeX, halfRangeX);
                    randomPos.z = centerZ + engine::Random::Float(-halfRangeZ, halfRangeZ);
                    randomPos.y = spawnY;

                    // 2. 밀집 위치 (첫 기둥 중심 ±2.5m 원)
                    float angle = engine::Random::Float(0.0f, DirectX::XM_2PI);
                    float distance = engine::Random::Float(0.0f, 2.5f);
                    engine::Vector3 clusteredPos = firstPillarPos;
                    clusteredPos.x += std::cos(angle) * distance;
                    clusteredPos.z += std::sin(angle) * distance;

                    // 3. 선형 보간 (밀집도에 따라)
                    pillarPos.x = std::lerp(randomPos.x, clusteredPos.x, clustering);
                    pillarPos.z = std::lerp(randomPos.z, clusteredPos.z, clustering);
                    pillarPos.y = spawnY;

                    // 4. 범위 체크 (블렌딩 후 초과 가능, 중점 기준)
                    if (std::abs(pillarPos.x - centerX) > halfRangeX || 
                        std::abs(pillarPos.z - centerZ) > halfRangeZ)
                    {
                        attempts++;
                        continue;  // 재시도
                    }

                    // 5. 겹침 체크
                    if (IsOverlapping(pillarPos, overlapRadius))
                    {
                        attempts++;
                        continue;  // 재시도
                    }

                    // 모든 체크 통과
                    foundValidPos = true;
                    break;
                }

                attempts++;
            }

            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            // 유효한 위치를 찾지 못한 경우
            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            if (!foundValidPos)
            {
                continue;  // 해당 기둥 생성 포기
            }

            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            // 기둥 생성
            // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
            auto go = engine::Prefab::Instantiate("BossPillar");
            if (!go) continue;

            go->GetTransform()->SetLocalPosition(pillarPos);

            // BossPillar 스크립트 설정
            auto pillarScript = go->GetComponent<BossPillar>();
            if (pillarScript)
            {
                engine::Ptr<BossPillar> pillarPtr(pillarScript);
                engine::Ptr<BossScript> bossPtr(boss);

                pillarScript->SetBoss(bossPtr);
                pillarScript->SetHP(boss->GetPillarHP());

                m_spawnedPillars.push_back(pillarPtr);
                boss->OnPillarCreated(pillarPtr);
            }
        }

        // 쉴드 이펙트 생성
        SpawnShieldEffects(boss);
    }

    // ═══════════════════════════════════════════════════════════════
    // 쉴드 이펙트 생성 (기존 로직 유지)
    // ═══════════════════════════════════════════════════════════════
    void BossPattern_PillarShield::SpawnShieldEffects(BossScript* boss)
    {
        if (!boss) return;

        auto bossPos = boss->GetTransform()->GetLocalPosition();
        
        // 보스 앞쪽 라인에 3개 배치
        for (int i = 0; i < 3; ++i)
        {
            auto offset = engine::Vector3(-3.0f + i * 3.0f, 0.0f, -2.0f);

            auto go = engine::Prefab::Instantiate("BossShieldEffect");
            if (!go) continue;

            go->GetTransform()->SetLocalPosition(bossPos + offset);

            m_shieldEffects.push_back(go);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 겹침 체크 헬퍼
    // ═══════════════════════════════════════════════════════════════
    bool BossPattern_PillarShield::IsOverlapping(const engine::Vector3& pos, float radius) const
    {
        // 기존 생성된 모든 기둥과 거리 체크
        for (const auto& pillar : m_spawnedPillars)
        {
            if (!pillar || !pillar->GetGameObject()) continue;

            engine::Vector3 existingPos = pillar->GetTransform()->GetWorldPosition();
            
            // XZ 평면 거리만 계산
            float dx = pos.x - existingPos.x;
            float dz = pos.z - existingPos.z;
            float distance = std::sqrt(dx * dx + dz * dz);

            // 겹침 판정: 두 반지름의 합보다 가까우면 겹침
            if (distance < radius * 2.0f)
            {
                return true;  // 겹침!
            }
        }

        return false;  // 안 겹침
    }
}
