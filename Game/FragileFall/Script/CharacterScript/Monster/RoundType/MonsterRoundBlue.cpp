#include "GamePCH.h"
#include "MonsterRoundBlue.h"

#include "Script/CharacterScript/Player/PlayerControllerScript.h"

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
    void MonsterRoundBlue::Awake()
    {
        MonsterRoundType::Awake();
        
        // Blue 등급 고정
        m_monsterTier = MonsterTier::Blue;
    }

    void MonsterRoundBlue::Start()
    {
        MonsterRoundType::Start();

        // Blue 등급 색상 설정 (파란색)
        // SkeletalMeshRenderer만 SetBaseColor 지원
        if (m_meshType == RoundMeshType::Skeletal)
        {
            if (auto* meshRenderer = GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>())
            {
                meshRenderer->SetBaseColor(engine::Vector4(0.0f, 0.5f, 1.0f, 1.0f));
            }
        }
        // StaticMeshRenderer는 현재 SetBaseColor 미지원
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::OnCollisionEnter(const engine::CollisionInfo& info)
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
        // 현재 상태가 IdleMove가 아니면 충돌 방향 전환 안함
        // ─────────────────────────────────────────────
        std::string currentState = GetCurrentState();
        if (currentState != "IdleMove")
        {
            return;
        }
        
        // 충돌 발생 플래그 설정 (다음 Update/FixedUpdate에서 처리)
        m_collisionOccurred = true;
    }

    // ═══════════════════════════════════════════════════════════════
    // Blue 전용 FSM 초기화
    // - 현재: Idle → IdleMove → Fragile → Dead (Engage 관련 나중에 추가)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::InitializeFSM()
    {
        if (!m_logicFSM) return;

        // ─────────────────────────────────────────────
        // 상태 정의 (Blue - 현재 버전, Engage 없음)
        // ─────────────────────────────────────────────
        AddFSMState("Idle", true);           // 기본 상태 (대기)
        AddFSMState("IdleMove", false);      // 곡선 배회
        AddFSMState("Fragile", false);
        AddFSMState("Dead", false);
        // TODO: EngageMove, EngageAttack 나중에 추가

        // ─────────────────────────────────────────────
        // StateMap 업데이트 (Transition 추가 전에 필수!)
        // ─────────────────────────────────────────────
        m_logicFSM->Initialize();

        // ─────────────────────────────────────────────
        // 파라미터 정의 (Blue - 현재 버전)
        // ─────────────────────────────────────────────
        m_logicFSM->SetParameter("IdleTimerComplete", false);     // Idle 대기 시간 완료
        m_logicFSM->SetParameter("Fragile", m_isFragile);
        m_logicFSM->SetParameter("Die", m_isDead);
        // TODO: PlayerDetected 등 나중에 추가

        // ─────────────────────────────────────────────
        // 전이 정의 (Blue - 현재 버전)
        // ─────────────────────────────────────────────
        
        // Idle → IdleMove (대기 시간 완료)
        AddFSMTransition("Idle", "IdleMove", "IdleTimerComplete", BoolTrue());

        // Any → Fragile (HP 0, Fragile 트리거)
        AddFSMTransition("Idle", "Fragile", "Fragile", Trigger());
        AddFSMTransition("IdleMove", "Fragile", "Fragile", Trigger());

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
    // 입력 처리 (Blue - 현재 감지 로직 없음)
    // TODO: 플레이어 감지 로직 나중에 추가
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ProcessInput()
    {
        // 현재는 플레이어 감지 없음
        // 나중에 EngageMove 추가 시 구현
    }

    // ═══════════════════════════════════════════════════════════════
    // IdleMove 상태 비물리 행동
    // - 타이머 관리
    // - 충돌 처리
    // - Roaming 파라미터 갱신
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ExecuteIdleMoveBehaviorNonPhysics(float deltaTime)
    {
        // 충돌 발생 처리
        if (m_collisionOccurred)
        {
            ChangeDirectionOnCollision();
            ResetRoamingParameters();
            m_collisionOccurred = false;
        }
        
        // Roaming 타이머 업데이트
        m_roamingTimer += deltaTime;
        
        // Roaming 지속 시간 완료 → 새로운 파라미터로 갱신
        if (m_roamingTimer >= m_roamingDuration)
        {
            ResetRoamingParameters();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // IdleMove 상태 물리 행동
    // - 방향 회전 (곡선 이동)
    // - 이동
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ExecuteIdleMoveBehaviorPhysics()
    {
        if (!m_rigidbody) return;

        float fixedDeltaTime = engine::Time::FixedDeltaTime();
        
        // ─────────────────────────────────────────────
        // 1. 방향 회전 (매 FixedUpdate마다 각도 누적)
        // - m_turnScale: 초당 회전 각도 (도/초)
        // - m_turnDirection: +1(좌/반시계), -1(우/시계)
        // ─────────────────────────────────────────────
        float angleChangeDeg = m_turnScale * fixedDeltaTime * static_cast<float>(m_turnDirection);
        float angleChangeRad = angleChangeDeg * 3.14159265f / 180.0f;  // 도 → 라디안
        m_currentAngle += angleChangeRad;
        
        // 각도 정규화 (0 ~ 2π)
        const float TWO_PI = 2.0f * 3.14159265f;
        while (m_currentAngle < 0.0f) m_currentAngle += TWO_PI;
        while (m_currentAngle >= TWO_PI) m_currentAngle -= TWO_PI;
        
        // ─────────────────────────────────────────────
        // 2. 현재 각도로 방향 벡터 계산 및 이동
        // ─────────────────────────────────────────────
        engine::Vector3 direction = GetDirectionVector();
        MoveInDirection(direction, m_moveSpeed);
    }

    // ═══════════════════════════════════════════════════════════════
    // IdleMove 초기화 (완전 랜덤 방향)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::InitializeIdleMove()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        // 초기 방향: 완전 랜덤 (0 ~ 360도)
        std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159265f);
        m_currentAngle = angleDist(gen);
        
        // Roaming 파라미터 초기화
        ResetRoamingParameters();
        
        m_roamingTimer = 0.0f;
        m_collisionOccurred = false;
    }

    // ═══════════════════════════════════════════════════════════════
    // Roaming 파라미터 재설정
    // - m_roamingDuration: 범위 내 랜덤
    // - m_turnDirection: 좌/우 50% 확률
    // - m_turnScale: 범위 내 랜덤
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ResetRoamingParameters()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        // 1. m_roamingDuration 설정
        std::uniform_real_distribution<float> durationDist(m_roamingDurationMin, m_roamingDurationMax);
        m_roamingDuration = durationDist(gen);
        
        // 2. 좌/우 방향 선택 (50% 확률)
        std::uniform_int_distribution<int> dirDist(0, 1);
        m_turnDirection = (dirDist(gen) == 0) ? 1 : -1;  // +1: 좌(반시계), -1: 우(시계)
        
        // 3. m_turnScale 설정 (범위 내 랜덤)
        std::uniform_real_distribution<float> turnDist(m_turnScaleMin, m_maxTurnScale);
        m_turnScale = turnDist(gen);
        
        // 타이머 리셋
        m_roamingTimer = 0.0f;
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 시 방향 전환
    // - 좌/우 50% 선택 후 90~180도 랜덤 각도로 전환
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::ChangeDirectionOnCollision()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        // 1. 좌/우 선택 (50% 확률)
        std::uniform_int_distribution<int> dirDist(0, 1);
        bool turnLeft = (dirDist(gen) == 0);
        
        // 2. 90~180도 사이 랜덤 각도 (라디안)
        std::uniform_real_distribution<float> angleDist(90.0f, 180.0f);
        float angleChangeDeg = angleDist(gen);
        float angleChangeRad = angleChangeDeg * 3.14159265f / 180.0f;
        
        // 3. 방향 적용
        if (turnLeft)
        {
            m_currentAngle += angleChangeRad;  // 반시계 방향
        }
        else
        {
            m_currentAngle -= angleChangeRad;  // 시계 방향
        }
        
        // 각도 정규화 (0 ~ 2π)
        const float TWO_PI = 2.0f * 3.14159265f;
        while (m_currentAngle < 0.0f) m_currentAngle += TWO_PI;
        while (m_currentAngle >= TWO_PI) m_currentAngle -= TWO_PI;
    }

    // ═══════════════════════════════════════════════════════════════
    // 현재 각도 → 방향 벡터 변환
    // - 0도 = +X, 90도 = +Z (반시계 방향)
    // ═══════════════════════════════════════════════════════════════
    engine::Vector3 MonsterRoundBlue::GetDirectionVector() const
    {
        return engine::Vector3(
            std::cos(m_currentAngle),   // X
            0.0f,                        // Y (수평 이동)
            std::sin(m_currentAngle)    // Z
        );
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태 진입 콜백 오버라이드
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::OnStateEntered(const std::string& state)
    {
        // 부모 클래스 호출
        MonsterRoundType::OnStateEntered(state);
        
        if (state == "IdleMove")
        {
            InitializeIdleMove();
        }
        else if (state == "Idle")
        {
            // Idle 진입 시 (시작/부활) - 특별한 처리 없음
            // 다음 IdleMove 진입 시 InitializeIdleMove()에서 랜덤 방향 설정
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundBlue::OnGui()
    {
        ImGui::Text("=== MonsterRoundBlue ===");
        ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "Tier: Blue (Curved Movement)");
        
        // 부모 클래스 OnGui 호출
        MonsterRoundType::OnGui();
        
        // Blue 전용 설정
        ImGui::Separator();
        ImGui::Text("=== Blue Roaming Settings ===");
        ImGui::DragFloat("Roaming Duration Min", &m_roamingDurationMin, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("Roaming Duration Max", &m_roamingDurationMax, 0.1f, 0.1f, 10.0f);
        
        ImGui::Separator();
        ImGui::Text("=== Blue Turn Scale Settings ===");
        ImGui::DragFloat("Turn Scale Min (deg/s)", &m_turnScaleMin, 1.0f, 1.0f, 90.0f);
        ImGui::DragFloat("Turn Scale Max (deg/s)", &m_maxTurnScale, 1.0f, 1.0f, 180.0f);
        
        ImGui::Separator();
        ImGui::Text("=== Blue Damage Settings ===");
        ImGui::DragFloat("Damage Cooldown", &m_damageCooldown, 0.1f, 0.1f, 5.0f);
        
        // 런타임 정보
        ImGui::Separator();
        ImGui::Text("=== Blue Runtime ===");
        float angleDeg = m_currentAngle * 180.0f / 3.14159265f;
        ImGui::Text("Current Angle: %.1f deg", angleDeg);
        ImGui::Text("Turn Direction: %s", (m_turnDirection > 0) ? "Left (CCW)" : "Right (CW)");
        ImGui::Text("Turn Scale: %.1f deg/s", m_turnScale);
        ImGui::Text("Roaming Timer: %.2f / %.2f", m_roamingTimer, m_roamingDuration);
        
        engine::Vector3 dir = GetDirectionVector();
        ImGui::Text("Direction Vector: (%.2f, %.2f, %.2f)", dir.x, dir.y, dir.z);
    }

    void MonsterRoundBlue::Save(engine::json& j) const
    {
        MonsterRoundType::Save(j);
        
        // Blue 전용 데이터 저장
        j["RoamingDurationMin"] = m_roamingDurationMin;
        j["RoamingDurationMax"] = m_roamingDurationMax;
        j["TurnScaleMin"] = m_turnScaleMin;
        j["MaxTurnScale"] = m_maxTurnScale;
        j["DamageCooldown"] = m_damageCooldown;
    }

    void MonsterRoundBlue::Load(const engine::json& j)
    {
        MonsterRoundType::Load(j);
        
        // Blue 전용 데이터 로드
        m_roamingDurationMin = j.value("RoamingDurationMin", 1.0f);
        m_roamingDurationMax = j.value("RoamingDurationMax", 3.0f);
        m_turnScaleMin = j.value("TurnScaleMin", 10.0f);
        m_maxTurnScale = j.value("MaxTurnScale", 45.0f);
        m_damageCooldown = j.value("DamageCooldown", 1.0f);
    }
}
