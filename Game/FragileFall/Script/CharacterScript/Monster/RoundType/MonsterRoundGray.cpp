#include "GamePCH.h"
#include "MonsterRoundGray.h"

#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/CornerTrigger.h"

#include <Framework/Object/Component/Renderer/SkeletalMeshRenderer.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Physics/PhysicsSystem.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/System/SystemManager.h>
#include <Engine/Core/System/MyTime.h>

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
        // SkeletalMeshRenderer만 SetBaseColor 지원
        if (m_meshType == RoundMeshType::Skeletal)
        {
            if (auto* meshRenderer = GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>())
            {
                meshRenderer->SetBaseColor(engine::Vector4(0.5f, 0.5f, 0.5f, 1.0f));
            }
        }
        // StaticMeshRenderer는 현재 SetBaseColor 미지원 (필요시 엔진 수정)
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::OnCollisionEnter(const engine::CollisionInfo& info)
    {
        if (!info.gameObject) return;
        
        // 총알 레이어 무시 (Projectile, EnemyProjectile)
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
        
        // ─────────────────────────────────────────────
        // 플레이어 충돌 시 데미지 처리
        // ─────────────────────────────────────────────
        auto* player = info.gameObject->GetComponent<PlayerControllerScript>();
        if (player)
        {
            float elapsedSinceLastDamage = engine::Time::GetElapsedSeconds(m_lastDamageTime);
            if (elapsedSinceLastDamage >= m_damageCooldown)
            {
                player->TakeDamage(static_cast<int>(m_attackDamage));
                m_lastDamageTime = engine::Time::GetTimestamp();
            }
        }
        
        // ─────────────────────────────────────────────
        // 충돌 방향 계산 (노말 벡터 사용)
        // ─────────────────────────────────────────────
        engine::Vector3 collisionDir = engine::Vector3::Zero;
        for (const auto& contact : info.contacts)
        {
            // 노말 반전: A→B 방향이므로, 몬스터 입장에서는 반대 방향
            engine::Vector3 normal = -contact.normal;
            normal.y = 0.0f;  // 수평 성분만
            if (normal.LengthSquared() > 0.0001f)
            {
                collisionDir = normal;
                collisionDir.Normalize();
                break;
            }
        }
        
        // 노말이 없으면 위치 기반으로 계산
        if (collisionDir.LengthSquared() < 0.0001f)
        {
            engine::Vector3 myPos = GetTransform()->GetWorldPosition();
            engine::Vector3 otherPos = info.gameObject->GetTransform()->GetWorldPosition();
            collisionDir = otherPos - myPos;
            collisionDir.y = 0.0f;
            if (collisionDir.LengthSquared() > 0.0001f)
            {
                collisionDir.Normalize();
            }
        }
        
        // ─────────────────────────────────────────────
        // 충돌 방향이 진행방향 앞인지 판단
        // - 내적 > 0: 앞쪽 충돌 (기존 로직)
        // - 내적 <= 0: 뒤/옆 충돌 (충돌 방향 기준 회전)
        // ─────────────────────────────────────────────
        engine::Vector3 moveDir = GetDirectionVector();
        float dotProduct = moveDir.Dot(collisionDir);
        
        // 충돌 기준 방향 결정 (회전의 기준이 되는 방향)
        MoveDirection collisionBaseDir;
        if (dotProduct > 0.3f)
        {
            // 앞쪽 충돌 → 현재 진행방향 기준으로 90도 회전
            collisionBaseDir = m_currentDirection;
        }
        else
        {
            // 뒤/옆 충돌 → 충돌 방향 기준으로 90도 회전
            // 대각선인 경우 성분이 큰 쪽 선택
            if (std::abs(collisionDir.x) >= std::abs(collisionDir.z))
            {
                collisionBaseDir = (collisionDir.x > 0) ? MoveDirection::PlusX : MoveDirection::MinusX;
            }
            else
            {
                collisionBaseDir = (collisionDir.z > 0) ? MoveDirection::PlusZ : MoveDirection::MinusZ;
            }
        }

        std::string currentState = GetCurrentState();
        
        if (currentState == "IdleMove")
        {
            // IdleMove 상태에서 충돌 → 충돌 방향 기준 90도 방향 전환
            // 충돌 기준 방향을 현재 방향으로 설정 후 ChangeDirectionOnCollision 호출
            m_currentDirection = collisionBaseDir;
            m_collisionOccurred = true;
        }
        else if (currentState == "EngageMove")
        {
            // EngageMove 상태에서 충돌 → IdleMove로 복귀 + 플레이어 무시
            StartPlayerIgnore();
            
            // 충돌 기준 방향으로 설정
            m_currentDirection = collisionBaseDir;
            
            m_fromEngageCollision = true;  // IdleMove 진입 시 90도 회전 적용
            
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("ReturnToIdleMove", true);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 트리거 콜백 - 코너 트리거 감지
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        if (!info.gameObject) return;
        
        // 이미 코너 방향이 결정되어 있으면 무시 (중복 진입 방지)
        if (m_hasCornerDirection) return;
        
        // 충돌한 오브젝트에서 CornerTrigger 스크립트가 있는지 확인
        auto* cornerTrigger = info.gameObject->GetComponent<CornerTrigger>();
        if (!cornerTrigger) return;
        
        // 현재 이동 방향을 CornerTrigger의 BlockedDirection으로 변환
        CornerTrigger::BlockedDirection currentDir = CornerTrigger::BlockedDirection::None;
        switch (m_currentDirection)
        {
        case MoveDirection::PlusX:  currentDir = CornerTrigger::BlockedDirection::PlusX;  break;
        case MoveDirection::MinusX: currentDir = CornerTrigger::BlockedDirection::MinusX; break;
        case MoveDirection::PlusZ:  currentDir = CornerTrigger::BlockedDirection::PlusZ;  break;
        case MoveDirection::MinusZ: currentDir = CornerTrigger::BlockedDirection::MinusZ; break;
        }
        
        // 막힌 방향 2개 가져오기
        auto blocked1 = cornerTrigger->GetBlockedDir1();
        auto blocked2 = cornerTrigger->GetBlockedDir2();
        
        // 이동 가능한 방향 계산 (4방향 - 막힌 방향 2개 = 가능한 방향 2개)
        // 그 중 현재 방향의 90도 회전인 방향 선택
        // 
        // 예: TopRight 코너 (막힌: +X, +Z)
        //     현재 +X로 이동 중 → 가능한 방향: -X, -Z → 90도 회전은 -Z
        //     현재 +Z로 이동 중 → 가능한 방향: -X, -Z → 90도 회전은 -X
        
        // 현재 방향의 90도 좌/우 방향 계산
        MoveDirection left90, right90;
        switch (m_currentDirection)
        {
        case MoveDirection::PlusX:
            left90 = MoveDirection::PlusZ;
            right90 = MoveDirection::MinusZ;
            break;
        case MoveDirection::MinusX:
            left90 = MoveDirection::MinusZ;
            right90 = MoveDirection::PlusZ;
            break;
        case MoveDirection::PlusZ:
            left90 = MoveDirection::MinusX;
            right90 = MoveDirection::PlusX;
            break;
        case MoveDirection::MinusZ:
            left90 = MoveDirection::PlusX;
            right90 = MoveDirection::MinusX;
            break;
        }
        
        // 좌/우 중 막히지 않은 방향 선택
        auto ToBlockedDir = [](MoveDirection dir) -> CornerTrigger::BlockedDirection {
            switch (dir)
            {
            case MoveDirection::PlusX:  return CornerTrigger::BlockedDirection::PlusX;
            case MoveDirection::MinusX: return CornerTrigger::BlockedDirection::MinusX;
            case MoveDirection::PlusZ:  return CornerTrigger::BlockedDirection::PlusZ;
            case MoveDirection::MinusZ: return CornerTrigger::BlockedDirection::MinusZ;
            default: return CornerTrigger::BlockedDirection::None;
            }
        };
        
        bool left90Blocked = cornerTrigger->IsDirectionBlocked(ToBlockedDir(left90));
        bool right90Blocked = cornerTrigger->IsDirectionBlocked(ToBlockedDir(right90));
        
        if (!left90Blocked && right90Blocked)
        {
            // 좌측만 가능
            m_cornerDirection = left90;
            m_hasCornerDirection = true;
        }
        else if (left90Blocked && !right90Blocked)
        {
            // 우측만 가능
            m_cornerDirection = right90;
            m_hasCornerDirection = true;
        }
        else if (!left90Blocked && !right90Blocked)
        {
            // 둘 다 가능 (코너가 아닌 경우?) → 코너 방향 사용 안함
            m_hasCornerDirection = false;
        }
        else
        {
            // 둘 다 막힘 (잘못된 설정) → 무시
            m_hasCornerDirection = false;
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // Gray 전용 FSM 초기화
    // - EngageStop, EngageAttack 상태 없음 (Gray는 공격하지 않음)
    // - 레이캐스트 기반 플레이어 감지만 사용
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::InitializeFSM()
    {
        if (!m_logicFSM) return;

        // ─────────────────────────────────────────────
        // 상태 정의 (Gray 전용 - 공격 상태 없음)
        // ─────────────────────────────────────────────
        AddFSMState("Idle", true);           // 기본 상태 (대기)
        AddFSMState("IdleMove", false);      // 맵 배회
        AddFSMState("EngageMove", false);    // 추적 이동 (충돌까지 직진)
        AddFSMState("Repositioning", false); // 위치 보정
        AddFSMState("Fragile", false);
        AddFSMState("Dead", false);
        // EngageStop, EngageAttack 없음!

        // ─────────────────────────────────────────────
        // StateMap 업데이트 (Transition 추가 전에 필수!)
        // ─────────────────────────────────────────────
        m_logicFSM->Initialize();

        // ─────────────────────────────────────────────
        // 파라미터 정의 (Gray에 필요한 것만)
        // ─────────────────────────────────────────────
        m_logicFSM->SetParameter("IdleTimerComplete", false);     // Idle 대기 시간 완료
        m_logicFSM->SetParameter("PlayerDetected", false);        // 4방향 레이캐스트 감지
        m_logicFSM->SetParameter("NeedRepositioning", false);     // 위치 보정 필요
        m_logicFSM->SetParameter("RepositioningComplete", false); // 위치 보정 완료
        m_logicFSM->SetParameter("ReturnToIdleMove", false);      // EngageMove 충돌 후 IdleMove 복귀
        m_logicFSM->SetParameter("Fragile", m_isFragile);
        m_logicFSM->SetParameter("Die", m_isDead);
        // PlayerInRange, CanFire, AttackComplete 없음!

        // ─────────────────────────────────────────────
        // 전이 정의 (Gray 전용)
        // ─────────────────────────────────────────────
        
        // Idle → IdleMove (대기 시간 완료)
        AddFSMTransition("Idle", "IdleMove", "IdleTimerComplete", BoolTrue());
        
        // IdleMove → EngageMove (플레이어 레이캐스트 감지)
        AddFSMTransition("IdleMove", "EngageMove", "PlayerDetected", BoolTrue());
        
        // IdleMove → Repositioning (위치 보정 필요)
        AddFSMTransition("IdleMove", "Repositioning", "NeedRepositioning", BoolTrue());
        
        // EngageMove → IdleMove (충돌 후 복귀)
        AddFSMTransition("EngageMove", "IdleMove", "ReturnToIdleMove", BoolTrue());
        
        // EngageMove → Repositioning (위치 보정 필요)
        AddFSMTransition("EngageMove", "Repositioning", "NeedRepositioning", BoolTrue());
        
        // Repositioning → IdleMove (위치 보정 완료)
        AddFSMTransition("Repositioning", "IdleMove", "RepositioningComplete", BoolTrue());

        // Any → Fragile (HP 0, Fragile 트리거)
        AddFSMTransition("Idle", "Fragile", "Fragile", Trigger());
        AddFSMTransition("IdleMove", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageMove", "Fragile", "Fragile", Trigger());
        AddFSMTransition("Repositioning", "Fragile", "Fragile", Trigger());

        // Fragile → Dead (Execution, Die 트리거)
        AddFSMTransition("Fragile", "Dead", "Die", Trigger());
        
        // Fragile → Idle (부활, Revive 트리거)
        AddFSMTransition("Fragile", "Idle", "Revive", Trigger());

        // ─────────────────────────────────────────────
        // 초기 상태 설정
        // ─────────────────────────────────────────────
        m_logicFSM->InitializeCurrentState();
    }

    // ═══════════════════════════════════════════════════════════════
    // 입력 처리 (Gray 전용 - 최소화)
    // - 거리 기반 감지 사용 안함
    // - 공격 관련 검사 없음
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::ProcessInput()
    {
        // Gray는 ProcessInput에서 할 일이 없음
        // - 플레이어 감지: DetectPlayerWithRaycast()에서 처리
        // - 공격: 없음
        // 
        // 참고: m_targetPlayer는 EngageMove 방향 계산에 필요할 수 있으므로
        // 필요시 FindPlayer()를 다른 곳에서 호출
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

        // 현재 방향으로 이동 (리지드바디 타입별 통합 인터페이스 사용)
        engine::Vector3 direction = GetDirectionVector();
        MoveInDirection(direction, m_moveSpeed);
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageMove 상태 물리 행동 (Gray 전용)
    // - 레이캐스트로 감지된 방향으로 충돌할 때까지 직진
    // - 플레이어를 향해 추적하지 않음
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::ExecuteEngageMoveBehaviorPhysics()
    {
        if (!m_rigidbody) return;

        // 감지된 방향으로 직진
        MoveInDirection(m_engageDirection, m_moveSpeed);
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
    // RaycastAll 사용 - 자기 자신을 제외하고 Player 레이어만 감지
    // 감지 성공 시 m_engageDirection에 방향 저장
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundGray::DetectPlayerWithRaycast()
    {
        engine::PhysicsSystem& physicsSystem = engine::SystemManager::Get().GetPhysicsSystem();

        engine::Vector3 origin = GetTransform()->GetWorldPosition();
        origin.y += 1.0f;  // 약간 위에서 시작 (바닥 충돌 방지)

        // 4방향 레이캐스트
        const engine::Vector3 directions[4] = {
            engine::Vector3(1.0f, 0.0f, 0.0f),   // +X
            engine::Vector3(-1.0f, 0.0f, 0.0f),  // -X
            engine::Vector3(0.0f, 0.0f, 1.0f),   // +Z
            engine::Vector3(0.0f, 0.0f, -1.0f)   // -Z
        };

        engine::GameObject* selfGO = GetGameObject();  // 자기 자신 제외용
        
        for (const auto& dir : directions)
        {
            // RaycastAll로 모든 충돌 수집
            std::vector<engine::RaycastHit> allHits;
            physicsSystem.RaycastAll(origin, dir, m_raycastDetectionRange, allHits, engine::PhysicsLayer::Mask::All);
            
            // All 레이어 결과에서 Player 레이어만 필터링
            for (auto& h : allHits)
            {
                if (h.hasHit && h.gameObject.Get() && h.gameObject.Get() != selfGO && h.collider.Get())
                {
                    uint32_t hitLayer = h.collider.Get()->GetLayer();
                    if (hitLayer == engine::PhysicsLayer::Index::Player)
                    {
                        // 감지된 방향 저장 (EngageMove에서 사용)
                        m_engageDirection = dir;
                        return true;  // 플레이어 감지!
                    }
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
            if (m_fromEngageCollision)
            {
                // EngageMove에서 충돌로 전이한 경우 → 진행 방향의 90도 좌/우로 전환
                ChangeDirectionOnCollision();
                ResetMoveDuration();
                OnDirectionChanged();  // 플레이어 무시 카운트 감소
                m_fromEngageCollision = false;
            }
            else
            {
                // 일반 IdleMove 진입 (Idle에서 전이 등) → 랜덤 방향
                InitializeIdleMove();
            }
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
