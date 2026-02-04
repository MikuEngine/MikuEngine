#include "GamePCH.h"
#include "MonsterRoundGreen.h"

#include "Script/CharacterScript/Player/PlayerControllerScript.h"
#include "Script/CharacterScript/Common/BulletFactory.h"

#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Object/Component/Renderer/SkeletalMeshRenderer.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Physics/PhysicsLayer.h>
#include <Engine/Core/System/MyTime.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::Awake()
    {
        MonsterRoundType::Awake();
        
        // Green 등급 고정
        m_monsterTier = MonsterTier::Green;
        
        // m_fireRate는 Load()에서 씬 파일 값 또는 kDefaultFireRate로 설정됨
        // Awake()에서 강제 설정하면 씬 파일 값을 덮어쓰므로 여기서 설정하지 않음
    }

    void MonsterRoundGreen::Start()
    {
        MonsterRoundType::Start();

        // Green 등급 색상 설정 (녹색)
        // SkeletalMeshRenderer만 SetBaseColor 지원
        if (m_meshType == RoundMeshType::Skeletal)
        {
            if (auto* meshRenderer = GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>())
            {
                meshRenderer->SetBaseColor(engine::Vector4(0.0f, 1.0f, 0.0f, 1.0f));
            }
        }
        // StaticMeshRenderer는 현재 SetBaseColor 미지원
        
        // 초기 방향 벡터 정규화
        m_moveDirectionVector = GetDiagonalDirectionVector(m_currentDiagonalDirection);
        m_moveDirectionVector.Normalize();
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::OnCollisionEnter(const engine::CollisionInfo& info)
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
        // 플레이어 충돌 시 데미지 처리 (주석 처리)
        // ─────────────────────────────────────────────
        auto* player = info.gameObject->GetComponent<PlayerControllerScript>();
        if (player)
        {
            float elapsedSinceLastDamage = engine::Time::GetElapsedSeconds(m_lastDamageTime);
            if (elapsedSinceLastDamage >= m_damageCooldown)
            {
                // TODO: 데미지 시스템 활성화 시 주석 해제
                // player->TakeDamage(static_cast<int>(m_attackDamage));
                m_lastDamageTime = engine::Time::GetTimestamp();
            }
        }
        
        // ─────────────────────────────────────────────
        // 현재 상태가 EngageMove 또는 EngageAttack이 아니면 반사 처리 안함
        // ─────────────────────────────────────────────
        std::string currentState = GetCurrentState();
        if (currentState != "EngageMove" && currentState != "EngageAttack")
        {
            return;
        }
        
        // ─────────────────────────────────────────────
        // 충돌 노말 추출
        // ─────────────────────────────────────────────
        engine::Vector3 collisionNormal = engine::Vector3::Zero;
        for (const auto& contact : info.contacts)
        {
            // 노말 반전: A→B 방향이므로, 몬스터 입장에서는 반대 방향
            engine::Vector3 normal = -contact.normal;
            normal.y = 0.0f;  // 수평 성분만
            if (normal.LengthSquared() > 0.0001f)
            {
                collisionNormal = normal;
                collisionNormal.Normalize();
                break;
            }
        }
        
        // 노말이 없으면 위치 기반으로 계산
        if (collisionNormal.LengthSquared() < 0.0001f)
        {
            engine::Vector3 myPos = GetTransform()->GetWorldPosition();
            engine::Vector3 otherPos = info.gameObject->GetTransform()->GetWorldPosition();
            collisionNormal = otherPos - myPos;
            collisionNormal.y = 0.0f;
            if (collisionNormal.LengthSquared() > 0.0001f)
            {
                collisionNormal.Normalize();
            }
            else
            {
                return;  // 유효한 노말 없음
            }
        }
        
        // ─────────────────────────────────────────────
        // 노말을 X축 또는 Z축으로 스냅
        // ─────────────────────────────────────────────
        engine::Vector3 snappedNormal = SnapNormalToAxis(collisionNormal);
        
        // ─────────────────────────────────────────────
        // 정반사 계산: R = V - 2(V·N)N
        // ─────────────────────────────────────────────
        m_moveDirectionVector = ReflectDirection(m_moveDirectionVector, snappedNormal);
        m_moveDirectionVector.Normalize();
        
        // ─────────────────────────────────────────────
        // 열거형 동기화
        // ─────────────────────────────────────────────
        UpdateDiagonalDirectionFromVector();
    }

    // ═══════════════════════════════════════════════════════════════
    // Green 전용 FSM 초기화
    // - Idle → EngageMove → EngageAttack → EngageMove (루프)
    // - Fragile 전이는 우선순위 높음 (트리거)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::InitializeFSM()
    {
        if (!m_logicFSM) return;

        // ─────────────────────────────────────────────
        // 상태 정의 (Green 전용)
        // ─────────────────────────────────────────────
        AddFSMState("Idle", true);           // 기본 상태 (대기)
        AddFSMState("EngageMove", false);    // 대각선 등속 운동
        AddFSMState("EngageAttack", false);  // 공격 (이동 유지)
        AddFSMState("Fragile", false);
        AddFSMState("Dead", false);

        // ─────────────────────────────────────────────
        // StateMap 업데이트 (Transition 추가 전에 필수!)
        // ─────────────────────────────────────────────
        m_logicFSM->Initialize();

        // ─────────────────────────────────────────────
        // 파라미터 정의
        // ─────────────────────────────────────────────
        m_logicFSM->SetParameter("IdleTimerComplete", false);     // Idle 대기 시간 완료
        m_logicFSM->SetParameter("PlayerInRange", m_isPlayerInRange);
        m_logicFSM->SetParameter("CanFire", m_canFire);
        m_logicFSM->SetParameter("AttackComplete", false);
        m_logicFSM->SetParameter("Fragile", m_isFragile);
        m_logicFSM->SetParameter("Die", m_isDead);

        // ─────────────────────────────────────────────
        // 전이 정의
        // ─────────────────────────────────────────────
        
        // Idle → EngageMove (대기 시간 완료)
        AddFSMTransition("Idle", "EngageMove", "IdleTimerComplete", BoolTrue());
        
        // EngageMove → EngageAttack (사거리 진입 + 발사 가능)
        AddFSMTransition("EngageMove", "EngageAttack", "PlayerInRange", BoolTrue(), "CanFire", BoolTrue());
        
        // EngageAttack → EngageMove (공격 완료, 다음 프레임)
        AddFSMTransition("EngageAttack", "EngageMove", "AttackComplete", BoolTrue());

        // Fragile 전이 (트리거 - 우선순위 높음)
        AddFSMTransition("Idle", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageMove", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageAttack", "Fragile", "Fragile", Trigger());

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
    // 총알 초기화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::InitializeBullet()
    {
        // Green: 포물선 총알
        m_bulletParams.type = BulletType::Parabolic;
        m_bulletParams.lifetime = m_bulletLifetime;
        m_bulletParams.damage = m_attackDamage;

        // ─────────────────────────────────────────────
        // 포물선 전용 파라미터
        // - launchAngle은 Attack() 시점에 거리 기반 자동 계산
        // - speed, ownGravity는 에디터에서 설정한 값 사용
        // ─────────────────────────────────────────────
        m_bulletParams.speed = m_parabolicSpeed;      // 에디터 설정값
        m_bulletParams.launchAngle = 45.0f;           // 기본값 (Attack에서 자동 계산)
        m_bulletParams.ownGravity = m_ownGravity;     // 에디터 설정값
    }

    // ═══════════════════════════════════════════════════════════════
    // 입력 처리 (Green 전용)
    // - 플레이어 찾기
    // - 사거리/쿨타임 체크
    // - FSM 파라미터 업데이트
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::ProcessInput()
    {
        if (!m_logicFSM) return;

        // 플레이어를 찾지 못했으면 재탐색
        if (!m_targetPlayer)
        {
            FindPlayer();
        }

        // 플레이어 공격 사거리 체크
        m_isPlayerInRange = IsPlayerInRange();
        
        // 발사 가능 여부 체크 (쿨타임)
        m_canFire = (m_fireTimer <= 0.0f);

        // FSM 파라미터 업데이트
        m_logicFSM->SetParameter("PlayerInRange", m_isPlayerInRange);
        m_logicFSM->SetParameter("CanFire", m_canFire);
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageMove 상태 물리 행동
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::ExecuteEngageMoveBehaviorPhysics()
    {
        ExecuteDiagonalMovement();
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageAttack 상태 비물리 행동
    // - Attack() 호출
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::ExecuteEngageAttackBehaviorNonPhysics(float deltaTime)
    {
        Attack(deltaTime);
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageAttack 상태 물리 행동
    // - EngageMove와 동일한 이동 로직 (반사 포함)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::ExecuteEngageAttackBehaviorPhysics()
    {
        ExecuteDiagonalMovement();
    }

    // ═══════════════════════════════════════════════════════════════
    // 공격 (Green 전용)
    // - 플레이어 방향으로 포물선 발사
    // - 발사 후 다음 프레임에 EngageMove로 복귀
    // 
    // 포물선 물리:
    //   - 에디터 설정: m_parabolicSpeed (속력), m_ownGravity (중력)
    //   - 자동 계산: launchAngle (플레이어 거리 기반, m_useHighArc로 선택)
    //   - PhysX: AddForce로 자체 중력 적용
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::Attack(float deltaTime)
    {
        // 발사 오프셋 설정
        float bulletStartOffsetForward = 0.3f;
        float bulletStartOffsetY = 1.5f;

        // 발사 가능 상태 (쿨타임 완료)
        if (m_fireTimer <= 0.0f)
        {
            if (m_bulletFactory && m_targetPlayer && m_targetPlayer->GetGameObject())
            {
                // 플레이어 방향으로 발사
                engine::Vector3 direction = CalculateDirectionToPlayer();
                engine::Vector3 monsterPos = GetTransform()->GetWorldPosition();
                
                // 실제 발사 위치 계산 (오프셋 적용)
                engine::Vector3 bulletStartPos = monsterPos + direction * bulletStartOffsetForward;
                bulletStartPos.y = bulletStartOffsetY;
                
                // ─────────────────────────────────────────────
                // Parabolic 타입: 발사각 자동 계산
                // - 착탄점: 플레이어 XZ, Y=0
                // - 에디터 설정: m_parabolicSpeed, m_ownGravity
                // - 자동 계산: launchAngle (m_useHighArc로 높은/낮은 선택)
                // ─────────────────────────────────────────────
                if (m_bulletParams.type == BulletType::Parabolic)
                {
                    // 착탄점 설정 (플레이어 XZ, Y=0)
                    engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();
                    engine::Vector3 targetPos(playerPos.x, 0.0f, playerPos.z);
                    
                    // 거리 계산 (디버그용)
                    float dx = targetPos.x - bulletStartPos.x;
                    float dz = targetPos.z - bulletStartPos.z;
                    float distance = std::sqrt(dx * dx + dz * dz);
                    
                    // 발사각 자동 계산 (m_useHighArc에 따라 높은/낮은 선택)
                    float angleRad = 0.0f;
                    CalculateParabolicLaunchAngle(bulletStartPos, targetPos, angleRad);
                    
                    // BulletParams에 값 설정
                    m_bulletParams.speed = m_parabolicSpeed;
                    m_bulletParams.launchAngle = angleRad * 180.0f / 3.14159265f;  // 도(degree)로 변환
                    m_bulletParams.ownGravity = m_ownGravity;
                    
                    // 디버그: 거리와 발사각 확인
                    LOG_PRINT("[Attack] Distance to player: {:.2f}m, LaunchAngle: {:.1f}deg",
                        distance, m_bulletParams.launchAngle);
                    LOG_PRINT("[Attack] BulletStart=({:.2f}, {:.2f}, {:.2f}), Target=({:.2f}, {:.2f}, {:.2f})",
                        bulletStartPos.x, bulletStartPos.y, bulletStartPos.z,
                        targetPos.x, targetPos.y, targetPos.z);
                }
                
                // 실제 발사 위치로 총알 발사
                m_bulletFactory->ParabolicFireMonster(bulletStartPos, direction, m_bulletParams);

                // 공격 애니메이션 재생 (SkeletalMesh 사용 시에만)
                if (HasAnimation() && !m_animName_EngageAttack.empty())
                {
                    m_skeletalAnimator->Play(m_animName_EngageAttack, false, 0, 1.0f);
                }

                // 발사 쿨타임 리셋
                m_fireTimer = m_fireRate;
            }
        }

        // 공격 완료 → 다음 프레임에 EngageMove로 복귀
        if (m_logicFSM)
        {
            m_logicFSM->SetParameter("AttackComplete", true);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 행동 제한 (Green 전용)
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundGreen::CanMove() const
    {
        std::string state = GetCurrentState();
        // EngageMove, EngageAttack 모두 이동 가능
        return state == "EngageMove" || state == "EngageAttack";
    }

    bool MonsterRoundGreen::CanAttack() const
    {
        std::string state = GetCurrentState();
        return state == "EngageAttack" && !m_isFragile && !m_isDead;
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태 진입 콜백 오버라이드
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::OnStateEntered(const std::string& state)
    {
        // 부모 클래스 호출
        MonsterRoundType::OnStateEntered(state);
        
        if (state == "EngageMove")
        {
            // EngageMove 진입 시 AttackComplete 초기화
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("AttackComplete", false);
            }
            
            // Idle에서 처음 전이한 경우에만 랜덤 방향 설정
            // (EngageAttack에서 복귀 시에는 방향 유지)
            // 부모의 OnStateEntered에서 Idle 타이머가 리셋되므로,
            // Idle에서 온 건지 확인하기 위해 별도 플래그 필요 없음
            // → Idle 진입 시 방향을 랜덤으로 설정하는 방식으로 변경
        }
        else if (state == "Idle")
        {
            // Idle 진입 시 (시작/부활) 랜덤 방향 설정
            SetRandomDiagonalDirection();
        }
        else if (state == "EngageAttack")
        {
            // EngageAttack 진입 시 AttackComplete 초기화
            if (m_logicFSM)
            {
                m_logicFSM->SetParameter("AttackComplete", false);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 대각선 이동 실행 (EngageMove, EngageAttack 공용)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::ExecuteDiagonalMovement()
    {
        if (!m_rigidbody) return;

        // 대각선 방향으로 등속 이동
        MoveInDirection(m_moveDirectionVector, m_moveSpeed);
    }

    // ═══════════════════════════════════════════════════════════════
    // 랜덤 대각선 방향 설정
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::SetRandomDiagonalDirection()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0, 3);
        
        m_currentDiagonalDirection = static_cast<DiagonalDirection>(dist(gen));
        m_moveDirectionVector = GetDiagonalDirectionVector(m_currentDiagonalDirection);
        m_moveDirectionVector.Normalize();
    }

    // ═══════════════════════════════════════════════════════════════
    // 열거형 → 방향벡터 변환
    // ═══════════════════════════════════════════════════════════════
    engine::Vector3 MonsterRoundGreen::GetDiagonalDirectionVector(DiagonalDirection dir) const
    {
        switch (dir)
        {
        case DiagonalDirection::PlusXPlusZ:
            return engine::Vector3(1.0f, 0.0f, 1.0f);
        case DiagonalDirection::PlusXMinusZ:
            return engine::Vector3(1.0f, 0.0f, -1.0f);
        case DiagonalDirection::MinusXPlusZ:
            return engine::Vector3(-1.0f, 0.0f, 1.0f);
        case DiagonalDirection::MinusXMinusZ:
            return engine::Vector3(-1.0f, 0.0f, -1.0f);
        default:
            return engine::Vector3(1.0f, 0.0f, 1.0f);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 방향벡터 → 열거형 동기화
    // 반사 후 방향벡터가 변경되면 열거형도 맞춰줌
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::UpdateDiagonalDirectionFromVector()
    {
        // X, Z 부호로 판단
        bool plusX = (m_moveDirectionVector.x > 0.0f);
        bool plusZ = (m_moveDirectionVector.z > 0.0f);
        
        if (plusX && plusZ)
            m_currentDiagonalDirection = DiagonalDirection::PlusXPlusZ;
        else if (plusX && !plusZ)
            m_currentDiagonalDirection = DiagonalDirection::PlusXMinusZ;
        else if (!plusX && plusZ)
            m_currentDiagonalDirection = DiagonalDirection::MinusXPlusZ;
        else
            m_currentDiagonalDirection = DiagonalDirection::MinusXMinusZ;
    }

    // ═══════════════════════════════════════════════════════════════
    // 노말을 X축 또는 Z축으로 스냅
    // - |X| >= |Z| → X축 방향 (1,0,0) 또는 (-1,0,0)
    // - |Z| > |X|  → Z축 방향 (0,0,1) 또는 (0,0,-1)
    // ═══════════════════════════════════════════════════════════════
    engine::Vector3 MonsterRoundGreen::SnapNormalToAxis(const engine::Vector3& normal) const
    {
        float absX = std::abs(normal.x);
        float absZ = std::abs(normal.z);
        
        if (absX >= absZ)
        {
            // X축으로 스냅
            return engine::Vector3((normal.x >= 0.0f) ? 1.0f : -1.0f, 0.0f, 0.0f);
        }
        else
        {
            // Z축으로 스냅
            return engine::Vector3(0.0f, 0.0f, (normal.z >= 0.0f) ? 1.0f : -1.0f);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 정반사 계산: R = V - 2(V·N)N
    // V: 입사 방향 (현재 이동 방향)
    // N: 노말 (스냅된 축 방향)
    // R: 반사 방향
    // ═══════════════════════════════════════════════════════════════
    engine::Vector3 MonsterRoundGreen::ReflectDirection(const engine::Vector3& direction, const engine::Vector3& normal) const
    {
        float dotProduct = direction.Dot(normal);
        engine::Vector3 reflected = direction - 2.0f * dotProduct * normal;
        return reflected;
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::OnGui()
    {
        ImGui::Text("=== MonsterRoundGreen ===");
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Tier: Green (Parabolic)");
        
        // 부모 클래스 OnGui 호출
        MonsterRoundType::OnGui();
        
        // Green 전용 설정
        ImGui::Separator();
        ImGui::Text("=== Green Settings ===");
        ImGui::DragFloat("Damage Cooldown", &m_damageCooldown, 0.1f, 0.1f, 5.0f);

        // ─────────────────────────────────────────────
        // 포물선 설정 (Green은 항상 Parabolic)
        // - 편집 가능: 속력, 자체 중력
        // - 읽기 전용: 발사각 (플레이어 거리 기반 자동 계산)
        // ─────────────────────────────────────────────
        ImGui::Separator();
        ImGui::Text("=== Parabolic Bullet Settings ===");
        
        // 편집 가능한 설정
        ImGui::DragFloat("Bullet Speed", &m_parabolicSpeed, 0.5f, 1.0f, 50.0f, "%.1f m/s");
        ImGui::DragFloat("Own Gravity", &m_ownGravity, 0.1f, 1.0f, 30.0f, "%.1f m/s^2");
        
        // 최대 사거리 표시 (v² / g)
        float maxRange = (m_parabolicSpeed * m_parabolicSpeed) / m_ownGravity;
        ImGui::Text("Max Range (at 45 deg): %.1f m", maxRange);
        
        // 사거리 검증
        if (maxRange < m_AttackRange)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), 
                "WARNING: Max Range < Attack Range!");
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), 
                "Increase Speed or decrease Gravity/AttackRange");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), 
                "OK: Max Range >= Attack Range");
        }
        
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Runtime values (read-only):");
        
        // 읽기 전용: 마지막으로 계산된 값 표시
        ImGui::Text("  Launch Angle: %.1f deg", m_bulletParams.launchAngle);
        ImGui::Text("  Speed: %.1f m/s", m_bulletParams.speed);
        
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
            "(Angle calculated at fire time based on player distance)");
        
        // 런타임 정보
        ImGui::Separator();
        ImGui::Text("=== Green Runtime ===");
        const char* diagDirNames[] = { "+X+Z (RightUp)", "+X-Z (RightDown)", "-X+Z (LeftUp)", "-X-Z (LeftDown)" };
        ImGui::Text("Current Direction: %s", diagDirNames[static_cast<int>(m_currentDiagonalDirection)]);
        ImGui::Text("Direction Vector: (%.2f, %.2f, %.2f)", 
            m_moveDirectionVector.x, m_moveDirectionVector.y, m_moveDirectionVector.z);
        ImGui::Text("Player In Attack Range: %s", m_isPlayerInRange ? "Yes" : "No");
        ImGui::Text("Can Fire: %s", m_canFire ? "Yes" : "No");
        ImGui::Text("Fire Timer: %.2f / %.2f", m_fireTimer, m_fireRate);
    }

    void MonsterRoundGreen::Save(engine::json& j) const
    {
        MonsterRoundType::Save(j);
        
        // Green 전용 데이터 저장
        j["DamageCooldown"] = m_damageCooldown;
    }

    void MonsterRoundGreen::Load(const engine::json& j)
    {
        MonsterRoundType::Load(j);
        
        // ─────────────────────────────────────────────
        // Green 전용 기본값 적용 (씬 파일에 값이 없을 때만)
        // 우선순위: 씬 파일 값 > kDefault 상수
        // ─────────────────────────────────────────────
        if (!j.contains("FireRate"))
        {
            m_fireRate = kDefaultFireRate;  // 5.0f
        }
        // else: MonsterScript::Load()에서 씬 파일 값이 이미 로드됨
        
        // Green 전용 데이터 로드
        if (!j.contains("DamageCooldown"))
        {
            m_damageCooldown = kDefaultDamageCooldown;  // 1.0f
        }
        else
        {
            m_damageCooldown = j.value("DamageCooldown", kDefaultDamageCooldown);
        }
    }
}
