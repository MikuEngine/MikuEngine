#include "GamePCH.h"
#include "BossScript.h"

#include <Framework/Object/Component/Renderer/StaticMeshRenderer.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>

#include "Script/Boss/BossScript.h"
#include "Script/Boss/BossPattern/BossPatternManager.h"
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
    }

    void BossScript::Start()
    {
        InitializeCrystalMeshes();
        InitializePatterns();

        auto playerGo = engine::GameObject::Find("Player");
        if (playerGo != nullptr)
        {
            m_targetPlayer = playerGo->GetComponent<PlayerControllerScript>();
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

    void BossScript::OnGui()
    {
    }

    void BossScript::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void BossScript::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}
