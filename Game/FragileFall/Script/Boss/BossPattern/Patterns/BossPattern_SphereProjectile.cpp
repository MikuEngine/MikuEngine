#include "GamePCH.h"
#include "BossPattern_SphereProjectile.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Object/Component/Renderer/StaticMeshRenderer.h>
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

#include "Script/Boss/BossScript.h"
#include "Script/Boss/BossPattern/Components/BossProjectile.h"

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
        if (!boss || !boss->GetGameObject()) return;

        auto* scene = engine::SceneManager::Get().GetScene();
        if (!scene) return;

        auto* bossTransform = boss->GetGameObject()->GetTransform();
        if (!bossTransform) return;

        engine::Vector3 bossPos = bossTransform->GetWorldPosition();

        // 플레이어 위치 찾기
        engine::Vector3 playerPos = bossPos;
        auto player = boss->GetTargetPlayer();
        if (player)
        {
            playerPos = player->GetTransform()->GetWorldPosition();
        }

        // 플레이어 방향 계산
        engine::Vector3 directionToPlayer = engine::Vector3(0.0f, 0.0f, 1.0f);  // 기본 방향
        if (player)
        {
            directionToPlayer = playerPos - bossPos;
            float length = directionToPlayer.Length();
            if (length > 0.001f)
            {
                directionToPlayer = directionToPlayer / length;
            }
        }

        // 투사체 GameObject 생성
        std::string projectileName = "BossProjectile_" + std::to_string(rand());
        auto* projectileGO = scene->CreateGameObject(projectileName);
        if (!projectileGO) return;

        // Transform 설정
        auto* projectileTransform = projectileGO->GetTransform();
        if (projectileTransform)
        {
            projectileTransform->SetLocalPosition(bossPos);
        }

        // BossProjectile 스크립트 추가
        auto* projectileScript = projectileGO->AddComponent<BossProjectile>();
        if (projectileScript)
        {
            projectileScript->Setup(
                directionToPlayer,
                m_projectileSpeed,
                m_projectileDamage,
                m_projectileLifetime,
                boss
            );
        }

        // TODO: StaticMeshRenderer 추가 (구체 모델)
        // TODO: Collider 추가 (충돌 감지용, 결정화 감지용)
    }
}
