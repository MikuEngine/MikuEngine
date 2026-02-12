#include "GamePCH.h"
#include "MonsterRoundGray.h"

#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/CornerTrigger.h"
#include "Script/Interface/IDamageable.h"

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

        if (auto* collider = GetGameObject()->GetComponent<engine::Collider>())
        {
            collider->SetLayer(engine::PhysicsLayer::Index::EnemyNonPath);
        }

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

        auto* collider = info.collider.Get();
        if (!collider) return;

        const uint32_t layer = collider->GetLayer();

        // 플레이어 충돌 데미지는 유지
        if (layer == engine::PhysicsLayer::Index::Player)
        {
            auto* player = info.gameObject->GetComponent<PlayerControllerScript>();
            if (player)
            {
                float elapsedSinceLastDamage = engine::Time::GetElapsedSeconds(m_lastDamageTime);
                if (elapsedSinceLastDamage >= m_damageCooldown)
                {
                    player->TakeDamage(m_attackDamage);
                    m_lastDamageTime = engine::Time::GetTimestamp();
                }
            }
        }

        const std::string currentState = GetCurrentState();
        if (currentState != "IdleMove" && currentState != "EngageMove")
        {
            return;
        }

        if (layer == engine::PhysicsLayer::Index::Environment)
        {
            if (!m_lastCollisionWallName.empty() && info.gameObject->GetName() == m_lastCollisionWallName)
            {
                return;
            }

            // Environment 충돌 시 기존 90도 회전 로직 복구
            engine::Vector3 normal = engine::Vector3::Zero;
            for (const auto& contact : info.contacts)
            {
                normal = contact.normal;
                normal.y = 0.0f;
                if (normal.LengthSquared() > 0.0001f)
                {
                    normal.Normalize();
                    break;
                }
            }

            if (normal.LengthSquared() < 0.0001f)
            {
                engine::Vector3 myPos = GetTransform()->GetWorldPosition();
                engine::Vector3 otherPos = info.gameObject->GetTransform()->GetWorldPosition();
                normal = myPos - otherPos;
                normal.y = 0.0f;
                if (normal.LengthSquared() > 0.0001f)
                {
                    normal.Normalize();
                }
                else
                {
                    normal = GetDirectionVector();
                }
            }

            float absX = std::abs(normal.x);
            float absZ = std::abs(normal.z);
            MoveDirection collisionBaseDir = m_currentDirection;
            if (absX >= absZ)
            {
                collisionBaseDir = (normal.x > 0.0f) ? MoveDirection::PlusX : MoveDirection::MinusX;
            }
            else
            {
                collisionBaseDir = (normal.z > 0.0f) ? MoveDirection::PlusZ : MoveDirection::MinusZ;
            }

            m_currentDirection = collisionBaseDir;
            ChangeDirectionOnCollision();
            ResetMoveDuration();
            OnDirectionChanged();
            m_lastCollisionWallName = info.gameObject->GetName();

            if (currentState == "EngageMove")
            {
                StartPlayerIgnore();
                m_fromEngageCollision = true;
                if (m_logicFSM)
                {
                    m_logicFSM->SetParameter("ReturnToIdleMove", true);
                }
            }
            return;
        }

        if (layer != engine::PhysicsLayer::Index::SubWall)
        {
            return;
        }

        const std::string subWallName = info.gameObject->GetName();
        if (!m_lastCollisionWallName.empty() && subWallName == m_lastCollisionWallName)
        {
            return;
        }

        m_currentDirection = DetermineDirectionFromSubWall(subWallName);
        ResetMoveDuration();
        OnDirectionChanged();
        m_lastCollisionWallName = subWallName;

        if (currentState == "EngageMove")
        {
            StartPlayerIgnore();
            m_fromEngageCollision = true;
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("ReturnToIdleMove", true);
            }
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
        // 상태 정의 (Gray 전용 - 공격 상태 없음, Fragile 없음)
        // ─────────────────────────────────────────────
        AddFSMState("Idle", true);           // 기본 상태 (대기)
        AddFSMState("IdleMove", false);      // 맵 배회
        AddFSMState("EngageMove", false);    // 추적 이동 (충돌까지 직진)
        AddFSMState("Repositioning", false); // 위치 보정
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

        // Any → Dead (Round 타입은 Fragile 없이 바로 Dead로 전이)
        AddFSMTransition("Idle", "Dead", "Die", Trigger());
        AddFSMTransition("IdleMove", "Dead", "Die", Trigger());
        AddFSMTransition("EngageMove", "Dead", "Die", Trigger());
        AddFSMTransition("Repositioning", "Dead", "Die", Trigger());

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
        // 이동 시간 업데이트
        m_moveDurationTimer += deltaTime;
        
        // 이동 지속 시간 완료 → 랜덤 방향 전환
        if (m_moveDurationTimer >= m_moveDuration)
        {
            ChangeDirectionRandom();
            ResetMoveDuration();
            OnDirectionChanged();  // 플레이어 무시 카운트 감소
        }
        
        // 맵 경계 체크 (주기적)
        if (IsOutOfBounds())
        {
            StartRepositioning();
            return;  // Repositioning으로 전이
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

        // EngageMove도 현재 방향 벡터 기반으로 이동
        MoveInDirection(GetDirectionVector(), m_moveSpeed);
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
        m_lastCollisionWallName.clear();  // 방향 바뀌면 중복 충돌 체크 리셋
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 시 90도 방향 전환
    // - 코너트리거에서 결정된 방향이 있으면 그 방향 사용
    // - 없으면 좌/우 랜덤 선택
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::ChangeDirectionOnCollision()
    {
        // 랜덤 좌/우 90도 회전
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
        
        // 방향 전환 후 장애물 체크 (즉각 대응)
        if (HasObstacleAhead(m_obstacleCheckDistance))
        {
            // 새 방향에도 장애물 → 다른 안전한 방향 찾기
            if (!TryFindSafeDirection())
            {
                // 모든 방향이 막힘 → Repositioning 필요
                StartRepositioning();
            }
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
    // RaycastAll 사용 - 자기 자신을 제외
    // 장애물(SubWall, Environment, Enemy)이 플레이어보다 가까우면 감지 실패
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
            
            // 자기 자신 제외 및 유효한 히트만 필터링
            std::vector<engine::RaycastHit> validHits;
            for (auto& h : allHits)
            {
                if (h.hasHit && h.gameObject.Get() && h.gameObject.Get() != selfGO && h.collider.Get())
                {
                    validHits.push_back(h);
                }
            }
            
            // 거리순 정렬 (가까운 것부터)
            std::sort(validHits.begin(), validHits.end(),
                [](const engine::RaycastHit& a, const engine::RaycastHit& b) {
                    return a.distance < b.distance;
                });
            
            // 가장 가까운 것부터 확인
            for (auto& h : validHits)
            {
                uint32_t hitLayer = h.collider.Get()->GetLayer();
                
                if (hitLayer == engine::PhysicsLayer::Index::Player)
                {
                    // 플레이어가 가장 가까움 → 감지 성공!
                    m_engageDirection = dir;
                    return true;
                }
                else if (hitLayer == engine::PhysicsLayer::Index::SubWall ||
                         hitLayer == engine::PhysicsLayer::Index::Environment ||
                         h.gameObject->GetInterface<IDamageable>() != nullptr)
                {
                    // 장애물이 가려서 이 방향 실패 → 다음 방향 확인
                    break;
                }
                // 그 외 레이어(Trigger 등)는 무시하고 계속 확인
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
    // SubWall 오브젝트 이름으로 방향 결정
    // 
    // 명명규칙:
    //   Col_Left_Front~  : 맵 좌상단 벽 (−X 진행 → −Z, +Z 진행 → +X)
    //   Col_Right_Front~ : 맵 우상단 벽 (+X 진행 → −Z, +Z 진행 → −X)
    //   Col_Left_Back~   : 맵 좌하단 벽 (−X 진행 → +Z, −Z 진행 → +X)
    //   Col_Right_Back~  : 맵 우하단 벽 (+X 진행 → +Z, −Z 진행 → −X)
    //
    // 해당하지 않는 진행방향에서의 충돌은 현재 방향 유지 (무시)
    // ═══════════════════════════════════════════════════════════════
    MonsterRoundGray::MoveDirection MonsterRoundGray::DetermineDirectionFromSubWall(const std::string& subWallName)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> turnDist(0, 1);
        const bool choosePlusX = (turnDist(gen) == 0);

        // Front/Back은 정면 충돌 시 ±X 랜덤
        if (subWallName == "Collider_SubWall_Front")
        {
            return choosePlusX ? MoveDirection::PlusX : MoveDirection::MinusX;
        }

        if (subWallName == "Collider_SubWall_Back")
        {
            return choosePlusX ? MoveDirection::PlusX : MoveDirection::MinusX;
        }

        // 좌상단 대각 경계: -X 진행 충돌이면 -Z, +Z 진행 충돌이면 +X
        if (subWallName == "Collider_SubWall_FrontLeft")
        {
            if (m_currentDirection == MoveDirection::MinusX) return MoveDirection::MinusZ;
            if (m_currentDirection == MoveDirection::PlusZ)  return MoveDirection::PlusX;
            // 이레귤러 충돌 안전장치(현재 방향 유지)는 일단 제외
            // return m_currentDirection;
        }

        // 우상단 대각 경계: +X 진행 충돌이면 -Z, +Z 진행 충돌이면 -X
        if (subWallName == "Collider_SubWall_FrontRight")
        {
            if (m_currentDirection == MoveDirection::PlusX)  return MoveDirection::MinusZ;
            if (m_currentDirection == MoveDirection::PlusZ)  return MoveDirection::MinusX;
            // 이레귤러 충돌 안전장치(현재 방향 유지)는 일단 제외
            // return m_currentDirection;
        }

        // 좌하단 대각 경계: -X 진행 충돌이면 +Z, -Z 진행 충돌이면 +X
        if (subWallName == "Collider_SubWall_BackLeft")
        {
            if (m_currentDirection == MoveDirection::MinusX) return MoveDirection::PlusZ;
            if (m_currentDirection == MoveDirection::MinusZ) return MoveDirection::PlusX;
            // 이레귤러 충돌 안전장치(현재 방향 유지)는 일단 제외
            // return m_currentDirection;
        }

        // 우하단 대각 경계: +X 진행 충돌이면 +Z, -Z 진행 충돌이면 -X
        if (subWallName == "Collider_SubWall_BackRight")
        {
            if (m_currentDirection == MoveDirection::PlusX)  return MoveDirection::PlusZ;
            if (m_currentDirection == MoveDirection::MinusZ) return MoveDirection::MinusX;
            // 이레귤러 충돌 안전장치(현재 방향 유지)는 일단 제외
            // return m_currentDirection;
        }

        return m_currentDirection;
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
                // EngageMove 충돌에서 넘어온 경우:
                // 충돌 시점에 이미 방향 전환이 끝났으므로 유지하고 타이머만 재설정
                ResetMoveDuration();
                m_fromEngageCollision = false;
            }
            else
            {
                // 일반 IdleMove 진입 (Idle에서 전이 등) → 랜덤 방향
                InitializeIdleMove();
            }
        }
        else if (state == "EngageMove")
        {
            // 플레이어 감지 방향을 축 이동 방향으로 동기화
            if (std::abs(m_engageDirection.x) > std::abs(m_engageDirection.z))
            {
                m_currentDirection = (m_engageDirection.x >= 0.0f) ? MoveDirection::PlusX : MoveDirection::MinusX;
            }
            else
            {
                m_currentDirection = (m_engageDirection.z >= 0.0f) ? MoveDirection::PlusZ : MoveDirection::MinusZ;
            }
        }
        else if (state == "Repositioning")
        {
            // Repositioning은 StartRepositioning()에서 이미 초기화됨
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("RepositioningComplete", false);
                m_logicFSM->SetParameter("NeedRepositioning", false);
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
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "[No Bullet Attack - Contact Damage Only]");
        
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
        
        ImGui::Separator();
        ImGui::Text("=== Wall/SubWall Collision ===");
        ImGui::Text("Last Collision Object: %s", m_lastCollisionWallName.empty() ? "None" : m_lastCollisionWallName.c_str());
        
        ImGui::Separator();
        ImGui::Text("=== Boundary & Repositioning ===");
        ImGui::DragFloat("Boundary Min X", &m_boundaryMinX, 1.0f, -500.0f, 500.0f);
        ImGui::DragFloat("Boundary Max X", &m_boundaryMaxX, 1.0f, -500.0f, 500.0f);
        ImGui::DragFloat("Boundary Min Z", &m_boundaryMinZ, 1.0f, -500.0f, 500.0f);
        ImGui::DragFloat("Boundary Max Z", &m_boundaryMaxZ, 1.0f, -500.0f, 500.0f);
        ImGui::DragFloat("Obstacle Check Dist", &m_obstacleCheckDistance, 0.1f, 0.5f, 10.0f);
        ImGui::DragFloat("Repositioning Speed", &m_repositioningSpeed, 0.5f, 1.0f, 20.0f);
        ImGui::DragFloat("Repositioning Duration", &m_repositioningDuration, 0.1f, 0.5f, 10.0f);
        
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
        
        // 맵 경계 및 복원 시스템
        j["BoundaryMinX"] = m_boundaryMinX;
        j["BoundaryMaxX"] = m_boundaryMaxX;
        j["BoundaryMinZ"] = m_boundaryMinZ;
        j["BoundaryMaxZ"] = m_boundaryMaxZ;
        j["ObstacleCheckDistance"] = m_obstacleCheckDistance;
        j["RepositioningSpeed"] = m_repositioningSpeed;
        j["RepositioningDuration"] = m_repositioningDuration;
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
        
        // 맵 경계 및 복원 시스템
        m_boundaryMinX = j.value("BoundaryMinX", -50.0f);
        m_boundaryMaxX = j.value("BoundaryMaxX", 50.0f);
        m_boundaryMinZ = j.value("BoundaryMinZ", -50.0f);
        m_boundaryMaxZ = j.value("BoundaryMaxZ", 50.0f);
        m_obstacleCheckDistance = j.value("ObstacleCheckDistance", 2.0f);
        m_repositioningSpeed = j.value("RepositioningSpeed", 5.0f);
        m_repositioningDuration = j.value("RepositioningDuration", 2.0f);
    }

    // ═══════════════════════════════════════════════════════════════
    // 현재 방향으로 장애물 확인 (레이캐스트)
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundGray::HasObstacleAhead(float distance) const
    {
        engine::PhysicsSystem& physicsSystem = engine::SystemManager::Get().GetPhysicsSystem();
        
        engine::Vector3 origin = GetTransform()->GetWorldPosition();
        origin.y += 1.0f;  // 약간 위에서 시작
        
        engine::Vector3 direction = GetDirectionVector();
        
        std::vector<engine::RaycastHit> allHits;
        physicsSystem.RaycastAll(origin, direction, distance, allHits, engine::PhysicsLayer::Mask::All);
        
        engine::GameObject* selfGO = GetGameObject();
        
        for (auto& h : allHits)
        {
            if (h.hasHit && h.gameObject.Get() && h.gameObject.Get() != selfGO && h.collider.Get())
            {
                uint32_t hitLayer = h.collider.Get()->GetLayer();
                if (hitLayer == engine::PhysicsLayer::Index::SubWall ||
                    hitLayer == engine::PhysicsLayer::Index::Environment ||
                    h.gameObject->GetInterface<IDamageable>() != nullptr)
                {
                    return true;  // 장애물 발견!
                }
            }
        }
        
        return false;
    }

    // ═══════════════════════════════════════════════════════════════
    // 안전한 방향 찾기 (4방향 체크)
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundGray::TryFindSafeDirection()
    {
        const MoveDirection allDirections[4] = {
            MoveDirection::PlusX,
            MoveDirection::MinusX,
            MoveDirection::PlusZ,
            MoveDirection::MinusZ
        };
        
        // 현재 방향 제외하고 체크
        for (auto dir : allDirections)
        {
            if (dir == m_currentDirection) continue;  // 현재 방향 제외
            
            // 임시로 방향 설정하고 체크
            MoveDirection prevDir = m_currentDirection;
            m_currentDirection = dir;
            
            if (!HasObstacleAhead(m_obstacleCheckDistance))
            {
                // 안전한 방향 발견!
                return true;
            }
            
            // 복원
            m_currentDirection = prevDir;
        }
        
        return false;  // 모든 방향이 막힘
    }

    // ═══════════════════════════════════════════════════════════════
    // 맵 밖으로 나갔는지 확인 (사각형 경계)
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundGray::IsOutOfBounds() const
    {
        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        
        return myPos.x < m_boundaryMinX || myPos.x > m_boundaryMaxX ||
               myPos.z < m_boundaryMinZ || myPos.z > m_boundaryMaxZ;
    }

    // ═══════════════════════════════════════════════════════════════
    // Repositioning 시작 (가장 가까운 경계 안쪽으로)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::StartRepositioning()
    {
        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        engine::Vector3 targetPos = myPos;
        
        // 경계를 벗어난 축을 경계 안쪽으로 보정
        if (myPos.x < m_boundaryMinX)
            targetPos.x = m_boundaryMinX + 2.0f;  // 경계 + 여유
        else if (myPos.x > m_boundaryMaxX)
            targetPos.x = m_boundaryMaxX - 2.0f;
        
        if (myPos.z < m_boundaryMinZ)
            targetPos.z = m_boundaryMinZ + 2.0f;
        else if (myPos.z > m_boundaryMaxZ)
            targetPos.z = m_boundaryMaxZ - 2.0f;
        
        m_repositioningDirection = targetPos - myPos;
        m_repositioningDirection.y = 0.0f;
        
        if (m_repositioningDirection.LengthSquared() > 0.0001f)
        {
            m_repositioningDirection.Normalize();
        }
        else
        {
            // 이미 경계 안 → 맵 중심으로
            float centerX = (m_boundaryMinX + m_boundaryMaxX) * 0.5f;
            float centerZ = (m_boundaryMinZ + m_boundaryMaxZ) * 0.5f;
            m_repositioningDirection = engine::Vector3(centerX - myPos.x, 0.0f, centerZ - myPos.z);
            
            if (m_repositioningDirection.LengthSquared() > 0.0001f)
                m_repositioningDirection.Normalize();
            else
                m_repositioningDirection = engine::Vector3(1.0f, 0.0f, 0.0f);
        }
        
        m_repositioningTimer = 0.0f;
        
        if (m_logicFSM)
        {
            m_logicFSM->SetParameter("NeedRepositioning", true);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // Repositioning 상태 비물리 행동
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::ExecuteRepositioningBehaviorNonPhysics(float deltaTime)
    {
        m_repositioningTimer += deltaTime;
        
        // 경계 안으로 들어왔거나 시간 초과
        if (!IsOutOfBounds() || m_repositioningTimer >= m_repositioningDuration)
        {
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("RepositioningComplete", true);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // Repositioning 상태 물리 행동
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGray::ExecuteRepositioningBehaviorPhysics()
    {
        if (!m_rigidbody) return;
        
        // 중심 방향으로 이동
        MoveInDirection(m_repositioningDirection, m_repositioningSpeed);
    }
}
