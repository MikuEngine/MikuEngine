#include "GamePCH.h"
#include "BossPattern_BulletFire.h"

#include "Script/Boss/BossScript.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/CharacterScript/Common/BulletParams.h"

namespace game
{
    void BossPattern_BulletFire::Start(BossScript* boss)
    {
        if (!boss)
        {
            return;
        }

        m_isActive = true;
        m_intervalTimer = 0.0f;

        // 다음 interval 설정 (랜덤 or 고정)
        m_currentInterval = boss->GetBulletFireInterval();

        // 시작 시 즉시 발사
        FireBullets(boss);
    }

    void BossPattern_BulletFire::Update(BossScript* boss, float deltaTime)
    {
        if (!boss) return;

        // intervalTimer 업데이트
        m_intervalTimer += deltaTime;

        // interval이 지나면 발사
        if (m_intervalTimer >= m_currentInterval)
        {
            FireBullets(boss);
            m_intervalTimer = 0.0f;

            // 다음 interval 설정 (랜덤 or 고정)
            m_currentInterval = boss->GetBulletFireInterval();
        }
    }

    void BossPattern_BulletFire::End(BossScript* boss)
    {
        m_isActive = false;
        m_intervalTimer = 0.0f;
    }

    void BossPattern_BulletFire::FireBullets(BossScript* boss)
    {
        if (!boss) return;

        // BulletFactory 가져오기
        auto bulletFactory = boss->GetBulletFactory();
        if (!bulletFactory) return;

        // 보스 XZ 좌표만 가져오기 (Y=0 기준)
        auto* bossTransform = boss->GetGameObject()->GetTransform();
        if (!bossTransform) return;
        engine::Vector3 bossWorldPos = bossTransform->GetWorldPosition();
        engine::Vector3 bossPos = engine::Vector3(bossWorldPos.x, 0.0f, bossWorldPos.z);

        // 플레이어 방향 계산 (XZ 평면만 사용)
        engine::Vector3 directionToPlayer = engine::Vector3(0.0f, 0.0f, 1.0f);  // 기본 방향
        auto player = boss->GetTargetPlayer();
        if (player)
        {
            auto* playerTr = player->GetTransform();
            if (playerTr)
            {
                engine::Vector3 playerWorldPos = playerTr->GetWorldPosition();
                directionToPlayer = playerWorldPos - bossPos;
                
                // Y 성분 제거 (XZ 평면 방향만 사용)
                directionToPlayer.y = 0.0f;
                
                // 정규화
                float length = directionToPlayer.Length();
                if (length > 0.001f)
                {
                    directionToPlayer = directionToPlayer / length;
                }
            }
        }

        // BulletParams 설정
        BulletParams params;
        params.type = BulletType::Linear;
        params.speed = boss->GetBulletFireSpeed();
        params.lifetime = boss->GetBulletFireLifetime();
        params.damage = boss->GetBulletFireDamage();
        params.scale = boss->GetBulletFireScale();

        // 퍼짐 각도 (랜덤 or 고정, Radian)
        float spreadAngleRad = boss->GetBulletFireSpread();

        // 발사 위치 오프셋 적용
        engine::Vector3 spawnOffset = boss->GetBulletFireSpawnOffset();
        engine::Vector3 finalSpawnPos = bossPos + spawnOffset;

        // BulletFactory를 통해 3방향 발사
        bulletFactory->ThreewayFireBoss(finalSpawnPos, directionToPlayer, spreadAngleRad, params);
    }
}
