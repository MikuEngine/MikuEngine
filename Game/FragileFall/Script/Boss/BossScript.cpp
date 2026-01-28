#include "GamePCH.h"
#include "BossScript.h"

#include <Framework/Object/Component/Renderer/StaticMeshRenderer.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>

#include "Script/Boss/BossScript.h"
#include "Script/Boss/BossPattern/BossPatternManager.h"
#include "Script/Boss/BossPattern/Components/BossPillar.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

namespace game
{
    BossScript::BossScript() = default;
    BossScript::~BossScript() = default;

    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void BossScript::Awake()
    {
    }

    void BossScript::Start()
    {
        CacheComponents();
        InitializeCrystalMeshes();
        InitializePatterns();

        // 플레이어 찾기
        auto* scene = engine::SceneManager::Get().GetScene();
        if (scene)
        {
            if (auto* playerGO = scene->FindGameObject(m_targetPlayerObjectName))
            {
                if (auto* player = playerGO->GetComponent<PlayerControllerScript>())
                {
                    m_targetPlayer = player;
                }
            }

            if (!m_targetPlayer)
            {
                for (const auto& go : scene->GetGameObjects())
                {
                    if (auto* player = go->GetComponent<PlayerControllerScript>())
                    {
                        m_targetPlayer = player;
                        break;
                    }
                }
            }
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

    // ═══════════════════════════════════════════════════════════════
    // 초기화
    // ═══════════════════════════════════════════════════════════════
    void BossScript::CacheComponents()
    {
        if (!GetGameObject()) return;

        m_mainTransform = GetGameObject()->GetComponent<engine::Transform>();

        // 수정 메쉬들 찾기 (자식 GameObject에서도 검색)
        // TODO: 실제 구조에 맞게 수정 필요
        auto* meshRenderer = GetGameObject()->GetComponent<engine::StaticMeshRenderer>();
        if (meshRenderer)
        {
            m_crystalMeshes.push_back(meshRenderer);
        }
    }

    void BossScript::InitializePatterns()
    {
        m_patternManager = std::make_unique<BossPatternManager>();
        m_patternManager->Initialize(this);

        // TODO: 패턴 등록은 자식 클래스에서 구현
        // InitializePatterns()를 가상 함수로 만들거나 자식에서 오버라이드
    }

    void BossScript::InitializeCrystalMeshes()
    {
        // TODO: 수정 메쉬들 초기화 및 회전 설정
        // 실제 구조에 맞게 구현 필요
    }

    // ═══════════════════════════════════════════════════════════════
    // 패턴 시스템
    // ═══════════════════════════════════════════════════════════════
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

    // ═══════════════════════════════════════════════════════════════
    // 체력 관리
    // ═══════════════════════════════════════════════════════════════
    void BossScript::TakeDamage(int damage)
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

    // ═══════════════════════════════════════════════════════════════
    // 쉴드 시스템
    // ═══════════════════════════════════════════════════════════════
    void BossScript::UpdateShieldStatus()
    {
        // 기둥 리스트에서 파괴된 기둥 제거
        m_activePillars.erase(
            std::remove_if(m_activePillars.begin(), m_activePillars.end(),
                [](const engine::Ptr<BossPillar>& pillar) {
                    if (!pillar) return true;
                    return pillar->IsDestroyed();
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

    // ═══════════════════════════════════════════════════════════════
    // 색상 시스템
    // ═══════════════════════════════════════════════════════════════
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

    // ═══════════════════════════════════════════════════════════════
    // 결정화/처형 시스템
    // ═══════════════════════════════════════════════════════════════
    void BossScript::OnCrystallized()
    {
        // TODO: 결정화 상태 진입 처리
    }

    void BossScript::OnExecutionReflected(engine::Vector3 direction)
    {
        // TODO: 처형 반사 시 큰 데미지 처리
        TakeDamage(200);
    }

    // ═══════════════════════════════════════════════════════════════
    // 간단한 움직임 (회전 등)
    // ═══════════════════════════════════════════════════════════════
    void BossScript::UpdateCrystalMovement(float deltaTime)
    {
        if (!m_mainTransform) return;

        // 메인 수정 주변을 도는 작은 수정들
        for (size_t i = 0; i < m_crystalMeshes.size(); ++i)
        {
            if (m_crystalMeshes[i] && m_mainTransform)
            {
                float angle = static_cast<float>(i) * (360.0f / m_crystalMeshes.size()) + deltaTime * 30.0f;  // 초당 30도 회전
                float radius = 2.0f;

                float angleRad = engine::ToRadian(angle);
                engine::Vector3 offset(std::cosf(angleRad) * radius, 0.0f, std::sinf(angleRad) * radius);

                auto* meshTransform = m_crystalMeshes[i]->GetTransform();
                if (meshTransform)
                {
                    meshTransform->SetLocalPosition(m_mainTransform->GetWorldPosition() + offset);
                }
            }
        }
    }
}
