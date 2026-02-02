#include "GamePCH.h"
#include "MonsterRoundGray.h"

#include "Script/CharacterScript/Common/BulletFactory.h"

#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Object/Component/Renderer/SkeletalMeshRenderer.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Physics/PhysicsSystem.h>
#include <Framework/Physics/CollisionTypes.h>
#include <Framework/Physics/PhysicsLayer.h>
#include <Engine/Core/System/MyTime.h>
#include <Engine/Core/System/Engine.h>

#include <random>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::Awake()
    {
        MonsterRoundType::Awake();
        
        // Gray 등급 고정
        m_monsterTier = MonsterTier::Gray;
    }

    void MonsterRoundGray::Start()
    {
        MonsterRoundType::Start();

        // Gray 등급 색상 설정 (회색)
        if (auto* meshRenderer = GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>())
        {
            meshRenderer->SetBaseColor(engine::Vector4(0.5f, 0.5f, 0.5f, 1.0f));
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::OnCollisionEnter(const engine::CollisionInfo& info)
    {
        // 총알 레이어 무시 (Projectile, EnemyProjectile)
        if (info.gameObject)
        {
            auto* collider = info.collider.Get();
            if (collider)
            {
                uint32_t layer = collider->GetLayer();
                if (layer == engine::PhysicsLayer::Index::Projectile ||
                    layer == engine::PhysicsLayer::Index::EnemyProjectile)
                {
                    return;  // 총알 충돌 무시
                }
            }
        }

        std::string currentState = GetCurrentState();
        
        if (currentState == "IdleMove")
        {
            // IdleMove 상태에서 충돌 → 90도 방향 전환
            m_collisionOccurred = true;
        }
        else if (currentState == "EngageMove")
        {
            // EngageMove 상태에서 충돌 → IdleMove로 복귀 + 플레이어 무시
            StartPlayerIgnore();
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("ReturnToIdleMove", true);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 총알 초기화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::InitializeBullet()
    {
        // Gray: 기본 Linear 총알
        m_bulletParams.type = BulletType::Linear;
        m_bulletParams.speed = m_bulletSpeed;
        m_bulletParams.lifetime = m_bulletLifetime;
        m_bulletParams.damage = static_cast<int>(m_attackDamage);
    }

    // ═══════════════════════════════════════════════════════════════
    // 공격 (Gray 전용)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::Attack(float deltaTime)
    {
        // 공격 애니메이션 타이머 업데이트
        m_attackAnimationTimer += deltaTime;

        // 발사 가능 상태
        if (m_fireTimer <= 0.0f)
        {
            if (m_bulletFactory && m_targetPlayer && m_targetPlayer->GetGameObject())
            {
                // Gray: 단순 직선 발사
                engine::Vector3 direction = CalculateDirectionToPlayer();
                engine::Vector3 firePosition = GetTransform()->GetWorldPosition();
                
                m_bulletFactory->LinearFireMonster(firePosition, direction, m_bulletParams);

                // 공격 애니메이션 재생
                if (m_skeletalAnimator && !m_animName_EngageAttack.empty())
                {
                    m_skeletalAnimator->Play(m_animName_EngageAttack, false, 0, 1.0f);
                }

                // 발사 쿨타임 리셋
                m_fireTimer = m_fireRate;
            }
        }

        // 공격 애니메이션 완료 체크
        if (m_attackAnimationTimer >= m_attackAnimationDuration)
        {
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("AttackComplete", true);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // IdleMove 상태 비물리 행동
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::ExecuteIdleMoveBehaviorNonPhysics(float deltaTime)
    {
        // 충돌 발생 처리 (Update에서 처리하여 FixedUpdate 전에 방향 변경)
        if (m_collisionOccurred)
        {
            ChangeDirectionOnCollision();
            ResetMoveDuration();
            OnDirectionChanged();  // 플레이어 무시 카운트 감소
            m_collisionOccurred = false;
        }
        
        // 이동 시간 업데이트
        m_moveDurationTimer += deltaTime;
        
        // 이동 지속 시간 완료 → 랜덤 방향 전환
        if (m_moveDurationTimer >= m_moveDuration)
        {
            ChangeDirectionRandom();
            ResetMoveDuration();
            OnDirectionChanged();  // 플레이어 무시 카운트 감소
        }
        
        // 4방향 레이캐스트로 플레이어 감지
        // 플레이어 무시 중이 아닌 경우에만 감지
        if (!m_isIgnoringPlayer)
        {
            if (DetectPlayerWithRaycast())
            {
                if (m_logicFSM)
                {
                    m_logicFSM->SetParameter("PlayerDetected", true);
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // IdleMove 상태 물리 행동
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::ExecuteIdleMoveBehaviorPhysics()
    {
        if (!m_rigidbody) return;

        // 현재 방향으로 이동
        engine::Vector3 direction = GetDirectionVector();
        engine::Vector3 velocity = direction * m_moveSpeed;
        
        // Y축은 유지 (중력 영향)
        engine::Vector3 currentVel = m_rigidbody->GetVelocity();
        velocity.y = currentVel.y;
        
        m_rigidbody->SetVelocity(velocity);
    }

    // ═══════════════════════════════════════════════════════════════
    // IdleMove 초기화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::InitializeIdleMove()
    {
        ChangeDirectionRandom();
        ResetMoveDuration();
        m_moveDurationTimer = 0.0f;
        m_collisionOccurred = false;
    }

    // ═══════════════════════════════════════════════════════════════
    // 랜덤 방향 전환 (4방향 중 하나)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::ChangeDirectionRandom()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0, 3);
        
        m_currentDirection = static_cast<MoveDirection>(dist(gen));
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 시 90도 방향 전환 (좌/우 랜덤)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::ChangeDirectionOnCollision()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0, 1);
        
        bool turnLeft = (dist(gen) == 0);
        
        // 현재 방향 기준 좌/우 90도 회전
        // 월드 좌표 기준:
        // +X → 좌: +Z, 우: -Z
        // -X → 좌: -Z, 우: +Z
        // +Z → 좌: -X, 우: +X
        // -Z → 좌: +X, 우: -X
        
        switch (m_currentDirection)
        {
        case MoveDirection::PlusX:
            m_currentDirection = turnLeft ? MoveDirection::PlusZ : MoveDirection::MinusZ;
            break;
        case MoveDirection::MinusX:
            m_currentDirection = turnLeft ? MoveDirection::MinusZ : MoveDirection::PlusZ;
            break;
        case MoveDirection::PlusZ:
            m_currentDirection = turnLeft ? MoveDirection::MinusX : MoveDirection::PlusX;
            break;
        case MoveDirection::MinusZ:
            m_currentDirection = turnLeft ? MoveDirection::PlusX : MoveDirection::MinusX;
            break;
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 이동 지속 시간 재설정
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::ResetMoveDuration()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(m_moveDurationMin, m_moveDurationMax);
        
        m_moveDuration = dist(gen);
        m_moveDurationTimer = 0.0f;
    }

    // ═══════════════════════════════════════════════════════════════
    // 현재 방향 → 방향 벡터 변환
    // ═══════════════════════════════════════════════════════════════
    engine::Vector3 MonsterRoundGray::GetDirectionVector() const
    {
        switch (m_currentDirection)
        {
        case MoveDirection::PlusX:
            return engine::Vector3(1.0f, 0.0f, 0.0f);
        case MoveDirection::MinusX:
            return engine::Vector3(-1.0f, 0.0f, 0.0f);
        case MoveDirection::PlusZ:
            return engine::Vector3(0.0f, 0.0f, 1.0f);
        case MoveDirection::MinusZ:
            return engine::Vector3(0.0f, 0.0f, -1.0f);
        default:
            return engine::Vector3(1.0f, 0.0f, 0.0f);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 4방향 레이캐스트로 플레이어 감지
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundGray::DetectPlayerWithRaycast()
    {
        auto* physicsSystem = engine::Engine::Instance()->GetPhysicsSystem();
        if (!physicsSystem) return false;

        engine::Vector3 origin = GetTransform()->GetWorldPosition();
        origin.y += 1.0f;  // 약간 위에서 시작 (바닥 충돌 방지)

        // 플레이어 레이어만 감지
        uint32_t layerMask = engine::PhysicsLayer::Mask::PlayerMask;

        // 4방향 레이캐스트
        const engine::Vector3 directions[4] = {
            engine::Vector3(1.0f, 0.0f, 0.0f),   // +X
            engine::Vector3(-1.0f, 0.0f, 0.0f),  // -X
            engine::Vector3(0.0f, 0.0f, 1.0f),   // +Z
            engine::Vector3(0.0f, 0.0f, -1.0f)   // -Z
        };

        engine::RaycastHit hit;
        for (const auto& dir : directions)
        {
            if (physicsSystem->Raycast(origin, dir, m_raycastDetectionRange, hit, layerMask))
            {
                if (hit.hasHit && hit.gameObject.Get())
                {
                    return true;  // 플레이어 감지!
                }
            }
        }

        return false;
    }

    // ═══════════════════════════════════════════════════════════════
    // 플레이어 무시 시스템 시작
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::StartPlayerIgnore()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(m_playerIgnoreCountMin, m_playerIgnoreCountMax);
        
        m_playerIgnoreCount = dist(gen);
        m_isIgnoringPlayer = true;
    }

    // ═══════════════════════════════════════════════════════════════
    // 방향 변경 시 호출 (플레이어 무시 횟수 감소)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::OnDirectionChanged()
    {
        if (m_isIgnoringPlayer && m_playerIgnoreCount > 0)
        {
            m_playerIgnoreCount--;
            if (m_playerIgnoreCount <= 0)
            {
                m_isIgnoringPlayer = false;
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태 진입 콜백 오버라이드
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::OnStateEntered(const std::string& state)
    {
        // 부모 클래스 호출
        MonsterRoundType::OnStateEntered(state);
        
        if (state == "IdleMove")
        {
            // IdleMove 진입 시 초기화
            InitializeIdleMove();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::OnGui()
    {
        ImGui::Text("=== MonsterRoundGray ===");
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Tier: Gray");
        
        // 부모 클래스 OnGui 호출
        MonsterRoundType::OnGui();
        
        // Gray 전용 설정
        ImGui::Separator();
        ImGui::Text("=== Gray IdleMove Settings ===");
        ImGui::DragFloat("Move Duration Min", &m_moveDurationMin, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Move Duration Max", &m_moveDurationMax, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Raycast Range", &m_raycastDetectionRange, 0.5f, 1.0f, 50.0f);
        
        ImGui::Separator();
        ImGui::Text("=== Player Ignore Settings ===");
        ImGui::DragInt("Ignore Count Min", &m_playerIgnoreCountMin, 1, 1, 10);
        ImGui::DragInt("Ignore Count Max", &m_playerIgnoreCountMax, 1, 1, 10);
        
        // 런타임 정보
        ImGui::Separator();
        ImGui::Text("=== Gray Runtime ===");
        const char* dirNames[] = { "+X", "-X", "+Z", "-Z" };
        ImGui::Text("Current Direction: %s", dirNames[static_cast<int>(m_currentDirection)]);
        ImGui::Text("Move Duration: %.2f / %.2f", m_moveDurationTimer, m_moveDuration);
        ImGui::Text("Ignoring Player: %s", m_isIgnoringPlayer ? "Yes" : "No");
        if (m_isIgnoringPlayer)
        {
            ImGui::Text("Ignore Count Remaining: %d", m_playerIgnoreCount);
        }
    }

    void MonsterRoundGray::Save(engine::json& j) const
    {
        MonsterRoundType::Save(j);
        
        // Gray 전용 데이터 저장
        j["MoveDurationMin"] = m_moveDurationMin;
        j["MoveDurationMax"] = m_moveDurationMax;
        j["RaycastDetectionRange"] = m_raycastDetectionRange;
        j["PlayerIgnoreCountMin"] = m_playerIgnoreCountMin;
        j["PlayerIgnoreCountMax"] = m_playerIgnoreCountMax;
    }

    void MonsterRoundGray::Load(const engine::json& j)
    {
        MonsterRoundType::Load(j);
        
        // Gray 전용 데이터 로드
        m_moveDurationMin = j.value("MoveDurationMin", 1.0f);
        m_moveDurationMax = j.value("MoveDurationMax", 4.0f);
        m_raycastDetectionRange = j.value("RaycastDetectionRange", 20.0f);
        m_playerIgnoreCountMin = j.value("PlayerIgnoreCountMin", 1);
        m_playerIgnoreCountMax = j.value("PlayerIgnoreCountMax", 4);
    }
}
