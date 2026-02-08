#include "GamePCH.h"
#include "BossPattern_SphereProjectile.h"

#include <Framework/Object/Component/Collider.h>
#include <Framework/Object/Component/Renderer/StaticMeshRenderer.h>
#include <Framework/Asset/Prefab.h>

#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/Boss/BossScript.h"
#include "Script/Boss/BossPattern/Components/BossBigProjectile.h"

namespace game
{
    void BossPattern_SphereProjectile::Start(BossScript* boss)
    {
        if (!boss) return;

        m_isActive = true;
        m_intervalTimer = 0.0f;

        // 시작 시 즉시 발사
        FireProjectile(boss);
    }

    void BossPattern_SphereProjectile::Update(BossScript* boss, float deltaTime)
    {
        if (!boss) return;

        // intervalTimer 업데이트
        m_intervalTimer += deltaTime;

        // interval이 지나면 발사
        if (m_intervalTimer >= m_interval)
        {
            FireProjectile(boss);
            m_intervalTimer = 0.0f;
        }
    }

    void BossPattern_SphereProjectile::End(BossScript* boss)
    {
        m_isActive = false;
        m_intervalTimer = 0.0f;
    }

    void BossPattern_SphereProjectile::FireProjectile(BossScript* boss)
    {
        if (!boss)
        {
            return;
        }

        auto* bossTransform = boss->GetGameObject()->GetTransform();
        if (!bossTransform) return;

        // 보스 XZ 좌표만 가져오기 (Y=0 기준)
        engine::Vector3 bossWorldPos = bossTransform->GetWorldPosition();
        engine::Vector3 bossPos = engine::Vector3(bossWorldPos.x, 0.0f, bossWorldPos.z);

        // 플레이어 위치 찾기
        engine::Vector3 playerPos = bossPos;
        auto player = boss->GetTargetPlayer();
        if (player)
        {
            playerPos = player->GetTransform()->GetWorldPosition();
        }

        // 보스 좌표(Y=0) + 오프셋 적용
        engine::Vector3 spawnOffset = boss->GetBigProjectileSpawnOffset();
        engine::Vector3 spawnPos = bossPos + spawnOffset;

        // 플레이어 방향 계산 (오프셋 적용된 위치에서, XZ 평면만)
        engine::Vector3 directionToPlayer = engine::Vector3(0.0f, 0.0f, 1.0f);  // 기본 방향
        if (player)
        {
            directionToPlayer = playerPos - spawnPos;
            
            // Y 성분 제거 (XZ 평면 방향만 사용)
            directionToPlayer.y = 0.0f;
            
            float length = directionToPlayer.Length();
            if (length > 0.001f)
            {
                directionToPlayer = directionToPlayer / length;
            }
        }

        // 투사체 GameObject 생성
        auto projectileGO = engine::Prefab::Instantiate("BossBigBulletProjectile");
        if (!projectileGO) return;

        // Transform 설정 (오프셋 적용된 위치)
        projectileGO->GetTransform()->SetLocalPosition(spawnPos);
        
        // BossBigProjectile 스크립트 Setup (boss에서 모든 설정 가져오기)
        auto* projectileScript = projectileGO->GetComponent<BossBigProjectile>();
        if (projectileScript)
        {
            projectileScript->Setup(directionToPlayer, boss);
        }
    }
}
