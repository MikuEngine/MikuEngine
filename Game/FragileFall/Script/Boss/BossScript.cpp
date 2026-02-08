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

    void BossScript::TakeDamage(float damage)
    {
        if (m_isShieldActive)
        {
            // 쉴드가 활성화되어 있으면 데미지 차단
            return;
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
        // TODO: 처형 반사 시 큰 데미지 처리
        TakeDamage(200);
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

    void BossScript::OnGui()
    {
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
        ImGui::Text("--- Valid Spawn Range (XZ) ---");
        ImGui::DragFloat("Valid Range X (±)", &m_meteorValidRangeX, 0.5f, 1.0f, 100.0f, "%.1f");
        ImGui::DragFloat("Valid Range Z (±)", &m_meteorValidRangeZ, 0.5f, 1.0f, 100.0f, "%.1f");

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
    }

    void BossScript::Save(engine::json& j) const
    {
        Object::Save(j);

        // BulletFire 설정 저장
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
    }

    void BossScript::Load(const engine::json& j)
    {
        Object::Load(j);

        // BulletFire 설정 로드 (클램핑 적용)
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
    }
}
