#include "GamePCH.h"
#include "BossPattern_Meteor.h"

#include <Framework/Object/Component/Renderer/StaticMeshRenderer.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Asset/Prefab.h>

#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/Boss/BossScript.h"
#include "Script/Boss/BossPattern/Components/BossPillar.h"
#include "Script/Boss/BossPattern/Components/BossBullet.h"

namespace game
{
    namespace
    {
        struct XZDirections
        {
            static constexpr float kDiag = 0.70710678f;

            static constexpr engine::Vector3 Forward{ 0.0f, 0.0f,  1.0f }; // 앞 (+Z)
            static constexpr engine::Vector3 Backward{ 0.0f, 0.0f, -1.0f }; // 뒤 (-Z)
            static constexpr engine::Vector3 Left{ -1.0f, 0.0f,  0.0f }; // 좌 (-X)
            static constexpr engine::Vector3 Right{ 1.0f, 0.0f,  0.0f }; // 우 (+X)

            static constexpr engine::Vector3 ForwardLeft{ -kDiag, 0.0f,  kDiag };
            static constexpr engine::Vector3 ForwardRight{ kDiag, 0.0f,  kDiag };
            static constexpr engine::Vector3 BackLeft{ -kDiag, 0.0f, -kDiag };
            static constexpr engine::Vector3 BackRight{ kDiag, 0.0f, -kDiag };
        };

        static constexpr std::array<engine::Vector3, 8> g_offsets8{
            XZDirections::Forward,
            XZDirections::ForwardRight,
            XZDirections::Right,
            XZDirections::BackRight,
            XZDirections::Backward,
            XZDirections::BackLeft,
            XZDirections::Left,
            XZDirections::ForwardLeft
        };
    }

    void BossPattern_Meteor::Start(BossScript* boss)
    {
        if (!boss) return;

        m_isActive = true;
        m_intervalTimer = 0.0f;

        // 시작 시 즉시 운석 생성
        SpawnMeteors(boss);
    }

    void BossPattern_Meteor::Update(BossScript* boss, float deltaTime)
    {
        if (!boss) return;

        // intervalTimer 업데이트
        m_intervalTimer += deltaTime;

        // interval이 지나면 새 운석 생성
        if (m_intervalTimer >= m_interval)
        {
            SpawnMeteors(boss);
            m_intervalTimer = 0.0f;
        }

        // 활성 운석들 업데이트
        UpdateMeteors(boss, deltaTime);
    }

    void BossPattern_Meteor::End(BossScript* boss)
    {
        m_isActive = false;
        m_intervalTimer = 0.0f;

        // 모든 운석 정리
        for (auto& meteor : m_activeMeteors)
        {
            if (meteor.meteorGO && meteor.meteorGO.Get())
            {
                meteor.meteorGO->Destroy();
            }
        }
        m_activeMeteors.clear();
    }

    void BossPattern_Meteor::SpawnMeteors(BossScript* boss)
    {
        auto bossTransform = boss->GetTransform();

        engine::Vector3 bossPos = bossTransform->GetWorldPosition();

        // 플레이어 위치 (운석 낙하 위치 참고용)
        engine::Vector3 playerPos = bossPos;
        auto player = boss->GetTargetPlayer();
        if (player)
        {
            playerPos = player->GetTransform()->GetWorldPosition();
        }

        // 운석 생성
        for (int i = 0; i < m_meteorCount; ++i)
        {
            engine::Vector3 spawnPos(
                engine::Random::Float(m_spawnPosMin.x, m_spawnPosMax.z),
                m_fallHeight,
                engine::Random::Float(m_spawnPosMin.z, m_spawnPosMax.z)
            );

            // 목표 위치 (지면)
            engine::Vector3 targetPos = spawnPos;
            targetPos.y = 0.0f;  // 보스와 같은 높이 (지면)

            // 운석 GameObject 생성
            std::string meteorName = "BossMeteor_" + std::to_string(i) + "_" + std::to_string(rand());
            auto meteorGO = engine::Prefab::Instantiate("BossMeteorProjectile");
            meteorGO->SetName(meteorName);

            // Transform 설정
            meteorGO->GetTransform()->SetLocalPosition(spawnPos);

            auto warningGO = engine::Prefab::Instantiate("BossMeteorWarning");
            warningGO->SetName(meteorName+ "_warning");

            engine::Vector3 warningPos = spawnPos;
            warningPos.y = 0.01f;
            warningGO->GetTransform()->SetLocalPosition(warningPos);

            // MeteorData 추가
            MeteorData meteorData;
            meteorData.meteorGO = engine::Ptr<engine::GameObject>(meteorGO);
            meteorData.warningGO = engine::Ptr<engine::GameObject>(warningGO);
            meteorData.targetPos = targetPos;
            meteorData.elapsedTime = 0.0f;
            meteorData.hasLanded = false;

            m_activeMeteors.push_back(meteorData);

            // TODO: StaticMeshRenderer 추가 (운석 모델)
            // TODO: Collider 추가 (충돌 감지용, 경고 표시용)
            // TODO: 경고 이펙트 표시 (지면에 마커 등)
        }
    }

    void BossPattern_Meteor::UpdateMeteors(BossScript* boss, float deltaTime)
    {
        if (!boss) return;

        // 운석들 업데이트 및 정리
        m_activeMeteors.erase(
            std::remove_if(m_activeMeteors.begin(), m_activeMeteors.end(),
                [this, boss, deltaTime](MeteorData& meteor) -> bool
                {
                    if (!meteor.meteorGO || !meteor.meteorGO.Get())
                    {
                        return true;  // 제거
                    }

                    if (meteor.hasLanded)
                    {
                        // 이미 착지한 운석은 제거
                        meteor.meteorGO->Destroy();
                        return true;
                    }

                    meteor.elapsedTime += deltaTime;

                    auto* transform = meteor.meteorGO->GetTransform();
                    if (!transform) return true;

                    engine::Vector3 currentPos = transform->GetWorldPosition();

                    // 경고 시간 동안은 제자리 (또는 경고 이펙트만 표시)
                    if (meteor.elapsedTime < m_warningTime)
                    {
                        // 경고 시간 동안은 위치 유지 (또는 경고 이펙트 표시)
                        // TODO: 경고 이펙트 업데이트
                        return false;
                    }

                    // 낙하 시작
                    float fallTime = meteor.elapsedTime - m_warningTime;
                    float fallDistance = m_fallSpeed * fallTime;

                    engine::Vector3 startPos = currentPos;
                    startPos.y = m_fallHeight;

                    float totalDistance = m_fallHeight;
                    float progress = std::min(fallDistance / totalDistance, 1.0f);

                    // Lerp 직접 구현 (Vector3::Lerp가 없을 수 있음)
                    engine::Vector3 newPos = engine::Vector3::Lerp(startPos, meteor.targetPos, progress);
                    transform->SetLocalPosition(newPos);

                    // 착지 체크
                    if (progress >= 1.0f)
                    {
                        meteor.hasLanded = true;

                        // 충돌 체크 (플레이어에게 데미지)
                        // TODO: 충돌 감지 및 데미지 처리
                        // auto* player = boss->GetTargetPlayer();
                        // if (player && ...) { player->TakeDamage(static_cast<int>(m_meteorDamage)); }

                        for (auto& dir : g_offsets8)
                        {
                            auto go = engine::Prefab::Instantiate("BossBulletProjectile");
                            go->GetTransform()->SetLocalPosition(meteor.meteorGO->GetTransform()->GetLocalPosition() + engine::Vector3(0.0f, 1.0f, 0.0f));

                            auto bullet = go->GetComponent<BossBullet>();
                            if (bullet)
                            {
                                bullet->Setup(dir, m_bulletSpeed, m_bulletDamage, m_bulletLifetime);
                            }
                        }

                        // 운석 파괴 (약간의 지연 후)
                        meteor.meteorGO->Destroy();
                        meteor.warningGO->Destroy();
                        return true;  // 제거
                    }

                    return false;  // 유지
                }),
            m_activeMeteors.end()
        );
    }
}
