#include "GamePCH.h"
#include "BossPattern_BulletFire.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Object/Component/Renderer/StaticMeshRenderer.h>
#include <Framework/Asset/Prefab.h>
#include <Common/Math/MathUtility.h>

#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/Boss/BossScript.h"
#include "Script/Boss/BossPattern/Components/BossBullet.h"

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

        // 시작 시 즉시 발사
        FireBullets(boss);
    }

    void BossPattern_BulletFire::Update(BossScript* boss, float deltaTime)
    {
        if (!boss) return;

        // intervalTimer 업데이트
        m_intervalTimer += deltaTime;

        // interval이 지나면 발사
        if (m_intervalTimer >= m_interval)
        {
            FireBullets(boss);
            m_intervalTimer = 0.0f;
        }
    }

    void BossPattern_BulletFire::End(BossScript* boss)
    {
        m_isActive = false;
        m_intervalTimer = 0.0f;
    }

    void BossPattern_BulletFire::FireBullets(BossScript* boss)
    {
        auto bossTransform = boss->GetGameObject()->GetTransform();

        engine::Vector3 bossPos = bossTransform->GetWorldPosition();

        // 플레이어 위치 찾기
        engine::Vector3 playerPos = bossPos;
        auto player = boss->GetTargetPlayer();
        
        // 플레이어 방향 계산 (보스가 플레이어를 향하도록)
        engine::Vector3 directionToPlayer = engine::Vector3(0.0f, 0.0f, 1.0f);  // 기본 방향
        if (player)
        {
            auto playerTr = player->GetTransform();
            if (playerTr)
            {
                engine::Vector3 playerWorldPos = playerTr->GetWorldPosition();
                directionToPlayer = playerWorldPos - bossPos;
                float length = directionToPlayer.Length();
                if (length > 0.001f)
                {
                    directionToPlayer = directionToPlayer / length;
                }
            }
        }

        // 탄환 발사
        for (int i = 0; i < m_bulletCount; ++i)
        {
            // 분산 각도 계산
            float angleOffset = 0.0f;
            if (m_bulletCount > 1)
            {
                float angleStep = m_spreadAngle / static_cast<float>(m_bulletCount - 1);
                angleOffset = -m_spreadAngle * 0.5f + angleStep * static_cast<float>(i);
            }

            // 방향 회전 (Y축 기준)
            engine::Vector3 fireDirection = directionToPlayer;
            if (std::abs(angleOffset) > 0.001f)
            {
                float angleRad = engine::ToRadian(angleOffset);
                float cosAngle = std::cosf(angleRad);
                float sinAngle = std::sinf(angleRad);

                // Y축 기준 회전 (수평면에서 회전)
                float x = fireDirection.x * cosAngle - fireDirection.z * sinAngle;
                float z = fireDirection.x * sinAngle + fireDirection.z * cosAngle;
                fireDirection = engine::Vector3(x, fireDirection.y, z);
                
                // 정규화
                float dirLength = fireDirection.Length();
                if (dirLength > 0.001f)
                {
                    fireDirection = fireDirection / dirLength;
                }
            }

            // 탄환 GameObject 생성
            std::string bulletName = "BossBullet_" + std::to_string(i);

            auto bulletGO = engine::Prefab::Instantiate("BossBullet");
            bulletGO->SetName(bulletName);

            // Transform 설정
            bulletGO->GetTransform()->SetLocalPosition(bossPos);

            // BossBullet 스크립트 추가
            auto bulletScript = bulletGO->GetComponent<BossBullet>();
            if (bulletScript)
            {
                bulletScript->Setup(fireDirection, m_bulletSpeed, m_bulletDamage, m_bulletLifetime);
            }
        }
    }
}
