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
        // 현재 상태가 EngageMove가 아니면 반사 처리 안함
        // ─────────────────────────────────────────────
        std::string currentState = GetCurrentState();
        if (currentState != "EngageMove")
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
    // - EngageStop, EngageAttack, IdleMove, Repositioning 상태 없음
    // - Idle → EngageMove → (반사 반복) → Fragile → Dead/Idle
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::InitializeFSM()
    {
        if (!m_logicFSM) return;

        // ─────────────────────────────────────────────
        // 상태 정의 (Green 전용 - 최소화)
        // ─────────────────────────────────────────────
        AddFSMState("Idle", true);           // 기본 상태 (대기)
        AddFSMState("EngageMove", false);    // 대각선 등속 운동
        AddFSMState("Fragile", false);
        AddFSMState("Dead", false);

        // ─────────────────────────────────────────────
        // StateMap 업데이트 (Transition 추가 전에 필수!)
        // ─────────────────────────────────────────────
        m_logicFSM->Initialize();

        // ─────────────────────────────────────────────
        // 파라미터 정의 (Green에 필요한 것만)
        // ─────────────────────────────────────────────
        m_logicFSM->SetParameter("IdleTimerComplete", false);     // Idle 대기 시간 완료
        m_logicFSM->SetParameter("Fragile", m_isFragile);
        m_logicFSM->SetParameter("Die", m_isDead);

        // ─────────────────────────────────────────────
        // 전이 정의 (Green 전용)
        // ─────────────────────────────────────────────
        
        // Idle → EngageMove (대기 시간 완료)
        AddFSMTransition("Idle", "EngageMove", "IdleTimerComplete", BoolTrue());

        // Any → Fragile (HP 0, Fragile 트리거)
        AddFSMTransition("Idle", "Fragile", "Fragile", Trigger());
        AddFSMTransition("EngageMove", "Fragile", "Fragile", Trigger());

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
    // 총알 초기화 (Green은 발사 공격 안함 - 빈 구현)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::InitializeBullet()
    {
        // Green은 총알 발사 없음
        // 필요시 나중에 구현
    }

    // ═══════════════════════════════════════════════════════════════
    // 입력 처리 (Green 전용 - 최소화)
    // - 레이캐스트 감지 없음
    // - 공격 관련 검사 없음
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::ProcessInput()
    {
        // Green은 ProcessInput에서 할 일이 없음
        // - 플레이어 감지: 없음 (시작 시 랜덤 방향, 이후 반사만)
        // - 공격: 없음 (충돌 데미지만)
    }

    // ═══════════════════════════════════════════════════════════════
    // EngageMove 상태 물리 행동 (Green 전용)
    // - 대각선 방향으로 등속 운동
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundGreen::ExecuteEngageMoveBehaviorPhysics()
    {
        if (!m_rigidbody) return;

        // 대각선 방향으로 등속 이동
        MoveInDirection(m_moveDirectionVector, m_moveSpeed);
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
            // EngageMove 진입 시 항상 랜덤 대각선 방향 설정
            // (Idle → EngageMove 전이 시, 부활 후 포함)
            SetRandomDiagonalDirection();
        }
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
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Tier: Green");
        
        // 부모 클래스 OnGui 호출
        MonsterRoundType::OnGui();
        
        // Green 전용 설정
        ImGui::Separator();
        ImGui::Text("=== Green Movement Settings ===");
        ImGui::DragFloat("Damage Cooldown", &m_damageCooldown, 0.1f, 0.1f, 5.0f);
        
        // 런타임 정보
        ImGui::Separator();
        ImGui::Text("=== Green Runtime ===");
        const char* diagDirNames[] = { "+X+Z (RightUp)", "+X-Z (RightDown)", "-X+Z (LeftUp)", "-X-Z (LeftDown)" };
        ImGui::Text("Current Direction: %s", diagDirNames[static_cast<int>(m_currentDiagonalDirection)]);
        ImGui::Text("Direction Vector: (%.2f, %.2f, %.2f)", 
            m_moveDirectionVector.x, m_moveDirectionVector.y, m_moveDirectionVector.z);
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
        
        // Green 전용 데이터 로드
        m_damageCooldown = j.value("DamageCooldown", 1.0f);
    }
}
