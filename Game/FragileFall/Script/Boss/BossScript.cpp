#include "GamePCH.h"
#include "BossScript.h"

#include <Framework/Object/Component/Renderer/StaticMeshRenderer.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Object/Component/UI/UIText.h>
#include <Common/Math/MathUtility.h>

#include "Script/Boss/BossPattern/BossPatternManager.h"
#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/Boss/BossPattern/Patterns/BossPattern_PillarShield.h"
#include "Script/Boss/BossPattern/Patterns/BossPattern_BulletFire.h"
#include "Script/Boss/BossPattern/Patterns/BossPattern_Meteor.h"
#include "Script/Boss/BossPattern/Patterns/BossPattern_Summon.h"
#include "Script/Boss/BossPattern/Patterns/BossPattern_SphereProjectile.h"
#include "Script/Boss/BossPattern/Components/BossPillar.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

namespace game
{
    BossScript::BossScript() = default;
    BossScript::~BossScript() = default;

    void BossScript::Awake()
    {
        auto go = engine::GameObject::Find("UI_BossHP");
        m_hpText = go->GetComponent<engine::UIText>();
    }

    void BossScript::Start()
    {
        m_hpText->SetText(std::format("HP: {}", m_currentHp));

        InitializeCrystalMeshes();
        InitializePatterns();

        auto playerGo = engine::GameObject::Find("Player");
        if (playerGo != nullptr)
        {
            m_targetPlayer = playerGo->GetComponent<PlayerControllerScript>();
        }

        // BulletFactory 찾기 (같은 GameObject 내)
        m_bulletFactory = GetGameObject()->GetComponent<BulletFactory>();
        if (!m_bulletFactory)
        {
            LOG_PRINT("[BossScript] WARNING: BulletFactory not found on Boss GameObject!");
        }
    }

    void BossScript::Update()
    {
        float deltaTime = engine::Time::DeltaTime();

        UpdateCrystalMovement(deltaTime);
        UpdateShieldStatus();
        UpdatePatternSystem(deltaTime);
        CheckHealth();
    }

    void BossScript::InitializePatterns()
    {
        m_patternManager = std::make_unique<BossPatternManager>();
        m_patternManager->Initialize(this);

        // 독립 패턴 (기둥 쉴드) - 10초마다 독립 실행
        auto pillarShield = std::make_unique<BossPattern_PillarShield>();
        m_patternManager->SetIndependentPattern(pillarShield.get());
        m_patternManager->RegisterPattern(std::move(pillarShield));

        // 일반 패턴들 등록
        m_patternManager->RegisterPattern(std::make_unique<BossPattern_BulletFire>());
        m_patternManager->RegisterPattern(std::make_unique<BossPattern_Meteor>());
        m_patternManager->RegisterPattern(std::make_unique<BossPattern_Summon>());
        m_patternManager->RegisterPattern(std::make_unique<BossPattern_SphereProjectile>());
    }

    void BossScript::InitializeCrystalMeshes()
    {
        const auto& children = GetTransform()->GetChildren();
        for (auto* childTransform : children)
        {
            m_crystalMeshGameObjects.push_back(childTransform->GetGameObject());
        }
    }

    void BossScript::UpdatePatternSystem(float deltaTime)
    {
        if (m_patternManager)
        {
            m_patternManager->Update(deltaTime);
        }
    }

    void BossScript::OnPatternStarted(const std::string& patternName)
    {
        // TODO: 패턴 시작 시 처리
    }

    void BossScript::OnPatternFinished(const std::string& patternName)
    {
        // TODO: 패턴 종료 시 처리
    }

    void BossScript::TakeDamage(float damage, bool isShieldPierce)
    {
        if (m_isShieldActive)
        {
            // 실드 관통 불가: 데미지 차단
            if (!isShieldPierce)
            {
                return;
            }
            // 실드 관통 가능: 감쇄율은 호출부(OnReflectedProjectileHit)에서 이미 적용됨
            // 감쇄율 100%면 damage는 이미 0.0f
        }

        m_currentHp -= damage;
        if (m_currentHp < 0)
        {
            m_currentHp = 0;
        }

        m_hpText->SetText(std::format("HP: {}", m_currentHp));
    }

    void BossScript::CheckHealth()
    {
        if (m_currentHp <= 0)
        {
            OnDeath();
        }
    }

    void BossScript::OnDeath()
    {
        // TODO: 보스 사망 처리
    }

    void BossScript::UpdateShieldStatus()
    {
        // 기둥 리스트에서 파괴된 기둥 제거
        m_activePillars.erase(
            std::remove_if(m_activePillars.begin(), m_activePillars.end(),
                [](const engine::Ptr<BossPillar>& pillar) {
                    if (!pillar) return true;
                    return false;
                }),
            m_activePillars.end()
        );

        // 기둥이 하나라도 살아있으면 쉴드 활성화
        m_isShieldActive = !m_activePillars.empty();
    }

    void BossScript::OnPillarCreated(engine::Ptr<BossPillar> pillar)
    {
        if (pillar)
        {
            m_activePillars.push_back(pillar);
        }
    }

    void BossScript::OnPillarDestroyed(engine::Ptr<BossPillar> pillar)
    {
        // 기둥 파괴 시 리스트에서 제거
        auto it = std::find(m_activePillars.begin(), m_activePillars.end(), pillar);
        if (it != m_activePillars.end())
        {
            m_activePillars.erase(it);
        }
    }

    void BossScript::SetColor(BossColor color)
    {
        m_currentColor = color;
    }

    std::string BossScript::GetColorName() const
    {
        switch (m_currentColor)
        {
        case BossColor::Red: return "Red";
        case BossColor::Blue: return "Blue";
        case BossColor::Green: return "Green";
        case BossColor::Yellow: return "Yellow";
        case BossColor::Purple: return "Purple";
        default: return "Unknown";
        }
    }
    
    engine::Ptr<PlayerControllerScript> BossScript::GetTargetPlayer() const
    {
        return m_targetPlayer;
    }

    void BossScript::OnCrystallized()
    {
        // TODO: 결정화 상태 진입 처리
    }

    void BossScript::OnExecutionReflected(engine::Vector3 direction)
    {
        // 기존 하위 호환 로직 (하드코딩)
        TakeDamage(200);
    }

    void BossScript::OnReflectedProjectileHit(const engine::Vector3& direction, float damage)
    {
        // 실드 감쇄 적용
        if (m_isShieldActive)
        {
            float reduction = m_bigProjectileShieldReduction;  // 0~100%
            damage *= (1.0f - reduction / 100.0f);
        }
        
        // 실드 관통 플래그와 함께 데미지 전달
        TakeDamage(damage, m_isShieldPierce);
    }

    void BossScript::UpdateCrystalMovement(float deltaTime)
    {
        // 메인 수정 주변을 도는 작은 수정들
        for (size_t i = 0; i < m_crystalMeshGameObjects.size(); ++i)
        {
            if (m_crystalMeshGameObjects[i])
            {
                float angle = static_cast<float>(i) * (360.0f / m_crystalMeshGameObjects.size()) + deltaTime * 30.0f;  // 초당 30도 회전
                float radius = 2.0f;

                float angleRad = engine::ToRadian(angle);
                engine::Vector3 offset(std::cosf(angleRad) * radius, 0.0f, std::sinf(angleRad) * radius);

                auto* meshTransform = m_crystalMeshGameObjects[i]->GetTransform();
                if (meshTransform)
                {
                    meshTransform->SetLocalPosition(GetTransform()->GetWorldPosition() + offset);
                }
            }
        }
    }

    float BossScript::GetBulletFireInterval() const
    {
        if (m_bulletFireUseFixedInterval)
        {
            return m_bulletFireFixedInterval;
        }
        else
        {
            // 랜덤 interval (최소 ~ 최대)
            float t = engine::Random::Float(0.0f, 1.0f);
            return m_bulletFireMinInterval + t * (m_bulletFireMaxInterval - m_bulletFireMinInterval);
        }
    }

    float BossScript::GetBulletFireSpread() const
    {
        float spreadDegree = 0.0f;

        if (m_bulletFireUseFixedSpread)
        {
            spreadDegree = m_bulletFireFixedSpread;
        }
        else
        {
            // 랜덤 spread (최소 ~ 최대)
            float t = engine::Random::Float(0.0f, 1.0f);
            spreadDegree = m_bulletFireMinSpread + t * (m_bulletFireMaxSpread - m_bulletFireMinSpread);
        }

        // Degree → Radian 변환
        return engine::ToRadian(spreadDegree);
    }

    // ═══════════════════════════════════════════════════════════════
    // Meteor 설정 Getter
    // ═══════════════════════════════════════════════════════════════
    float BossScript::GetMeteorInterval() const
    {
        if (m_meteorUseFixedInterval)
        {
            return m_meteorFixedInterval;
        }
        else
        {
            // 랜덤 interval (최소 ~ 최대)
            float t = engine::Random::Float(0.0f, 1.0f);
            return m_meteorMinInterval + t * (m_meteorMaxInterval - m_meteorMinInterval);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // PillarShield 설정 Getter
    // ═══════════════════════════════════════════════════════════════
    float BossScript::GetPillarRespawnDelay() const
    {
        if (m_pillarUseFixedRespawnDelay)
        {
            return m_pillarFixedRespawnDelay;
        }
        else
        {
            // 랜덤 delay (최소 ~ 최대)
            float t = engine::Random::Float(0.0f, 1.0f);
            return m_pillarMinRespawnDelay + t * (m_pillarMaxRespawnDelay - m_pillarMinRespawnDelay);
        }
    }

    int BossScript::GetPillarSpawnCount() const
    {
        if (m_pillarUseFixedCount)
        {
            return m_pillarFixedCount;
        }
        else
        {
            // 랜덤 count (최소 ~ 최대)
            return engine::Random::Int(m_pillarMinCount, m_pillarMaxCount);
        }
    }

    void BossScript::OnGui()
    {
        // ═════════════════════════════════════════════
        // 보스 기본 설정
        // ═════════════════════════════════════════════
        ImGui::SeparatorText("=== Boss Core Settings ===");
        
        if (ImGui::DragFloat("Max HP", &m_maxHp, 10.0f, 1.0f, 100000.0f, "%.1f"))
        {
            m_maxHp = std::clamp(m_maxHp, 1.0f, 100000.0f);
            
            // 현재 HP가 최대 HP를 초과하지 않도록
            if (m_currentHp > m_maxHp)
            {
                m_currentHp = m_maxHp;
            }
        }
        
        ImGui::Text("Current HP: %.1f", m_currentHp);
        ImGui::Spacing();

        // ═════════════════════════════════════════════
        // BulletFire 패턴 설정
        // ═════════════════════════════════════════════
        ImGui::SeparatorText("=== Boss BulletFire Settings ===");

        // ─────────────────────────────────────────────
        // 발사 주기 (Interval)
        // ─────────────────────────────────────────────
        ImGui::Text("--- Fire Interval ---");
        ImGui::Checkbox("Use Fixed Interval", &m_bulletFireUseFixedInterval);

        if (m_bulletFireUseFixedInterval)
        {
            if (ImGui::DragFloat("Fixed Interval (sec)", &m_bulletFireFixedInterval, 0.1f, 0.1f, 100.0f, "%.2f"))
            {
                m_bulletFireFixedInterval = std::clamp(m_bulletFireFixedInterval, 0.1f, 100.0f);
            }
        }
        else
        {
            if (ImGui::DragFloat("Min Interval (sec)", &m_bulletFireMinInterval, 0.1f, 0.1f, 100.0f, "%.2f"))
            {
                m_bulletFireMinInterval = std::clamp(m_bulletFireMinInterval, 0.1f, 100.0f);
            }
            if (ImGui::DragFloat("Max Interval (sec)", &m_bulletFireMaxInterval, 0.1f, 0.1f, 100.0f, "%.2f"))
            {
                m_bulletFireMaxInterval = std::clamp(m_bulletFireMaxInterval, 0.1f, 100.0f);
            }

            // 최소 <= 최대 보장
            if (m_bulletFireMinInterval > m_bulletFireMaxInterval)
            {
                m_bulletFireMaxInterval = m_bulletFireMinInterval;
            }
        }

        ImGui::Spacing();

        // ─────────────────────────────────────────────
        // 탄퍼짐 각도 (Spread Angle)
        // ─────────────────────────────────────────────
        ImGui::Text("--- Spread Angle (Degree) ---");
        ImGui::Checkbox("Use Fixed Spread", &m_bulletFireUseFixedSpread);

        if (m_bulletFireUseFixedSpread)
        {
            if (ImGui::DragFloat("Fixed Spread (deg)", &m_bulletFireFixedSpread, 0.5f, 5.0f, 80.0f, "%.1f"))
            {
                m_bulletFireFixedSpread = std::clamp(m_bulletFireFixedSpread, 5.0f, 80.0f);
            }
        }
        else
        {
            if (ImGui::DragFloat("Min Spread (deg)", &m_bulletFireMinSpread, 0.5f, 5.0f, 80.0f, "%.1f"))
            {
                m_bulletFireMinSpread = std::clamp(m_bulletFireMinSpread, 5.0f, 80.0f);
            }
            if (ImGui::DragFloat("Max Spread (deg)", &m_bulletFireMaxSpread, 0.5f, 5.0f, 80.0f, "%.1f"))
            {
                m_bulletFireMaxSpread = std::clamp(m_bulletFireMaxSpread, 5.0f, 80.0f);
            }

            // 최소 <= 최대 보장
            if (m_bulletFireMinSpread > m_bulletFireMaxSpread)
            {
                m_bulletFireMaxSpread = m_bulletFireMinSpread;
            }
        }

        ImGui::Spacing();

        // ─────────────────────────────────────────────
        // 탄환 속성
        // ─────────────────────────────────────────────
        ImGui::Text("--- Bullet Properties ---");
        ImGui::DragFloat("Speed (m/s)", &m_bulletFireSpeed, 0.5f, 1.0f, 50.0f, "%.1f");
        
        if (ImGui::DragFloat("Lifetime (sec)", &m_bulletFireLifetime, 0.1f, 0.1f, 20.0f, "%.1f"))
        {
            m_bulletFireLifetime = std::clamp(m_bulletFireLifetime, 0.1f, 20.0f);
        }
        
        ImGui::DragFloat("Scale", &m_bulletFireScale, 0.05f, 0.1f, 5.0f, "%.2f");
        ImGui::DragFloat("Damage", &m_bulletFireDamage, 1.0f, 0.0f, 1000.0f, "%.1f");

        ImGui::Spacing();

        // ─────────────────────────────────────────────
        // 발사 위치 오프셋
        // ─────────────────────────────────────────────
        ImGui::Text("--- Spawn Offset (XZ) ---");
        ImGui::DragFloat("Offset X", &m_bulletFireSpawnOffsetX, 0.1f, -50.0f, 50.0f, "%.2f");
        ImGui::DragFloat("Offset Z", &m_bulletFireSpawnOffsetZ, 0.1f, -50.0f, 50.0f, "%.2f");

        ImGui::Separator();

        // ═══════════════════════════════════════════════════════════════
        // Meteor 패턴 설정
        // ═══════════════════════════════════════════════════════════════
        ImGui::SeparatorText("=== Boss Meteor Settings ===");

        // ─────────────────────────────────────────────
        // 메테오 스폰 주기
        // ─────────────────────────────────────────────
        ImGui::Text("--- Meteor Spawn Interval ---");
        ImGui::Checkbox("Use Fixed Meteor Interval", &m_meteorUseFixedInterval);

        if (m_meteorUseFixedInterval)
        {
            if (ImGui::DragFloat("Fixed Meteor Interval (sec)", &m_meteorFixedInterval, 0.1f, 0.1f, 100.0f, "%.2f"))
            {
                m_meteorFixedInterval = std::clamp(m_meteorFixedInterval, 0.1f, 100.0f);
            }
        }
        else
        {
            if (ImGui::DragFloat("Min Meteor Interval (sec)", &m_meteorMinInterval, 0.1f, 0.1f, 100.0f, "%.2f"))
            {
                m_meteorMinInterval = std::clamp(m_meteorMinInterval, 0.1f, 100.0f);
            }
            if (ImGui::DragFloat("Max Meteor Interval (sec)", &m_meteorMaxInterval, 0.1f, 0.1f, 100.0f, "%.2f"))
            {
                m_meteorMaxInterval = std::clamp(m_meteorMaxInterval, 0.1f, 100.0f);
            }

            if (m_meteorMinInterval > m_meteorMaxInterval)
            {
                m_meteorMaxInterval = m_meteorMinInterval;
            }
        }

        ImGui::Spacing();

        // ─────────────────────────────────────────────
        // 메테오 스폰
        // ─────────────────────────────────────────────
        ImGui::Text("--- Meteor Spawn ---");
        ImGui::DragFloat("Spawn Height (Y)", &m_meteorSpawnHeight, 0.5f, 1.0f, 50.0f, "%.1f");

        ImGui::Spacing();

        // ─────────────────────────────────────────────
        // 메테오 물리
        // ─────────────────────────────────────────────
        ImGui::Text("--- Meteor Physics ---");
        ImGui::DragFloat("Initial Speed (m/s)", &m_meteorInitialSpeed, 0.5f, 0.1f, 50.0f, "%.2f");
        ImGui::DragFloat("Own Gravity (m/s²)", &m_meteorOwnGravity, 0.5f, 0.0f, 50.0f, "%.2f");
        ImGui::DragFloat("Meteor Scale", &m_meteorScale, 0.05f, 0.1f, 5.0f, "%.2f");

        ImGui::Spacing();

        // ─────────────────────────────────────────────
        // 착지 판정
        // ─────────────────────────────────────────────
        ImGui::Text("--- Landing Detection ---");
        ImGui::DragFloat("Landing Y", &m_meteorLandingY, 0.1f, -10.0f, 10.0f, "%.2f");
        ImGui::DragFloat("Landing Threshold", &m_meteorLandingThreshold, 0.01f, 0.0f, 5.0f, "%.3f");

        ImGui::Spacing();

        // ─────────────────────────────────────────────
        // XZ 유효 범위
        // ─────────────────────────────────────────────
        ImGui::Text("--- Valid Spawn Area (XZ Rectangle) ---");
        ImGui::DragFloat("Center X", &m_meteorSpawnCenterX, 0.5f, -100.0f, 100.0f, "%.1f");
        ImGui::DragFloat("Center Z", &m_meteorSpawnCenterZ, 0.5f, -100.0f, 100.0f, "%.1f");
        ImGui::DragFloat("Width (X)", &m_meteorValidRangeX, 0.5f, 1.0f, 200.0f, "%.1f");
        ImGui::DragFloat("Height (Z)", &m_meteorValidRangeZ, 0.5f, 1.0f, 200.0f, "%.1f");

        ImGui::Spacing();

        // ─────────────────────────────────────────────
        // 예측 설정
        // ─────────────────────────────────────────────
        ImGui::Text("--- Prediction Settings ---");
        
        // 예측 강도
        if (ImGui::SliderFloat("Strength (0=now, 1=full)", &m_meteorPredictionStrength, 0.0f, 1.0f, "%.2f"))
        {
            m_meteorPredictionStrength = std::clamp(m_meteorPredictionStrength, 0.0f, 1.0f);
        }
        
        // 예측 정확도
        if (ImGui::SliderFloat("Accuracy (0=exact, 10=±10m)", &m_meteorPredictionAccuracy, 0.0f, 10.0f, "%.1f"))
        {
            m_meteorPredictionAccuracy = std::clamp(m_meteorPredictionAccuracy, 0.0f, 10.0f);
        }

        ImGui::Spacing();

        // ─────────────────────────────────────────────
        // 8방향 총알 속성
        // ─────────────────────────────────────────────
        ImGui::Text("--- Meteor 8-way Bullets ---");
        ImGui::DragFloat("Meteor Bullet Speed (m/s)", &m_meteorBulletSpeed, 0.5f, 1.0f, 50.0f, "%.1f");
        ImGui::DragFloat("Meteor Bullet Damage", &m_meteorBulletDamage, 1.0f, 0.0f, 1000.0f, "%.1f");
        
        if (ImGui::DragFloat("Meteor Bullet Lifetime (sec)", &m_meteorBulletLifetime, 0.1f, 0.1f, 20.0f, "%.1f"))
        {
            m_meteorBulletLifetime = std::clamp(m_meteorBulletLifetime, 0.1f, 20.0f);
        }
        
        ImGui::DragFloat("Meteor Bullet Scale", &m_meteorBulletScale, 0.05f, 0.1f, 5.0f, "%.2f");

        ImGui::Spacing();

        // ─────────────────────────────────────────────
        // 폭발 속성
        // ─────────────────────────────────────────────
        ImGui::Text("--- Meteor Explosion ---");
        ImGui::DragFloat("Explosion Damage", &m_meteorExplosionDamage, 1.0f, 0.0f, 1000.0f, "%.1f");
        ImGui::DragFloat("Explosion Radius (m)", &m_meteorExplosionRadius, 0.5f, 0.1f, 50.0f, "%.1f");
        ImGui::DragFloat("Explosion Lifetime (sec)", &m_meteorExplosionLifetime, 0.05f, 0.05f, 5.0f, "%.2f");

        ImGui::Separator();

        // ═══════════════════════════════════════════════════════════════
        // PillarShield 패턴 설정
        // ═══════════════════════════════════════════════════════════════
        ImGui::SeparatorText("=== Boss PillarShield Settings ===");

        // ─────────────────────────────────────────────
        // 패턴 모드
        // ─────────────────────────────────────────────
        ImGui::Text("--- Pattern Mode ---");
        ImGui::Checkbox("Use Basic Pattern (Left/Right 2 pillars)", &m_pillarUseBasicPattern);

        ImGui::Spacing();

        // ─────────────────────────────────────────────
        // 재생성 대기 시간
        // ─────────────────────────────────────────────
        ImGui::Text("--- Respawn Delay (After All Destroyed) ---");
        ImGui::Checkbox("Use Fixed Respawn Delay", &m_pillarUseFixedRespawnDelay);

        if (m_pillarUseFixedRespawnDelay)
        {
            if (ImGui::DragFloat("Fixed Delay (sec)", &m_pillarFixedRespawnDelay, 0.1f, 1.0f, 20.0f, "%.2f"))
            {
                m_pillarFixedRespawnDelay = std::clamp(m_pillarFixedRespawnDelay, 1.0f, 20.0f);
            }
        }
        else
        {
            if (ImGui::DragFloat("Min Delay (sec)", &m_pillarMinRespawnDelay, 0.1f, 1.0f, 20.0f, "%.2f"))
            {
                m_pillarMinRespawnDelay = std::clamp(m_pillarMinRespawnDelay, 1.0f, 20.0f);
            }
            if (ImGui::DragFloat("Max Delay (sec)", &m_pillarMaxRespawnDelay, 0.1f, 1.0f, 20.0f, "%.2f"))
            {
                m_pillarMaxRespawnDelay = std::clamp(m_pillarMaxRespawnDelay, 1.0f, 20.0f);
            }

            if (m_pillarMinRespawnDelay > m_pillarMaxRespawnDelay)
            {
                m_pillarMaxRespawnDelay = m_pillarMinRespawnDelay;
            }
        }

        ImGui::Spacing();

        // ─────────────────────────────────────────────
        // 생성 개수 (Use Basic = false일 때만 의미)
        // ─────────────────────────────────────────────
        if (!m_pillarUseBasicPattern)
        {
            ImGui::Text("--- Pillar Spawn Count ---");
            ImGui::Checkbox("Use Fixed Count", &m_pillarUseFixedCount);

            if (m_pillarUseFixedCount)
            {
                if (ImGui::DragInt("Fixed Count", &m_pillarFixedCount, 1, 1, 10))
                {
                    m_pillarFixedCount = std::clamp(m_pillarFixedCount, 1, 10);
                }
            }
            else
            {
                if (ImGui::DragInt("Min Count", &m_pillarMinCount, 1, 1, 10))
                {
                    m_pillarMinCount = std::clamp(m_pillarMinCount, 1, 10);
                }
                if (ImGui::DragInt("Max Count", &m_pillarMaxCount, 1, 1, 10))
                {
                    m_pillarMaxCount = std::clamp(m_pillarMaxCount, 1, 10);
                }

                if (m_pillarMinCount > m_pillarMaxCount)
                {
                    m_pillarMaxCount = m_pillarMinCount;
                }
            }

            ImGui::Spacing();
        }

        // ─────────────────────────────────────────────
        // 생성 영역 (0,0,0 중심 직사각형)
        // ─────────────────────────────────────────────
        if (!m_pillarUseBasicPattern)
        {
            ImGui::Text("--- Spawn Area (XZ Rectangle) ---");
            ImGui::DragFloat("Center X", &m_pillarSpawnCenterX, 0.5f, -100.0f, 100.0f, "%.1f");
            ImGui::DragFloat("Center Z", &m_pillarSpawnCenterZ, 0.5f, -100.0f, 100.0f, "%.1f");
            ImGui::DragFloat("Width (X)", &m_pillarSpawnRangeX, 0.5f, 1.0f, 200.0f, "%.1f");
            ImGui::DragFloat("Height (Z)", &m_pillarSpawnRangeZ, 0.5f, 1.0f, 200.0f, "%.1f");
            ImGui::DragFloat("Spawn Y", &m_pillarSpawnY, 0.1f, -10.0f, 10.0f, "%.1f");

            ImGui::Spacing();

            // ─────────────────────────────────────────────
            // 밀집도
            // ─────────────────────────────────────────────
            ImGui::Text("--- Clustering ---");
            if (ImGui::SliderFloat("Clustering (0=spread, 1=tight)", &m_pillarClusteringStrength, 0.0f, 1.0f, "%.2f"))
            {
                m_pillarClusteringStrength = std::clamp(m_pillarClusteringStrength, 0.0f, 1.0f);
            }

            ImGui::Spacing();
        }

        // ─────────────────────────────────────────────
        // 겹침 방지
        // ─────────────────────────────────────────────
        ImGui::Text("--- Overlap Prevention ---");
        ImGui::DragFloat("Overlap Radius (m)", &m_pillarOverlapRadius, 0.1f, 0.5f, 5.0f, "%.1f");
        ImGui::DragInt("Max Spawn Attempts", &m_pillarMaxSpawnAttempts, 1, 5, 100);

        ImGui::Spacing();

        // ─────────────────────────────────────────────
        // 기둥 HP
        // ─────────────────────────────────────────────
        ImGui::Text("--- Pillar Settings ---");
        ImGui::DragFloat("Pillar HP", &m_pillarHP, 1.0f, 1.0f, 500.0f, "%.1f");

        ImGui::Separator();

        // ═══════════════════════════════════════════════════════════════
        // BigProjectile 패턴 설정
        // ═══════════════════════════════════════════════════════════════
        ImGui::SeparatorText("=== Boss Big Projectile Settings ===");

        // ─────────────────────────────────────────────
        // 기본 설정
        // ─────────────────────────────────────────────
        ImGui::Text("--- Basic Settings ---");
        
        if (ImGui::DragFloat("BBP_HP", &m_bigProjectileHP, 0.1f, 0.1f, 10000.0f, "%.1f"))
        {
            if (m_bigProjectileHP <= 0.0f)
            {
                m_bigProjectileHP = 0.1f;
            }
        }
        
        ImGui::DragFloat("BBP_Speed", &m_bigProjectileSpeed, 0.1f, 0.1f, 50.0f, "%.1f");
        ImGui::DragFloat("BBP_Scale", &m_bigProjectileScale, 0.1f, 0.1f, 10.0f, "%.1f");
        ImGui::DragFloat("BBP_Lifetime (sec)", &m_bigProjectileLifetime, 0.1f, 1.0f, 30.0f, "%.1f");
        ImGui::DragFloat("BBP_Damage", &m_bigProjectileDamage, 1.0f, 1.0f, 500.0f, "%.1f");

        ImGui::Spacing();

        // ─────────────────────────────────────────────
        // 발사 위치 오프셋
        // ─────────────────────────────────────────────
        ImGui::Text("--- Spawn Offset ---");
        float spawnOffset[3] = { m_bigProjectileSpawnOffsetX, m_bigProjectileSpawnOffsetY, m_bigProjectileSpawnOffsetZ };
        if (ImGui::DragFloat3("BBP_Spawn Offset (XYZ)", spawnOffset, 0.1f, -50.0f, 50.0f, "%.2f"))
        {
            m_bigProjectileSpawnOffsetX = spawnOffset[0];
            m_bigProjectileSpawnOffsetY = spawnOffset[1];
            m_bigProjectileSpawnOffsetZ = spawnOffset[2];
        }

        ImGui::Spacing();

        // ─────────────────────────────────────────────
        // 반사 설정
        // ─────────────────────────────────────────────
        ImGui::Text("--- Reflect Settings ---");
        ImGui::DragFloat("BBP_Reflect Damage", &m_bigProjectileReflectDamage, 1.0f, 1.0f, 1000.0f, "%.1f");
        ImGui::DragFloat("BBP_Reflect Lifetime Extension (sec)", &m_bigProjectileReflectLifetimeExtension, 0.1f, 1.0f, 30.0f, "%.1f");
        
        ImGui::Checkbox("BBP_Shield Pierce", &m_isShieldPierce);
        
        if (ImGui::SliderFloat("BBP_Shield Reduction (%)", &m_bigProjectileShieldReduction, 0.0f, 100.0f, "%.1f%%"))
        {
            m_bigProjectileShieldReduction = std::clamp(m_bigProjectileShieldReduction, 0.0f, 100.0f);
        }

        ImGui::Separator();
    }

    void BossScript::Save(engine::json& j) const
    {
        Object::Save(j);

        // ─────────────────────────────────────────────
        // 보스 기본 설정 저장
        // ─────────────────────────────────────────────
        j["MaxHP"] = m_maxHp;

        // ─────────────────────────────────────────────
        // BulletFire 설정 저장
        // ─────────────────────────────────────────────
        j["BulletFire_UseFixedInterval"] = m_bulletFireUseFixedInterval;
        j["BulletFire_FixedInterval"] = m_bulletFireFixedInterval;
        j["BulletFire_MinInterval"] = m_bulletFireMinInterval;
        j["BulletFire_MaxInterval"] = m_bulletFireMaxInterval;

        j["BulletFire_UseFixedSpread"] = m_bulletFireUseFixedSpread;
        j["BulletFire_FixedSpread"] = m_bulletFireFixedSpread;
        j["BulletFire_MinSpread"] = m_bulletFireMinSpread;
        j["BulletFire_MaxSpread"] = m_bulletFireMaxSpread;

        j["BulletFire_Speed"] = m_bulletFireSpeed;
        j["BulletFire_Scale"] = m_bulletFireScale;
        j["BulletFire_Damage"] = m_bulletFireDamage;
        j["BulletFire_Lifetime"] = m_bulletFireLifetime;
        j["BulletFire_SpawnOffsetX"] = m_bulletFireSpawnOffsetX;
        j["BulletFire_SpawnOffsetZ"] = m_bulletFireSpawnOffsetZ;

        // Meteor 설정 저장
        j["Meteor_UseFixedInterval"] = m_meteorUseFixedInterval;
        j["Meteor_FixedInterval"] = m_meteorFixedInterval;
        j["Meteor_MinInterval"] = m_meteorMinInterval;
        j["Meteor_MaxInterval"] = m_meteorMaxInterval;

        j["Meteor_SpawnHeight"] = m_meteorSpawnHeight;
        j["Meteor_InitialSpeed"] = m_meteorInitialSpeed;
        j["Meteor_OwnGravity"] = m_meteorOwnGravity;
        j["Meteor_LandingY"] = m_meteorLandingY;
        j["Meteor_LandingThreshold"] = m_meteorLandingThreshold;
        j["Meteor_SpawnCenterX"] = m_meteorSpawnCenterX;
        j["Meteor_SpawnCenterZ"] = m_meteorSpawnCenterZ;
        j["Meteor_ValidRangeX"] = m_meteorValidRangeX;
        j["Meteor_ValidRangeZ"] = m_meteorValidRangeZ;
        j["Meteor_PredictionStrength"] = m_meteorPredictionStrength;
        j["Meteor_PredictionAccuracy"] = m_meteorPredictionAccuracy;

        j["Meteor_BulletSpeed"] = m_meteorBulletSpeed;
        j["Meteor_BulletDamage"] = m_meteorBulletDamage;
        j["Meteor_BulletLifetime"] = m_meteorBulletLifetime;
        j["Meteor_BulletScale"] = m_meteorBulletScale;

        j["Meteor_ExplosionDamage"] = m_meteorExplosionDamage;
        j["Meteor_ExplosionRadius"] = m_meteorExplosionRadius;
        j["Meteor_ExplosionLifetime"] = m_meteorExplosionLifetime;

        j["Meteor_Scale"] = m_meteorScale;

        // PillarShield 설정 저장
        j["Pillar_UseBasicPattern"] = m_pillarUseBasicPattern;
        j["Pillar_UseFixedRespawnDelay"] = m_pillarUseFixedRespawnDelay;
        j["Pillar_FixedRespawnDelay"] = m_pillarFixedRespawnDelay;
        j["Pillar_MinRespawnDelay"] = m_pillarMinRespawnDelay;
        j["Pillar_MaxRespawnDelay"] = m_pillarMaxRespawnDelay;
        j["Pillar_UseFixedCount"] = m_pillarUseFixedCount;
        j["Pillar_FixedCount"] = m_pillarFixedCount;
        j["Pillar_MinCount"] = m_pillarMinCount;
        j["Pillar_MaxCount"] = m_pillarMaxCount;
        j["Pillar_SpawnCenterX"] = m_pillarSpawnCenterX;
        j["Pillar_SpawnCenterZ"] = m_pillarSpawnCenterZ;
        j["Pillar_SpawnRangeX"] = m_pillarSpawnRangeX;
        j["Pillar_SpawnRangeZ"] = m_pillarSpawnRangeZ;
        j["Pillar_SpawnY"] = m_pillarSpawnY;
        j["Pillar_ClusteringStrength"] = m_pillarClusteringStrength;
        j["Pillar_OverlapRadius"] = m_pillarOverlapRadius;
        j["Pillar_MaxSpawnAttempts"] = m_pillarMaxSpawnAttempts;
        j["Pillar_HP"] = m_pillarHP;

        // BigProjectile 패턴 설정
        j["BBP_HP"] = m_bigProjectileHP;
        j["BBP_Speed"] = m_bigProjectileSpeed;
        j["BBP_Scale"] = m_bigProjectileScale;
        j["BBP_Lifetime"] = m_bigProjectileLifetime;
        j["BBP_Damage"] = m_bigProjectileDamage;
        j["BBP_SpawnOffsetX"] = m_bigProjectileSpawnOffsetX;
        j["BBP_SpawnOffsetY"] = m_bigProjectileSpawnOffsetY;
        j["BBP_SpawnOffsetZ"] = m_bigProjectileSpawnOffsetZ;
        j["BBP_ReflectDamage"] = m_bigProjectileReflectDamage;
        j["BBP_ReflectLifetimeExtension"] = m_bigProjectileReflectLifetimeExtension;
        j["BBP_ShieldReduction"] = m_bigProjectileShieldReduction;
        j["BBP_ShieldPierce"] = m_isShieldPierce;
    }

    void BossScript::Load(const engine::json& j)
    {
        Object::Load(j);

        // ─────────────────────────────────────────────
        // 보스 기본 설정 로드
        // ─────────────────────────────────────────────
        if (j.contains("MaxHP"))
        {
            m_maxHp = std::clamp(j["MaxHP"].get<float>(), 1.0f, 100000.0f);
            
            // 현재 HP가 최대 HP를 초과하지 않도록
            if (m_currentHp > m_maxHp)
            {
                m_currentHp = m_maxHp;
            }
        }

        // ─────────────────────────────────────────────
        // BulletFire 설정 로드 (클램핑 적용)
        // ─────────────────────────────────────────────
        if (j.contains("BulletFire_UseFixedInterval"))
            m_bulletFireUseFixedInterval = j["BulletFire_UseFixedInterval"].get<bool>();
        if (j.contains("BulletFire_FixedInterval"))
            m_bulletFireFixedInterval = std::clamp(j["BulletFire_FixedInterval"].get<float>(), 0.1f, 100.0f);
        if (j.contains("BulletFire_MinInterval"))
            m_bulletFireMinInterval = std::clamp(j["BulletFire_MinInterval"].get<float>(), 0.1f, 100.0f);
        if (j.contains("BulletFire_MaxInterval"))
            m_bulletFireMaxInterval = std::clamp(j["BulletFire_MaxInterval"].get<float>(), 0.1f, 100.0f);

        if (j.contains("BulletFire_UseFixedSpread"))
            m_bulletFireUseFixedSpread = j["BulletFire_UseFixedSpread"].get<bool>();
        if (j.contains("BulletFire_FixedSpread"))
            m_bulletFireFixedSpread = std::clamp(j["BulletFire_FixedSpread"].get<float>(), 5.0f, 80.0f);
        if (j.contains("BulletFire_MinSpread"))
            m_bulletFireMinSpread = std::clamp(j["BulletFire_MinSpread"].get<float>(), 5.0f, 80.0f);
        if (j.contains("BulletFire_MaxSpread"))
            m_bulletFireMaxSpread = std::clamp(j["BulletFire_MaxSpread"].get<float>(), 5.0f, 80.0f);

        if (j.contains("BulletFire_Speed"))
            m_bulletFireSpeed = j["BulletFire_Speed"].get<float>();
        if (j.contains("BulletFire_Scale"))
            m_bulletFireScale = j["BulletFire_Scale"].get<float>();
        if (j.contains("BulletFire_Damage"))
            m_bulletFireDamage = j["BulletFire_Damage"].get<float>();
        if (j.contains("BulletFire_Lifetime"))
            m_bulletFireLifetime = std::clamp(j["BulletFire_Lifetime"].get<float>(), 0.1f, 20.0f);
        if (j.contains("BulletFire_SpawnOffsetX"))
            m_bulletFireSpawnOffsetX = j["BulletFire_SpawnOffsetX"].get<float>();
        if (j.contains("BulletFire_SpawnOffsetZ"))
            m_bulletFireSpawnOffsetZ = j["BulletFire_SpawnOffsetZ"].get<float>();

        // 최소 <= 최대 보장
        if (m_bulletFireMinInterval > m_bulletFireMaxInterval)
            m_bulletFireMaxInterval = m_bulletFireMinInterval;
        if (m_bulletFireMinSpread > m_bulletFireMaxSpread)
            m_bulletFireMaxSpread = m_bulletFireMinSpread;

        // Meteor 설정 로드 (클램핑 적용)
        if (j.contains("Meteor_UseFixedInterval"))
            m_meteorUseFixedInterval = j["Meteor_UseFixedInterval"].get<bool>();
        if (j.contains("Meteor_FixedInterval"))
            m_meteorFixedInterval = std::clamp(j["Meteor_FixedInterval"].get<float>(), 0.1f, 100.0f);
        if (j.contains("Meteor_MinInterval"))
            m_meteorMinInterval = std::clamp(j["Meteor_MinInterval"].get<float>(), 0.1f, 100.0f);
        if (j.contains("Meteor_MaxInterval"))
            m_meteorMaxInterval = std::clamp(j["Meteor_MaxInterval"].get<float>(), 0.1f, 100.0f);

        if (j.contains("Meteor_SpawnHeight"))
            m_meteorSpawnHeight = j["Meteor_SpawnHeight"].get<float>();
        if (j.contains("Meteor_InitialSpeed"))
            m_meteorInitialSpeed = j["Meteor_InitialSpeed"].get<float>();
        if (j.contains("Meteor_OwnGravity"))
            m_meteorOwnGravity = j["Meteor_OwnGravity"].get<float>();
        if (j.contains("Meteor_LandingY"))
            m_meteorLandingY = j["Meteor_LandingY"].get<float>();
        if (j.contains("Meteor_LandingThreshold"))
            m_meteorLandingThreshold = j["Meteor_LandingThreshold"].get<float>();
        if (j.contains("Meteor_SpawnCenterX"))
            m_meteorSpawnCenterX = j["Meteor_SpawnCenterX"].get<float>();
        if (j.contains("Meteor_SpawnCenterZ"))
            m_meteorSpawnCenterZ = j["Meteor_SpawnCenterZ"].get<float>();
        if (j.contains("Meteor_ValidRangeX"))
            m_meteorValidRangeX = j["Meteor_ValidRangeX"].get<float>();
        if (j.contains("Meteor_ValidRangeZ"))
            m_meteorValidRangeZ = j["Meteor_ValidRangeZ"].get<float>();
        if (j.contains("Meteor_PredictionStrength"))
            m_meteorPredictionStrength = std::clamp(j["Meteor_PredictionStrength"].get<float>(), 0.0f, 1.0f);
        if (j.contains("Meteor_PredictionAccuracy"))
            m_meteorPredictionAccuracy = std::clamp(j["Meteor_PredictionAccuracy"].get<float>(), 0.0f, 10.0f);

        if (j.contains("Meteor_BulletSpeed"))
            m_meteorBulletSpeed = j["Meteor_BulletSpeed"].get<float>();
        if (j.contains("Meteor_BulletDamage"))
            m_meteorBulletDamage = j["Meteor_BulletDamage"].get<float>();
        if (j.contains("Meteor_BulletLifetime"))
            m_meteorBulletLifetime = std::clamp(j["Meteor_BulletLifetime"].get<float>(), 0.1f, 20.0f);
        if (j.contains("Meteor_BulletScale"))
            m_meteorBulletScale = j["Meteor_BulletScale"].get<float>();

        if (j.contains("Meteor_ExplosionDamage"))
            m_meteorExplosionDamage = j["Meteor_ExplosionDamage"].get<float>();
        if (j.contains("Meteor_ExplosionRadius"))
            m_meteorExplosionRadius = j["Meteor_ExplosionRadius"].get<float>();
        if (j.contains("Meteor_ExplosionLifetime"))
            m_meteorExplosionLifetime = j["Meteor_ExplosionLifetime"].get<float>();

        if (j.contains("Meteor_Scale"))
            m_meteorScale = j["Meteor_Scale"].get<float>();

        // 최소 <= 최대 보장 (Meteor)
        if (m_meteorMinInterval > m_meteorMaxInterval)
            m_meteorMaxInterval = m_meteorMinInterval;

        // PillarShield 설정 로드
        if (j.contains("Pillar_UseBasicPattern"))
            m_pillarUseBasicPattern = j["Pillar_UseBasicPattern"].get<bool>();
        if (j.contains("Pillar_UseFixedRespawnDelay"))
            m_pillarUseFixedRespawnDelay = j["Pillar_UseFixedRespawnDelay"].get<bool>();
        if (j.contains("Pillar_FixedRespawnDelay"))
            m_pillarFixedRespawnDelay = std::clamp(j["Pillar_FixedRespawnDelay"].get<float>(), 1.0f, 20.0f);
        if (j.contains("Pillar_MinRespawnDelay"))
            m_pillarMinRespawnDelay = std::clamp(j["Pillar_MinRespawnDelay"].get<float>(), 1.0f, 20.0f);
        if (j.contains("Pillar_MaxRespawnDelay"))
            m_pillarMaxRespawnDelay = std::clamp(j["Pillar_MaxRespawnDelay"].get<float>(), 1.0f, 20.0f);
        if (j.contains("Pillar_UseFixedCount"))
            m_pillarUseFixedCount = j["Pillar_UseFixedCount"].get<bool>();
        if (j.contains("Pillar_FixedCount"))
            m_pillarFixedCount = std::clamp(j["Pillar_FixedCount"].get<int>(), 1, 10);
        if (j.contains("Pillar_MinCount"))
            m_pillarMinCount = std::clamp(j["Pillar_MinCount"].get<int>(), 1, 10);
        if (j.contains("Pillar_MaxCount"))
            m_pillarMaxCount = std::clamp(j["Pillar_MaxCount"].get<int>(), 1, 10);
        if (j.contains("Pillar_SpawnCenterX"))
            m_pillarSpawnCenterX = j["Pillar_SpawnCenterX"].get<float>();
        if (j.contains("Pillar_SpawnCenterZ"))
            m_pillarSpawnCenterZ = j["Pillar_SpawnCenterZ"].get<float>();
        if (j.contains("Pillar_SpawnRangeX"))
            m_pillarSpawnRangeX = j["Pillar_SpawnRangeX"].get<float>();
        if (j.contains("Pillar_SpawnRangeZ"))
            m_pillarSpawnRangeZ = j["Pillar_SpawnRangeZ"].get<float>();
        if (j.contains("Pillar_SpawnY"))
            m_pillarSpawnY = j["Pillar_SpawnY"].get<float>();
        if (j.contains("Pillar_ClusteringStrength"))
            m_pillarClusteringStrength = std::clamp(j["Pillar_ClusteringStrength"].get<float>(), 0.0f, 1.0f);
        if (j.contains("Pillar_OverlapRadius"))
            m_pillarOverlapRadius = j["Pillar_OverlapRadius"].get<float>();
        if (j.contains("Pillar_MaxSpawnAttempts"))
            m_pillarMaxSpawnAttempts = j["Pillar_MaxSpawnAttempts"].get<int>();
        if (j.contains("Pillar_HP"))
            m_pillarHP = j["Pillar_HP"].get<float>();

        // BigProjectile 패턴 설정
        if (j.contains("BBP_HP"))
        {
            m_bigProjectileHP = j["BBP_HP"].get<float>();
            if (m_bigProjectileHP <= 0.0f)
            {
                m_bigProjectileHP = 0.1f;
            }
        }
        if (j.contains("BBP_Speed"))
            m_bigProjectileSpeed = j["BBP_Speed"].get<float>();
        if (j.contains("BBP_Scale"))
            m_bigProjectileScale = j["BBP_Scale"].get<float>();
        if (j.contains("BBP_Lifetime"))
            m_bigProjectileLifetime = j["BBP_Lifetime"].get<float>();
        if (j.contains("BBP_Damage"))
            m_bigProjectileDamage = j["BBP_Damage"].get<float>();
        if (j.contains("BBP_SpawnOffsetX"))
            m_bigProjectileSpawnOffsetX = j["BBP_SpawnOffsetX"].get<float>();
        if (j.contains("BBP_SpawnOffsetY"))
            m_bigProjectileSpawnOffsetY = j["BBP_SpawnOffsetY"].get<float>();
        if (j.contains("BBP_SpawnOffsetZ"))
            m_bigProjectileSpawnOffsetZ = j["BBP_SpawnOffsetZ"].get<float>();
        if (j.contains("BBP_ReflectDamage"))
            m_bigProjectileReflectDamage = j["BBP_ReflectDamage"].get<float>();
        if (j.contains("BBP_ReflectLifetimeExtension"))
            m_bigProjectileReflectLifetimeExtension = j["BBP_ReflectLifetimeExtension"].get<float>();
        if (j.contains("BBP_ShieldReduction"))
            m_bigProjectileShieldReduction = std::clamp(j["BBP_ShieldReduction"].get<float>(), 0.0f, 100.0f);
        if (j.contains("BBP_ShieldPierce"))
            m_isShieldPierce = j["BBP_ShieldPierce"].get<bool>();

        // 최소 <= 최대 보장 (Pillar)
        if (m_pillarMinRespawnDelay > m_pillarMaxRespawnDelay)
            m_pillarMaxRespawnDelay = m_pillarMinRespawnDelay;
        if (m_pillarMinCount > m_pillarMaxCount)
            m_pillarMaxCount = m_pillarMinCount;
    }
}
