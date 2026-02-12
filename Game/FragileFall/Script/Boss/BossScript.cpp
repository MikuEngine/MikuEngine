#include "GamePCH.h"
#include "BossScript.h"

#include <Framework/Object/Component/Renderer/StaticMeshRenderer.h>
#include <Framework/Object/Component/Camera.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Object/Component/UI/UIProgressBar.h>
#include <Framework/Object/Component/Particle/ParticleEffect.h>
#include <Common/Math/MathUtility.h>
#include <Framework/Asset/Prefab.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_set>

#include "Script/Boss/BossPattern/BossPatternManager.h"
#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/Boss/BossPattern/Patterns/BossPattern_PillarShield.h"
#include "Script/Boss/BossPattern/Patterns/BossPattern_BulletFire.h"
#include "Script/Boss/BossPattern/Patterns/BossPattern_Meteor.h"
#include "Script/Boss/BossPattern/Patterns/BossPattern_Summon.h"
#include "Script/Boss/BossPattern/Patterns/BossPattern_SphereProjectile.h"
#include "Script/Boss/BossPattern/Components/BossPillar.h"
#include "Script/Boss/BossPattern/Components/BossShieldEffect.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/Boss/BossSubPartsController.h"
#include "Script/AimModeController.h"
#include "Scene/GameScene.h"

namespace game
{
    namespace
    {
        engine::Vector3 ToLocalPosition(engine::Transform* transform, const engine::Vector3& worldPosition)
        {
            if (!transform)
            {
                return worldPosition;
            }

            engine::Transform* parent = transform->GetParent();
            if (!parent)
            {
                return worldPosition;
            }

            const engine::Matrix parentWorldInv = parent->GetWorld().Invert();
            return engine::Vector3::Transform(worldPosition, parentWorldInv);
        }

        engine::Quaternion ToLocalRotation(engine::Transform* transform, const engine::Quaternion& worldRotation)
        {
            if (!transform)
            {
                return worldRotation;
            }

            engine::Transform* parent = transform->GetParent();
            if (!parent)
            {
                return worldRotation;
            }

            const engine::Quaternion parentWorldRot = parent->GetWorldRotation();
            engine::Quaternion invParentWorldRot;
            parentWorldRot.Inverse(invParentWorldRot);
            return invParentWorldRot * worldRotation;
        }
    }

    BossScript::BossScript() = default;
    BossScript::~BossScript() = default;

    void BossScript::Awake()
    {
        auto* hpBarObject = engine::GameObject::Find("BossHPBar");
        if (!hpBarObject)
        {
            LOG_PRINT("[BossScript] WARNING: BossHPBar GameObject not found.");
            return;
        }

        m_bossHpBarObject = hpBarObject;
        m_bossHpBar = hpBarObject->GetComponent<engine::UIProgressBar>();
        if (!m_bossHpBar)
        {
            LOG_PRINT("[BossScript] WARNING: UIProgressBar not found on BossHPBar.");
        }
    }

    void BossScript::Start()
    {
        m_patternsInitialized = false;
        UpdateBossHpBarValue();
        SetBossHpBarVisible(false);

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

        CacheIntroReferences();
        if (m_cameraTransform)
        {
            m_hasSceneStartCameraPose = true;
            m_sceneStartCameraWorldPosition = m_cameraTransform->GetWorldPosition();
            m_sceneStartCameraWorldRotation = m_cameraTransform->GetWorldRotation();
        }

        if (m_enableIntroSequence)
        {
            BeginIntroSequence();
        }
        else
        {
            m_isBattleStarted = true;
        }

        if (m_isBattleStarted && !m_patternsInitialized)
        {
            InitializePatterns();
            m_patternsInitialized = true;
        }

        if (m_isBattleStarted)
        {
            SetBossHpBarVisible(true);
        }
    }

    void BossScript::Update()
    {
        float deltaTime = engine::Time::DeltaTime();

        if (m_deathSequenceActive)
        {
            UpdateDeathSequence(deltaTime);
            return;
        }

        if (m_isIntroRunning)
        {
            UpdateIntroSequence(deltaTime);
        }

        UpdateShieldStatus();
        if (m_isBattleStarted)
        {
            UpdatePatternSystem(deltaTime);
        }
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
        UpdateBossHpBarValue();
    }

    void BossScript::CheckHealth()
    {
        if (!m_isDead && m_currentHp <= 0)
        {
            OnDeath();
        }

        if (engine::Input::IsKeyPressed(engine::Keys::D6))
        {
            OnDeath();
        }
    }

    void BossScript::OnDeath()
    {
        if (m_isDead)
            return;

        m_isDead = true;
        BeginDeathSequence();
    }

    void BossScript::RegisterMapCrystalForDeath(engine::GameObject* crystal)
    {
        if (!crystal)
            return;

        for (auto& item : m_registeredMapCrystals)
        {
            if (item.object.Get() == crystal)
                return;
        }

        RegisteredMapCrystal item;
        item.object = crystal;
        item.burst = false;
        if (crystal->GetTransform())
            item.zSort = crystal->GetTransform()->GetWorldPosition().z;
        m_registeredMapCrystals.push_back(item);
    }

    void BossScript::UnregisterMapCrystalForDeath(engine::GameObject* crystal)
    {
        if (!crystal)
            return;

        m_registeredMapCrystals.erase(
            std::remove_if(m_registeredMapCrystals.begin(), m_registeredMapCrystals.end(),
                [crystal](const RegisteredMapCrystal& item)
                {
                    return item.object.Get() == crystal;
                }),
            m_registeredMapCrystals.end());
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

    float BossScript::EaseInOutSine(float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);
        return -(std::cos(DirectX::XM_PI * t) - 1.0f) * 0.5f;
    }

    void BossScript::CacheIntroReferences()
    {
        m_subPartsController = nullptr;
        m_subPartsRoot = nullptr;
        m_targetAimController = nullptr;
        m_mainCamera = nullptr;
        m_cameraTransform = nullptr;

        if (auto* cameraGO = engine::GameObject::Find("MainCamera"))
        {
            m_mainCamera = cameraGO->GetComponent<engine::Camera>();
            m_cameraTransform = cameraGO->GetTransform();
        }

        if (m_targetPlayer)
        {
            m_targetAimController = m_targetPlayer->GetGameObject()->GetComponent<AimModeController>();
        }

        // BossSubParts root (can be disabled in editor).
        if (auto* bossSubPartsObject = GetTransform()->FindChildByNameRecursive("BossSubParts"))
        {
            m_subPartsRoot = bossSubPartsObject;
        }

        // BossSubPartsController is usually attached to the same Boss object.
        m_subPartsController = GetGameObject()->GetComponent<BossSubPartsController>();
        if (!m_subPartsController)
        {
            auto* bossSubPartsObject = m_subPartsRoot.Get();
            if (bossSubPartsObject)
            {
                m_subPartsController = bossSubPartsObject->GetComponent<BossSubPartsController>();
            }
        }
    }

    void BossScript::SetBossHpBarVisible(bool visible)
    {
        if (m_bossHpBarObject)
        {
            m_bossHpBarObject->SetActive(visible);
        }
    }

    void BossScript::UpdateBossHpBarValue()
    {
        if (!m_bossHpBar)
        {
            return;
        }

        const float maxHp = std::max(0.01f, m_maxHp);
        const float ratio = std::clamp(m_currentHp / maxHp, 0.0f, 1.0f);
        m_bossHpBar->SetValue(ratio);
    }

    void BossScript::BeginIntroSequence()
    {
        if (!m_targetPlayer || !m_cameraTransform)
        {
            m_isBattleStarted = true;
            m_isIntroRunning = false;
            m_introPhase = IntroPhase::Complete;
            return;
        }

        m_isBattleStarted = false;
        m_isIntroRunning = true;
        m_introPhase = IntroPhase::MoveToBoss;
        m_introPhaseElapsed = 0.0f;
        m_introAssembleTriggered = false;

        m_targetPlayer->SetControlLocked(true);
        if (m_targetAimController)
        {
            m_targetAimController->SetPaused(true);
        }

        const engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();
        const engine::Vector3 bossPos = GetTransform()->GetWorldPosition();

        if (!m_hasSceneStartCameraPose)
        {
            m_hasSceneStartCameraPose = true;
            m_sceneStartCameraWorldPosition = m_cameraTransform->GetWorldPosition();
            m_sceneStartCameraWorldRotation = m_cameraTransform->GetWorldRotation();
        }

        // Keep local backup for fallback, but intro return target is scene-start world pose.
        m_introCameraOriginalPosition = ToLocalPosition(m_cameraTransform, m_sceneStartCameraWorldPosition);
        m_introCameraOriginalRotation = ToLocalRotation(m_cameraTransform, m_sceneStartCameraWorldRotation);
        m_introCameraStartPosition = playerPos + engine::Vector3(
            m_introCameraPlayerOffsetX,
            m_introCameraPlayerOffsetY,
            m_introCameraPlayerOffsetZ
        );

        engine::Vector3 toBoss = bossPos - playerPos;
        toBoss.y = 0.0f;
        if (toBoss.LengthSquared() < 0.0001f)
        {
            toBoss = engine::Vector3(0.0f, 0.0f, 1.0f);
        }
        else
        {
            toBoss.Normalize();
        }

        m_introCameraBossTargetPosition = bossPos - (toBoss * m_introCameraBossDistance);
        m_introCameraBossTargetPosition.y = m_introCameraBossHeight;
        m_cameraTransform->SetLocalPosition(ToLocalPosition(m_cameraTransform, m_introCameraStartPosition));
        UpdateIntroCameraLookAtBoss();
    }

    void BossScript::UpdateIntroSequence(float deltaTime)
    {
        m_introPhaseElapsed += deltaTime;

        switch (m_introPhase)
        {
        case IntroPhase::MoveToBoss:
        {
            const float duration = std::max(0.01f, m_introMoveToBossDuration);
            const float t = EaseInOutSine(m_introPhaseElapsed / duration);
            const engine::Vector3 pos = m_introCameraStartPosition + ((m_introCameraBossTargetPosition - m_introCameraStartPosition) * t);
            m_cameraTransform->SetLocalPosition(ToLocalPosition(m_cameraTransform, pos));
            UpdateIntroCameraLookAtBoss();

            if (m_introPhaseElapsed >= duration)
            {
                m_introPhase = IntroPhase::HoldAtBoss;
                m_introPhaseElapsed = 0.0f;
                m_cameraTransform->SetLocalPosition(ToLocalPosition(m_cameraTransform, m_introCameraBossTargetPosition));
                UpdateIntroCameraLookAtBoss();
            }
            break;
        }
        case IntroPhase::HoldAtBoss:
        {
            UpdateIntroCameraLookAtBoss();
            if (m_introPhaseElapsed >= std::max(0.0f, m_introHoldDuration))
            {
                m_introPhase = IntroPhase::AssembleParts;
                m_introPhaseElapsed = 0.0f;
            }
            break;
        }
        case IntroPhase::AssembleParts:
        {
            UpdateIntroCameraLookAtBoss();
            if (!m_introAssembleTriggered && m_subPartsController)
            {
                if (m_subPartsRoot)
                {
                    m_subPartsRoot->SetActive(true);
                }
                m_subPartsController->SetIntroAssembleDuration(m_introAssembleDuration);
                m_subPartsController->PrepareIntroAssembly();
                m_introAssembleTriggered = true;
            }

            const bool assembled = (!m_subPartsController) ? true : m_subPartsController->IsIntroAssemblyComplete();
            if (assembled || m_introPhaseElapsed >= std::max(0.01f, m_introAssembleDuration))
            {
                m_introCameraBossTargetPosition = m_cameraTransform->GetWorldPosition();
                m_introCameraBossTargetRotation = m_cameraTransform->GetWorldRotation();
                m_introPhase = IntroPhase::ReturnToPlayer;
                m_introPhaseElapsed = 0.0f;
            }
            break;
        }
        case IntroPhase::ReturnToPlayer:
        {
            const float duration = std::max(0.01f, m_introReturnDuration);
            const float t = EaseInOutSine(m_introPhaseElapsed / duration);
            const engine::Vector3 returnTargetPos = m_hasSceneStartCameraPose ? m_sceneStartCameraWorldPosition : m_cameraTransform->GetWorldPosition();
            const engine::Quaternion returnTargetRot = m_hasSceneStartCameraPose ? m_sceneStartCameraWorldRotation : m_cameraTransform->GetWorldRotation();
            const engine::Vector3 pos = m_introCameraBossTargetPosition + ((returnTargetPos - m_introCameraBossTargetPosition) * t);
            const engine::Quaternion rot = engine::Quaternion::Slerp(m_introCameraBossTargetRotation, returnTargetRot, t);
            m_cameraTransform->SetLocalPosition(ToLocalPosition(m_cameraTransform, pos));
            m_cameraTransform->SetLocalRotation(ToLocalRotation(m_cameraTransform, rot));

            if (m_introPhaseElapsed >= duration)
            {
                EndIntroSequence(true);
            }
            break;
        }
        case IntroPhase::Complete:
        case IntroPhase::None:
        default:
            break;
        }
    }

    void BossScript::EndIntroSequence(bool forceComplete)
    {
        if (forceComplete && m_cameraTransform)
        {
            if (m_hasSceneStartCameraPose)
            {
                m_cameraTransform->SetLocalPosition(ToLocalPosition(m_cameraTransform, m_sceneStartCameraWorldPosition));
                m_cameraTransform->SetLocalRotation(ToLocalRotation(m_cameraTransform, m_sceneStartCameraWorldRotation));
            }
            else
            {
                m_cameraTransform->SetLocalPosition(m_introCameraOriginalPosition);
                m_cameraTransform->SetLocalRotation(m_introCameraOriginalRotation);
            }
        }

        m_isIntroRunning = false;
        m_introPhase = IntroPhase::Complete;
        m_introPhaseElapsed = 0.0f;
        m_isBattleStarted = true;
        SetBossHpBarVisible(true);
        UpdateBossHpBarValue();

        if (!m_patternsInitialized)
        {
            InitializePatterns();
            m_patternsInitialized = true;
        }

        if (m_targetPlayer)
        {
            m_targetPlayer->SetControlLocked(false);
        }
        if (m_targetAimController)
        {
            m_targetAimController->SetPaused(false);
        }
    }

    void BossScript::StartIntroSequence()
    {
        BeginIntroSequence();
    }

    void BossScript::SkipIntroSequence()
    {
        EndIntroSequence(true);
    }

    void BossScript::UpdateIntroCameraLookAtBoss()
    {
        if (!m_cameraTransform)
        {
            return;
        }

        const engine::Vector3 cameraPos = m_cameraTransform->GetWorldPosition();
        const engine::Vector3 bossCorePos = GetTransform()->GetWorldPosition() + m_introLookTargetOffset;
        engine::Vector3 toTarget = bossCorePos - cameraPos;
        if (toTarget.LengthSquared() < 0.0001f)
        {
            return;
        }

        const float yaw = std::atan2(toTarget.x, toTarget.z);
        const float horizontal = std::sqrt((toTarget.x * toTarget.x) + (toTarget.z * toTarget.z));
        const float pitch = -std::atan2(toTarget.y, horizontal);
        const engine::Quaternion worldLookRot = engine::Quaternion::CreateFromYawPitchRoll(yaw, pitch, 0.0f);
        m_cameraTransform->SetLocalRotation(ToLocalRotation(m_cameraTransform, worldLookRot));
    }

    void BossScript::BeginDeathSequence()
    {
        m_deathSequenceActive = true;
        m_deathPhase = DeathPhase::CameraMoveToBoss;
        m_deathPhaseElapsed = 0.0f;
        m_deathPartDustSpawned = false;
        if (m_deathPartDustInstance)
        {
            m_deathPartDustInstance->Destroy();
            m_deathPartDustInstance = nullptr;
        }
        m_isBattleStarted = false;

        // 보스 클리어 연출 중 플레이어 조작 차단
        if (m_targetPlayer)
        {
            m_targetPlayer->SetControlLocked(true);
        }
        if (m_targetAimController)
        {
            m_targetAimController->SetPaused(true);
        }

        SetBossHpBarVisible(false);
        if (m_patternManager)
            m_patternManager.reset();

        // 보스 사망 시 남아있는 기둥 정리
        for (auto& pillar : m_activePillars)
        {
            if (pillar && pillar->GetGameObject())
            {
                pillar->GetGameObject()->Destroy();
            }
        }
        m_activePillars.clear();
        m_isShieldActive = false;

        // m_activePillars 누락 케이스 대비: 씬 전체에서 BossPillar를 강제 정리
        if (auto* scene = engine::SceneManager::Get().GetScene())
        {
            for (const auto& go : scene->GetGameObjects())
            {
                if (!go) continue;
                const std::string& n = go->GetName();
                if (go->GetComponent<BossPillar>() ||
                    n.find("BossPillar") != std::string::npos ||
                    n.find("PillarCrystalizedPiece") != std::string::npos)
                {
                    go->Destroy();
                }
            }
        }

        // 보스 하위 트리 + 씬 전체에서 실드 형태 오브젝트 제거
        auto isShieldName = [](const std::string& name) -> bool
        {
            std::string lower = name;
            for (char& c : lower)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return lower.find("shield") != std::string::npos;
        };

        if (auto* rootTr = GetTransform())
        {
            std::vector<engine::Transform*> stack;
            stack.push_back(rootTr);
            while (!stack.empty())
            {
                engine::Transform* cur = stack.back();
                stack.pop_back();
                if (!cur) continue;

                const auto& children = cur->GetChildren();
                for (auto* child : children)
                {
                    if (!child) continue;
                    stack.push_back(child);

                    auto* childGO = child->GetGameObject();
                    if (!childGO) continue;

                    if (childGO->GetComponent<BossShieldEffect>() || isShieldName(childGO->GetName()))
                    {
                        childGO->Destroy();
                    }
                }
            }
        }

        if (auto* scene = engine::SceneManager::Get().GetScene())
        {
            for (const auto& go : scene->GetGameObjects())
            {
                if (!go) continue;
                if (go->GetComponent<BossShieldEffect>() || isShieldName(go->GetName()))
                {
                    go->Destroy();
                }
            }
        }

        if (m_subPartsController)
            m_subPartsController->SetActive(false);

        if (m_cameraTransform)
        {
            m_deathCamStartPos = m_cameraTransform->GetWorldPosition();
        }

        const engine::Vector3 bossPos = GetTransform()->GetWorldPosition();

        // 카메라는 항상 보스 기준 월드 -Z 쪽으로 이동
        const engine::Vector3 minusZ(0.0f, 0.0f, -1.0f);
        m_deathCamBossPos = bossPos + (minusZ * m_deathCamBossDistance);
        m_deathCamBossPos.y = m_deathCamBossHeight;

        // 카메라는 연출 끝에 기존 시작 위치로 복귀
        m_deathCamEndPos = m_deathCamStartPos;

        CollectDeathPartsFromHierarchy();
        PrepareRegisteredMapCrystalSort();
    }

    void BossScript::UpdateDeathCameraLookAtBoss()
    {
        if (!m_cameraTransform)
            return;

        const engine::Vector3 cameraPos = m_cameraTransform->GetWorldPosition();
        const engine::Vector3 lookTarget = GetTransform()->GetWorldPosition() + m_deathCamLookAtOffset;
        engine::Vector3 toTarget = lookTarget - cameraPos;
        if (toTarget.LengthSquared() < 0.0001f)
            return;

        const float yaw = std::atan2(toTarget.x, toTarget.z);
        const float horizontal = std::sqrt((toTarget.x * toTarget.x) + (toTarget.z * toTarget.z));
        const float pitch = -std::atan2(toTarget.y, horizontal);
        const engine::Quaternion worldLookRot = engine::Quaternion::CreateFromYawPitchRoll(yaw, pitch, 0.0f);
        m_cameraTransform->SetLocalRotation(ToLocalRotation(m_cameraTransform, worldLookRot));
    }

    void BossScript::CollectDeathPartsFromHierarchy()
    {
        m_rotatingPartsForDeath.clear();
        m_remainingPartsForDeath.clear();

        std::unordered_set<engine::Transform*> visited;

        auto pushPartsRecursive = [this, &visited](engine::Transform* rootTr, std::vector<DeathPartRuntime>& outParts, float startOffset)
        {
            if (!rootTr) return;

            std::vector<engine::Transform*> stack;
            stack.push_back(rootTr);

            size_t pushedCount = 0;
            while (!stack.empty())
            {
                engine::Transform* cur = stack.back();
                stack.pop_back();
                if (!cur) continue;

                const auto& children = cur->GetChildren();
                for (auto* child : children)
                {
                    if (!child) continue;
                    if (!visited.insert(child).second) continue;

                    DeathPartRuntime p;
                    p.transform = child;
                    p.baseLocalPos = child->GetLocalPosition();
                    p.startDelay = startOffset + (static_cast<float>(pushedCount) * m_deathPartDelayStep);
                    p.started = false;
                    outParts.push_back(p);
                    ++pushedCount;

                    stack.push_back(child);
                }
            }
        };

        if (auto* rotatingRoot = GetTransform()->FindChildByNameRecursive("RotatingParts"))
        {
            pushPartsRecursive(rotatingRoot->GetTransform(), m_rotatingPartsForDeath, 0.0f);
        }
        if (auto* floatingRoot = GetTransform()->FindChildByNameRecursive("FloatingParts"))
        {
            pushPartsRecursive(floatingRoot->GetTransform(), m_remainingPartsForDeath, 0.0f);
        }
        if (auto* nestRoot = GetTransform()->FindChildByNameRecursive("NestParts"))
        {
            pushPartsRecursive(nestRoot->GetTransform(), m_remainingPartsForDeath,
                static_cast<float>(m_remainingPartsForDeath.size()) * m_deathPartDelayStep);
        }
    }

    void BossScript::UpdateDeathPartDrops(std::vector<DeathPartRuntime>& parts, float deltaTime)
    {
        for (auto& p : parts)
        {
            if (!p.transform) continue;
            if (m_deathPhaseElapsed < p.startDelay) continue;

            p.started = true;
            engine::Vector3 pos = p.transform->GetLocalPosition();

            // 부들부들 떨림
            const float shakeX = std::sin((m_deathPhaseElapsed + p.startDelay) * m_deathShakeSpeed) * m_deathShakeAmount;
            const float shakeZ = std::cos((m_deathPhaseElapsed + p.startDelay) * (m_deathShakeSpeed * 1.1f)) * m_deathShakeAmount;
            pos.x = p.baseLocalPos.x + shakeX;
            pos.z = p.baseLocalPos.z + shakeZ;

            // Y축 아래로 드롭
            pos.y -= (m_deathPartDropSpeed * deltaTime);
            p.transform->SetLocalPosition(pos);
        }
    }

    void BossScript::SpawnPartDropDustIfNeeded()
    {
        if (m_deathPartDustSpawned || m_deathPartDropDustParticlePrefab.empty())
            return;

        auto effect = engine::Prefab::Instantiate(m_deathPartDropDustParticlePrefab);
        if (!effect || !effect->GetTransform())
            return;

        engine::Vector3 dustPos = GetTransform()->GetWorldPosition();
        dustPos.y = m_deathPartDustGroundY;
        dustPos.z += m_deathPartDustOffsetZ;
        effect->GetTransform()->SetLocalPosition(ToLocalPosition(effect->GetTransform(), dustPos));
        m_deathPartDustInstance = effect;
        m_deathPartDustSpawned = true;
    }

    void BossScript::StopPartDropDustNaturally()
    {
        if (!m_deathPartDustInstance)
            return;

        if (auto* pe = m_deathPartDustInstance->GetComponent<engine::ParticleEffect>())
        {
            pe->SetAutoDestroy(true);
            pe->Stop();
        }
        else
        {
            m_deathPartDustInstance->Destroy();
        }
        m_deathPartDustInstance = nullptr;
    }

    void BossScript::StopAndHideBossRenderers()
    {
        if (auto* renderer = GetGameObject()->GetComponent<engine::StaticMeshRenderer>())
        {
            renderer->SetActive(false);
        }
    }

    void BossScript::BurstBossCore()
    {
        StopAndHideBossRenderers();

        if (!m_deathCoreBurstParticlePrefab.empty())
        {
            const engine::Vector3 center = GetTransform()->GetWorldPosition() + engine::Vector3(0.0f, m_deathCoreBurstOffsetY, 0.0f);

            auto spawnBurst = [this, &center](float yMin, float yMax)
            {
                auto extra = engine::Prefab::Instantiate(m_deathCoreBurstParticlePrefab);
                if (!extra || !extra->GetTransform())
                    return;

                const float angle = engine::Random::Float(0.0f, DirectX::XM_2PI);
                // 원형 면적에 고르게 퍼지도록 sqrt(rand) 사용
                const float radius = std::sqrt(engine::Random::Float(0.0f, 1.0f)) * std::max(0.01f, m_deathCoreBurstRadius);
                const engine::Vector3 offset(
                    std::cos(angle) * radius,
                    engine::Random::Float(std::min(yMin, yMax), std::max(yMin, yMax)),
                    std::sin(angle) * radius);
                const engine::Vector3 spawnPos = center + offset;
                extra->GetTransform()->SetLocalPosition(ToLocalPosition(extra->GetTransform(), spawnPos));
                extra->GetTransform()->SetLocalScale(engine::Vector3(
                    m_deathDestroyParticleScale,
                    m_deathDestroyParticleScale,
                    m_deathDestroyParticleScale));
            };

            for (int i = 0; i < std::max(0, m_deathCoreBurstUpperCount); ++i)
            {
                spawnBurst(m_deathCoreBurstUpperYMin, m_deathCoreBurstUpperYMax);
            }

            for (int i = 0; i < std::max(0, m_deathCoreBurstLowerCount); ++i)
            {
                spawnBurst(m_deathCoreBurstLowerYMin, m_deathCoreBurstLowerYMax);
            }
        }
    }

    void BossScript::BurstMapCrystal(RegisteredMapCrystal& crystal)
    {
        engine::GameObject* go = crystal.object.Get();
        if (!go || crystal.burst)
            return;

        crystal.burst = true;
        if (auto* smr = go->GetComponent<engine::StaticMeshRenderer>())
            smr->SetActive(false);

        if (!m_deathMapCrystalBurstParticlePrefab.empty())
        {
            auto effect = engine::Prefab::Instantiate(m_deathMapCrystalBurstParticlePrefab);
            if (effect && effect->GetTransform() && go->GetTransform())
            {
                engine::Vector3 pos = go->GetTransform()->GetWorldPosition();
                pos.y += m_deathMapCrystalBurstOffsetY;
                effect->GetTransform()->SetLocalPosition(ToLocalPosition(effect->GetTransform(), pos));
                effect->GetTransform()->SetLocalScale(engine::Vector3(
                    m_deathDestroyParticleScale,
                    m_deathDestroyParticleScale,
                    m_deathDestroyParticleScale));
            }
        }
    }

    void BossScript::PrepareRegisteredMapCrystalSort()
    {
        for (auto& item : m_registeredMapCrystals)
        {
            if (item.object && item.object->GetTransform())
                item.zSort = item.object->GetTransform()->GetWorldPosition().z;
        }
        std::sort(m_registeredMapCrystals.begin(), m_registeredMapCrystals.end(),
            [](const RegisteredMapCrystal& a, const RegisteredMapCrystal& b)
            {
                return a.zSort > b.zSort;
            });
    }

    void BossScript::UpdateDeathSequence(float deltaTime)
    {
        if (!m_cameraTransform)
        {
            m_deathSequenceActive = false;
            m_deathPhase = DeathPhase::Finished;
            return;
        }

        m_deathPhaseElapsed += deltaTime;

        switch (m_deathPhase)
        {
        case DeathPhase::CameraMoveToBoss:
        {
            const float t = EaseInOutSine(m_deathPhaseElapsed / std::max(0.01f, m_deathCamMoveDuration));
            const engine::Vector3 pos = m_deathCamStartPos + ((m_deathCamBossPos - m_deathCamStartPos) * t);
            m_cameraTransform->SetLocalPosition(ToLocalPosition(m_cameraTransform, pos));
            UpdateDeathCameraLookAtBoss();

            if (m_deathPhaseElapsed >= m_deathCamMoveDuration)
            {
                m_deathPhase = DeathPhase::CoreBurst;
                m_deathPhaseElapsed = 0.0f;
            }
            break;
        }
        case DeathPhase::CoreBurst:
            BurstBossCore();
            SpawnPartDropDustIfNeeded();
            m_deathPhase = DeathPhase::RotatingPartsDrop;
            m_deathPhaseElapsed = 0.0f;
            break;
        case DeathPhase::RotatingPartsDrop:
            UpdateDeathPartDrops(m_rotatingPartsForDeath, deltaTime);
            if (m_deathPhaseElapsed >= m_deathRotatingDropDuration)
            {
                m_deathPhase = DeathPhase::RemainingPartsDrop;
                m_deathPhaseElapsed = 0.0f;
            }
            break;
        case DeathPhase::RemainingPartsDrop:
            UpdateDeathPartDrops(m_remainingPartsForDeath, deltaTime);
            if (m_deathPhaseElapsed >= m_deathRemainingDropDuration)
            {
                StopPartDropDustNaturally();
                m_deathPhase = DeathPhase::CameraRetreatAndMapCrystalBurst;
                m_deathPhaseElapsed = 0.0f;
            }
            break;
        case DeathPhase::CameraRetreatAndMapCrystalBurst:
        {
            const float t = EaseInOutSine(m_deathPhaseElapsed / std::max(0.01f, m_deathCamRetreatDuration));
            const engine::Vector3 pos = m_deathCamBossPos + ((m_deathCamEndPos - m_deathCamBossPos) * t);
            m_cameraTransform->SetLocalPosition(ToLocalPosition(m_cameraTransform, pos));
            UpdateDeathCameraLookAtBoss();

            const int total = static_cast<int>(m_registeredMapCrystals.size());
            const int burstCount = static_cast<int>(std::floor(t * static_cast<float>(total)));
            for (int i = 0; i < burstCount; ++i)
                BurstMapCrystal(m_registeredMapCrystals[static_cast<size_t>(i)]);

            if (m_deathPhaseElapsed >= m_deathCamRetreatDuration)
            {
                for (auto& c : m_registeredMapCrystals)
                    BurstMapCrystal(c);
                m_deathPhase = DeathPhase::EndHold;
                m_deathPhaseElapsed = 0.0f;
            }
            break;
        }
        case DeathPhase::EndHold:
            if (m_deathPhaseElapsed >= m_deathEndHoldDuration)
            {
                m_deathPhase = DeathPhase::Finished;
                m_deathPhaseElapsed = 0.0f;
                m_deathSequenceActive = false;
                GameScene::Change(SceneID::Lobby);
                return;
            }
            break;
        case DeathPhase::Finished:
        case DeathPhase::None:
        default:
            break;
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
        ImGui::SeparatorText("=== 보스 클리어 연출 설정 ===");
        ImGui::Text("현재 페이즈: %d, 진행중: %s", static_cast<int>(m_deathPhase), m_deathSequenceActive ? "true" : "false");
        ImGui::DragFloat("카메라 접근 시간", &m_deathCamMoveDuration, 0.05f, 0.1f, 10.0f);
        ImGui::DragFloat("회전 파츠 낙하 시간", &m_deathRotatingDropDuration, 0.05f, 0.1f, 10.0f);
        ImGui::DragFloat("주변 파츠 낙하 시간", &m_deathRemainingDropDuration, 0.05f, 0.1f, 10.0f);
        ImGui::DragFloat("카메라 후진 시간", &m_deathCamRetreatDuration, 0.05f, 0.1f, 10.0f);
        ImGui::DragFloat("엔딩 대기 시간", &m_deathEndHoldDuration, 0.05f, 0.1f, 10.0f);
        ImGui::DragFloat("파츠 낙하 속도", &m_deathPartDropSpeed, 0.1f, 0.1f, 40.0f);
        ImGui::DragFloat("파츠 시차(초)", &m_deathPartDelayStep, 0.005f, 0.0f, 1.0f);
        ImGui::DragFloat("파츠 떨림 강도", &m_deathShakeAmount, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("파츠 떨림 속도", &m_deathShakeSpeed, 0.1f, 0.1f, 80.0f);
        ImGui::DragFloat("카메라-보스 거리", &m_deathCamBossDistance, 0.1f, 1.0f, 50.0f);
        ImGui::DragFloat("카메라-보스 높이", &m_deathCamBossHeight, 0.1f, 1.0f, 50.0f);
        ImGui::DragFloat("카메라 후진 거리", &m_deathCamRetreatDistance, 0.1f, 1.0f, 80.0f);
        ImGui::DragFloat3("카메라 바라보기 오프셋", &m_deathCamLookAtOffset.x, 0.05f, -20.0f, 20.0f);
        ImGui::DragInt("코어 파괴 위쪽 개수", &m_deathCoreBurstUpperCount, 1, 0, 50);
        ImGui::DragInt("코어 파괴 아래쪽 개수", &m_deathCoreBurstLowerCount, 1, 0, 50);
        ImGui::DragFloat("코어 파괴 원형 반경", &m_deathCoreBurstRadius, 0.05f, 0.1f, 12.0f);
        ImGui::DragFloat("코어 위쪽 Y 최소", &m_deathCoreBurstUpperYMin, 0.05f, -10.0f, 10.0f);
        ImGui::DragFloat("코어 위쪽 Y 최대", &m_deathCoreBurstUpperYMax, 0.05f, -10.0f, 10.0f);
        ImGui::DragFloat("코어 아래쪽 Y 최소", &m_deathCoreBurstLowerYMin, 0.05f, -10.0f, 10.0f);
        ImGui::DragFloat("코어 아래쪽 Y 최대", &m_deathCoreBurstLowerYMax, 0.05f, -10.0f, 10.0f);
        ImGui::DragFloat("코어 파괴 파티클 Y 오프셋", &m_deathCoreBurstOffsetY, 0.05f, -10.0f, 10.0f);
        ImGui::DragFloat("맵 수정 파괴 파티클 Y 오프셋", &m_deathMapCrystalBurstOffsetY, 0.05f, -10.0f, 10.0f);
        ImGui::DragFloat("파츠 먼지 이펙트 Y", &m_deathPartDustGroundY, 0.05f, -10.0f, 10.0f);
        ImGui::DragFloat("파츠 먼지 이펙트 Z 오프셋", &m_deathPartDustOffsetZ, 0.05f, -20.0f, 20.0f);
        ImGui::DragFloat("파괴 파티클 공통 스케일", &m_deathDestroyParticleScale, 0.05f, 0.1f, 10.0f);
        ImGui::InputText("코어 파괴 파티클", &m_deathCoreBurstParticlePrefab);
        ImGui::InputText("맵 수정 파괴 파티클", &m_deathMapCrystalBurstParticlePrefab);
        ImGui::InputText("파츠 낙하 먼지 파티클", &m_deathPartDropDustParticlePrefab);
        ImGui::Separator();

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

        ImGui::SeparatorText("=== Boss Intro Sequence ===");
        ImGui::Checkbox("Enable Intro Sequence", &m_enableIntroSequence);
        ImGui::Text("Battle Started: %s", m_isBattleStarted ? "true" : "false");
        ImGui::Text("Intro Running: %s", m_isIntroRunning ? "true" : "false");
        ImGui::DragFloat("Move To Boss Duration", &m_introMoveToBossDuration, 0.05f, 0.1f, 10.0f);
        ImGui::DragFloat("Hold Duration", &m_introHoldDuration, 0.05f, 0.0f, 5.0f);
        ImGui::DragFloat("Assemble Duration", &m_introAssembleDuration, 0.05f, 0.1f, 10.0f);
        ImGui::DragFloat("Return Duration", &m_introReturnDuration, 0.05f, 0.1f, 10.0f);
        ImGui::Separator();
        ImGui::DragFloat("Camera Start Offset X", &m_introCameraPlayerOffsetX, 0.1f, -50.0f, 50.0f);
        ImGui::DragFloat("Camera Start Offset Y", &m_introCameraPlayerOffsetY, 0.1f, 0.0f, 80.0f);
        ImGui::DragFloat("Camera Start Offset Z", &m_introCameraPlayerOffsetZ, 0.1f, -80.0f, 80.0f);
        ImGui::DragFloat("Camera Boss Distance", &m_introCameraBossDistance, 0.1f, 0.0f, 50.0f);
        ImGui::DragFloat("Camera Boss Height", &m_introCameraBossHeight, 0.1f, 0.0f, 80.0f);
        if (ImGui::Button("Start Intro Sequence"))
        {
            StartIntroSequence();
        }
        ImGui::SameLine();
        if (ImGui::Button("Skip Intro Sequence"))
        {
            SkipIntroSequence();
        }

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
        j["EnableIntroSequence"] = m_enableIntroSequence;
        j["IntroMoveToBossDuration"] = m_introMoveToBossDuration;
        j["IntroHoldDuration"] = m_introHoldDuration;
        j["IntroAssembleDuration"] = m_introAssembleDuration;
        j["IntroReturnDuration"] = m_introReturnDuration;
        j["IntroCameraPlayerOffsetX"] = m_introCameraPlayerOffsetX;
        j["IntroCameraPlayerOffsetY"] = m_introCameraPlayerOffsetY;
        j["IntroCameraPlayerOffsetZ"] = m_introCameraPlayerOffsetZ;
        j["IntroCameraBossDistance"] = m_introCameraBossDistance;
        j["IntroCameraBossHeight"] = m_introCameraBossHeight;

        j["DeathCamMoveDuration"] = m_deathCamMoveDuration;
        j["DeathCamRetreatDuration"] = m_deathCamRetreatDuration;
        j["DeathRotatingDropDuration"] = m_deathRotatingDropDuration;
        j["DeathRemainingDropDuration"] = m_deathRemainingDropDuration;
        j["DeathEndHoldDuration"] = m_deathEndHoldDuration;
        j["DeathPartDropSpeed"] = m_deathPartDropSpeed;
        j["DeathPartDelayStep"] = m_deathPartDelayStep;
        j["DeathShakeAmount"] = m_deathShakeAmount;
        j["DeathShakeSpeed"] = m_deathShakeSpeed;
        j["DeathCamBossDistance"] = m_deathCamBossDistance;
        j["DeathCamBossHeight"] = m_deathCamBossHeight;
        j["DeathCamRetreatDistance"] = m_deathCamRetreatDistance;
        j["DeathCamLookAtOffset"] = { m_deathCamLookAtOffset.x, m_deathCamLookAtOffset.y, m_deathCamLookAtOffset.z };
        j["DeathCoreBurstUpperCount"] = m_deathCoreBurstUpperCount;
        j["DeathCoreBurstLowerCount"] = m_deathCoreBurstLowerCount;
        j["DeathCoreBurstRadius"] = m_deathCoreBurstRadius;
        j["DeathCoreBurstUpperYMin"] = m_deathCoreBurstUpperYMin;
        j["DeathCoreBurstUpperYMax"] = m_deathCoreBurstUpperYMax;
        j["DeathCoreBurstLowerYMin"] = m_deathCoreBurstLowerYMin;
        j["DeathCoreBurstLowerYMax"] = m_deathCoreBurstLowerYMax;
        j["DeathCoreBurstOffsetY"] = m_deathCoreBurstOffsetY;
        j["DeathMapCrystalBurstOffsetY"] = m_deathMapCrystalBurstOffsetY;
        j["DeathPartDustGroundY"] = m_deathPartDustGroundY;
        j["DeathPartDustOffsetZ"] = m_deathPartDustOffsetZ;
        j["DeathDestroyParticleScale"] = m_deathDestroyParticleScale;
        j["DeathCoreBurstParticlePrefab"] = m_deathCoreBurstParticlePrefab;
        j["DeathMapCrystalBurstParticlePrefab"] = m_deathMapCrystalBurstParticlePrefab;
        j["DeathPartDropDustParticlePrefab"] = m_deathPartDropDustParticlePrefab;

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
        if (j.contains("EnableIntroSequence"))
            m_enableIntroSequence = j["EnableIntroSequence"].get<bool>();
        if (j.contains("IntroMoveToBossDuration"))
            m_introMoveToBossDuration = j["IntroMoveToBossDuration"].get<float>();
        if (j.contains("IntroHoldDuration"))
            m_introHoldDuration = j["IntroHoldDuration"].get<float>();
        if (j.contains("IntroAssembleDuration"))
            m_introAssembleDuration = j["IntroAssembleDuration"].get<float>();
        if (j.contains("IntroReturnDuration"))
            m_introReturnDuration = j["IntroReturnDuration"].get<float>();
        if (j.contains("IntroCameraPlayerOffsetX"))
            m_introCameraPlayerOffsetX = j["IntroCameraPlayerOffsetX"].get<float>();
        if (j.contains("IntroCameraPlayerOffsetY"))
            m_introCameraPlayerOffsetY = j["IntroCameraPlayerOffsetY"].get<float>();
        if (j.contains("IntroCameraPlayerOffsetZ"))
            m_introCameraPlayerOffsetZ = j["IntroCameraPlayerOffsetZ"].get<float>();
        if (j.contains("IntroCameraBossDistance"))
            m_introCameraBossDistance = j["IntroCameraBossDistance"].get<float>();
        if (j.contains("IntroCameraBossHeight"))
            m_introCameraBossHeight = j["IntroCameraBossHeight"].get<float>();
        if (j.contains("DeathCamMoveDuration"))
            m_deathCamMoveDuration = std::clamp(j["DeathCamMoveDuration"].get<float>(), 0.1f, 10.0f);
        if (j.contains("DeathCamRetreatDuration"))
            m_deathCamRetreatDuration = std::clamp(j["DeathCamRetreatDuration"].get<float>(), 0.1f, 10.0f);
        if (j.contains("DeathRotatingDropDuration"))
            m_deathRotatingDropDuration = std::clamp(j["DeathRotatingDropDuration"].get<float>(), 0.1f, 10.0f);
        if (j.contains("DeathRemainingDropDuration"))
            m_deathRemainingDropDuration = std::clamp(j["DeathRemainingDropDuration"].get<float>(), 0.1f, 10.0f);
        if (j.contains("DeathEndHoldDuration"))
            m_deathEndHoldDuration = std::clamp(j["DeathEndHoldDuration"].get<float>(), 0.1f, 10.0f);
        if (j.contains("DeathPartDropSpeed"))
            m_deathPartDropSpeed = std::clamp(j["DeathPartDropSpeed"].get<float>(), 0.1f, 40.0f);
        if (j.contains("DeathPartDelayStep"))
            m_deathPartDelayStep = std::clamp(j["DeathPartDelayStep"].get<float>(), 0.0f, 1.0f);
        if (j.contains("DeathShakeAmount"))
            m_deathShakeAmount = std::clamp(j["DeathShakeAmount"].get<float>(), 0.0f, 1.0f);
        if (j.contains("DeathShakeSpeed"))
            m_deathShakeSpeed = std::clamp(j["DeathShakeSpeed"].get<float>(), 0.1f, 80.0f);
        if (j.contains("DeathCamBossDistance"))
            m_deathCamBossDistance = std::clamp(j["DeathCamBossDistance"].get<float>(), 1.0f, 50.0f);
        if (j.contains("DeathCamBossHeight"))
            m_deathCamBossHeight = std::clamp(j["DeathCamBossHeight"].get<float>(), 1.0f, 50.0f);
        if (j.contains("DeathCamRetreatDistance"))
            m_deathCamRetreatDistance = std::clamp(j["DeathCamRetreatDistance"].get<float>(), 1.0f, 80.0f);
        if (j.contains("DeathCamLookAtOffset") && j["DeathCamLookAtOffset"].is_array() && j["DeathCamLookAtOffset"].size() >= 3)
        {
            m_deathCamLookAtOffset.x = j["DeathCamLookAtOffset"][0].get<float>();
            m_deathCamLookAtOffset.y = j["DeathCamLookAtOffset"][1].get<float>();
            m_deathCamLookAtOffset.z = j["DeathCamLookAtOffset"][2].get<float>();
        }
        if (j.contains("DeathCoreBurstUpperCount"))
            m_deathCoreBurstUpperCount = std::clamp(j["DeathCoreBurstUpperCount"].get<int>(), 0, 50);
        if (j.contains("DeathCoreBurstLowerCount"))
            m_deathCoreBurstLowerCount = std::clamp(j["DeathCoreBurstLowerCount"].get<int>(), 0, 50);
        if (j.contains("DeathCoreBurstRadius"))
            m_deathCoreBurstRadius = std::clamp(j["DeathCoreBurstRadius"].get<float>(), 0.1f, 12.0f);
        if (j.contains("DeathCoreBurstUpperYMin"))
            m_deathCoreBurstUpperYMin = std::clamp(j["DeathCoreBurstUpperYMin"].get<float>(), -10.0f, 10.0f);
        if (j.contains("DeathCoreBurstUpperYMax"))
            m_deathCoreBurstUpperYMax = std::clamp(j["DeathCoreBurstUpperYMax"].get<float>(), -10.0f, 10.0f);
        if (j.contains("DeathCoreBurstLowerYMin"))
            m_deathCoreBurstLowerYMin = std::clamp(j["DeathCoreBurstLowerYMin"].get<float>(), -10.0f, 10.0f);
        if (j.contains("DeathCoreBurstLowerYMax"))
            m_deathCoreBurstLowerYMax = std::clamp(j["DeathCoreBurstLowerYMax"].get<float>(), -10.0f, 10.0f);
        // 하위 호환(이전 저장값)
        if (j.contains("DeathCoreExtraBurstCount"))
        {
            const int legacyCount = std::clamp(j["DeathCoreExtraBurstCount"].get<int>(), 0, 50);
            if (!j.contains("DeathCoreBurstUpperCount") && !j.contains("DeathCoreBurstLowerCount"))
            {
                m_deathCoreBurstUpperCount = legacyCount;
                m_deathCoreBurstLowerCount = legacyCount;
            }
        }
        if (j.contains("DeathCoreExtraBurstRadius") && !j.contains("DeathCoreBurstRadius"))
            m_deathCoreBurstRadius = std::clamp(j["DeathCoreExtraBurstRadius"].get<float>(), 0.1f, 12.0f);
        if (j.contains("DeathCoreBurstOffsetY"))
            m_deathCoreBurstOffsetY = std::clamp(j["DeathCoreBurstOffsetY"].get<float>(), -10.0f, 10.0f);
        if (j.contains("DeathMapCrystalBurstOffsetY"))
            m_deathMapCrystalBurstOffsetY = std::clamp(j["DeathMapCrystalBurstOffsetY"].get<float>(), -10.0f, 10.0f);
        if (j.contains("DeathPartDustGroundY"))
            m_deathPartDustGroundY = std::clamp(j["DeathPartDustGroundY"].get<float>(), -10.0f, 10.0f);
        if (j.contains("DeathPartDustOffsetZ"))
            m_deathPartDustOffsetZ = std::clamp(j["DeathPartDustOffsetZ"].get<float>(), -20.0f, 20.0f);
        if (j.contains("DeathDestroyParticleScale"))
            m_deathDestroyParticleScale = std::clamp(j["DeathDestroyParticleScale"].get<float>(), 0.1f, 10.0f);
        if (j.contains("DeathCoreBurstParticlePrefab"))
            m_deathCoreBurstParticlePrefab = j["DeathCoreBurstParticlePrefab"].get<std::string>();
        if (j.contains("DeathMapCrystalBurstParticlePrefab"))
            m_deathMapCrystalBurstParticlePrefab = j["DeathMapCrystalBurstParticlePrefab"].get<std::string>();
        if (j.contains("DeathPartDropDustParticlePrefab"))
            m_deathPartDropDustParticlePrefab = j["DeathPartDropDustParticlePrefab"].get<std::string>();

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
