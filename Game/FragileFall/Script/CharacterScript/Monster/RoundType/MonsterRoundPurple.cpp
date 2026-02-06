#include "GamePCH.h"
#include "MonsterRoundPurple.h"

#include "Script/CharacterScript/Player/PlayerControllerScript.h"

#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Object/Component/Renderer/SkeletalMeshRenderer.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Physics/PhysicsSystem.h>
#include <Framework/Physics/CollisionTypes.h>
#include <Framework/Asset/Prefab.h>

namespace game
{
    // 스케일 프리셋 정의
    constexpr float MonsterRoundPurple::kScalePresets[4];
    
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundPurple::Awake()
    {
        MonsterRoundType::Awake();
        
        // Purple 등급 고정
        m_monsterTier = MonsterTier::Purple;
    }

    void MonsterRoundPurple::Start()
    {
        MonsterRoundType::Start();

        // Purple 등급 색상 설정 (보라색)
        if (auto* meshRenderer = GetGameObject()->GetComponent<engine::SkeletalMeshRenderer>())
        {
            meshRenderer->SetBaseColor(engine::Vector4(0.6f, 0.2f, 0.8f, 1.0f));
        }
        
        // SplittingEnemy 레이어로 설정 (자기 자신끼리 충돌 안 함)
        if (auto* collider = GetGameObject()->GetComponent<engine::Collider>())
        {
            m_originalLayer = collider->GetLayer();
            collider->SetLayer(engine::PhysicsLayer::Index::SplittingEnemy);
        }
        
        // 방향 벡터 정규화
        m_moveDirectionVector.Normalize();
        
        // 분열 횟수에 따른 스케일 적용
        if (m_splitCount >= 0 && m_splitCount <= kMaxSplitCount)
        {
            float scale = kScalePresets[m_splitCount];
            GetTransform()->SetLocalScale(engine::Vector3(scale, scale, scale));
        }
        
        // 마지막 데미지 시간 초기화
        m_lastDamageTime = engine::Time::GetTimestamp();
    }
    
    // ═══════════════════════════════════════════════════════════════
    // FSM 초기화 (Purple 전용 - 간단한 FSM)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundPurple::InitializeFSM()
    {
        if (!m_logicFSM) return;

        // ─────────────────────────────────────────────
        // 상태 정의 (Fragile 없음, 공격 상태 없음)
        // ─────────────────────────────────────────────
        AddFSMState("Idle", true);              // 기본 상태
        AddFSMState("EngageMove", false);       // 이동 중
        AddFSMState("Dead", false);

        // ─────────────────────────────────────────────
        // StateMap 업데이트
        // ─────────────────────────────────────────────
        m_logicFSM->Initialize();

        // ─────────────────────────────────────────────
        // 파라미터 정의
        // ─────────────────────────────────────────────
        m_logicFSM->SetParameter("IdleTimerComplete", false);
        m_logicFSM->SetParameter("Die", m_isDead);

        // ─────────────────────────────────────────────
        // 전이 정의
        // ─────────────────────────────────────────────
        
        // Idle → EngageMove (대기 완료)
        AddFSMTransition("Idle", "EngageMove", "IdleTimerComplete", BoolTrue());
        
        // Any → Dead
        AddFSMTransition("Idle", "Dead", "Die", Trigger());
        AddFSMTransition("EngageMove", "Dead", "Die", Trigger());

        // ─────────────────────────────────────────────
        // 초기 상태 설정
        // ─────────────────────────────────────────────
        m_logicFSM->InitializeCurrentState();
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백 (반사 + 플레이어 데미지)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundPurple::OnCollisionEnter(const engine::CollisionInfo& info)
    {
        if (!info.collider) return;
        
        uint32_t otherLayer = info.collider->GetLayer();
        
        // ─────────────────────────────────────────────
        // 플레이어와 충돌 → 데미지
        // ─────────────────────────────────────────────
        if (otherLayer == engine::PhysicsLayer::Index::Player)
        {
            // 쿨다운 체크
            float elapsedSinceLastDamage = engine::Time::GetElapsedSeconds(m_lastDamageTime);
            if (elapsedSinceLastDamage >= m_damageCooldown)
            {
                // 플레이어에게 데미지
                if (info.gameObject)
                {
                    if (auto* playerScript = info.gameObject->GetComponent<PlayerControllerScript>())
                    {
                        playerScript->TakeDamage(m_attackDamage);
                        m_lastDamageTime = engine::Time::GetTimestamp();
                    }
                }
            }
        }
        
        // ─────────────────────────────────────────────
        // 벽/환경과 충돌 → 반사
        // ─────────────────────────────────────────────
        if (otherLayer == engine::PhysicsLayer::Index::Environment ||
            otherLayer == engine::PhysicsLayer::Index::Wall ||
            otherLayer == engine::PhysicsLayer::Index::Enemy)
        {
            // 현재 상태 확인
            std::string currentState = GetCurrentState();
            if (currentState != "EngageMove")
            {
                return;  // 이동 중이 아니면 반사 안 함
            }
            
            // 접촉점에서 노말 가져오기
            if (info.contacts.empty()) return;
            
            engine::Vector3 contactNormal = info.contacts[0].normal;
            
            // 충돌 노말 스냅 (X축 또는 Z축)
            engine::Vector3 snappedNormal = SnapNormalToAxis(contactNormal);
            
            // 정반사
            engine::Vector3 reflectedDir = ReflectDirection(m_moveDirectionVector, snappedNormal);
            
            // 방향 업데이트
            m_moveDirectionVector = reflectedDir;
            m_moveDirectionVector.Normalize();
            
            // 열거형 동기화
            UpdateDiagonalDirectionFromVector();
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 입력 처리
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundPurple::ProcessInput()
    {
        if (!m_logicFSM) return;

        // 플레이어 찾기
        if (!m_targetPlayer)
        {
            FindPlayer();
        }

        // Purple은 공격 기능 없음, 사거리 체크 불필요
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 상태 진입 콜백
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundPurple::OnStateEntered(const std::string& state)
    {
        MonsterRoundType::OnStateEntered(state);
        
        if (state == "Idle")
        {
            // 분열 후 Idle 대기 스킵
            if (m_skipIdleWait && m_logicFSM)
            {
                m_logicFSM->SetParameter("IdleTimerComplete", true);
                m_skipIdleWait = false;  // 플래그 리셋 (1회성)
            }
        }
        else if (state == "EngageMove")
        {
            // 랜덤 대각선 방향 설정 (처음 시작 시, 분열 시에는 이미 설정됨)
            if (!m_skipIdleWait)
            {
                SetRandomDiagonalDirection();
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 체력 관리 (분열 로직)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundPurple::CheckHealth()
    {
        std::string currentState = GetCurrentState();

        if (m_Hp <= 0 && currentState != "Dead")
        {
            if (m_splitCount < kMaxSplitCount)
            {
                // 분열 가능 → 분열 실행
                PerformSplit();
                
                // 원본은 즉시 파괴 (Dead 상태 없이)
                GetGameObject()->Destroy();
            }
            else
            {
                // 분열 불가 → 일반 사망
                TriggerDeath();
            }
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 분열 로직
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundPurple::PerformSplit()
    {
        // 현재 위치
        engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
        
        // 새로운 분열 횟수
        int newSplitCount = m_splitCount + 1;
        
        // 새로운 HP (현재 MaxHP의 절반)
        float newMaxHp = m_maxHp * 0.5f;
        float newHp = newMaxHp;
        
        // 서로 다른 대각선 방향 2개 선택
        DiagonalDirection dir1 = m_currentDiagonalDirection;
        DiagonalDirection dir2 = GetOppositeDirection(dir1);
        
        // ─────────────────────────────────────────────
        // 첫 번째 분열체 생성
        // ─────────────────────────────────────────────
        engine::GameObject* split1 = engine::Prefab::Instantiate(m_prefabName);
        if (split1)
        {
            split1->GetTransform()->SetLocalPosition(currentPos);
            
            if (auto* purpleScript = split1->GetComponent<MonsterRoundPurple>())
            {
                purpleScript->SetSplitCount(newSplitCount);
                purpleScript->m_maxHp = newMaxHp;
                purpleScript->m_Hp = newHp;
                purpleScript->SetDiagonalDirection(dir1);
                purpleScript->m_skipIdleWait = true;  // Idle 대기 스킵
            }
        }
        
        // ─────────────────────────────────────────────
        // 두 번째 분열체 생성
        // ─────────────────────────────────────────────
        engine::GameObject* split2 = engine::Prefab::Instantiate(m_prefabName);
        if (split2)
        {
            split2->GetTransform()->SetLocalPosition(currentPos);
            
            if (auto* purpleScript = split2->GetComponent<MonsterRoundPurple>())
            {
                purpleScript->SetSplitCount(newSplitCount);
                purpleScript->m_maxHp = newMaxHp;
                purpleScript->m_Hp = newHp;
                purpleScript->SetDiagonalDirection(dir2);
                purpleScript->m_skipIdleWait = true;  // Idle 대기 스킵
            }
        }
        
        LOG_INFO("[MonsterRoundPurple] Split performed! Count: {} -> {}, HP: {:.1f} -> {:.1f}", 
            m_splitCount, newSplitCount, m_maxHp, newMaxHp);
    }
    
    MonsterRoundPurple::DiagonalDirection MonsterRoundPurple::GetOppositeDirection(DiagonalDirection dir) const
    {
        switch (dir)
        {
        case DiagonalDirection::PlusXPlusZ:
            return DiagonalDirection::MinusXMinusZ;
        case DiagonalDirection::PlusXMinusZ:
            return DiagonalDirection::MinusXPlusZ;
        case DiagonalDirection::MinusXPlusZ:
            return DiagonalDirection::PlusXMinusZ;
        case DiagonalDirection::MinusXMinusZ:
            return DiagonalDirection::PlusXPlusZ;
        default:
            return DiagonalDirection::PlusXPlusZ;
        }
    }
    
    void MonsterRoundPurple::SetDiagonalDirection(DiagonalDirection dir)
    {
        m_currentDiagonalDirection = dir;
        m_moveDirectionVector = GetDiagonalDirectionVector(dir);
        m_moveDirectionVector.Normalize();
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 총알 초기화 (Purple은 총알 없음)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundPurple::InitializeBullet()
    {
        // 총알 없음 - 빈 구현
    }

    // ═══════════════════════════════════════════════════════════════
    // 공격 (Purple은 접촉 데미지만)
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundPurple::Attack(float deltaTime)
    {
        // 접촉 데미지는 OnCollisionEnter에서 처리
        // 총알 발사 없음
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 상태별 물리 행동
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundPurple::ExecuteEngageMoveBehaviorPhysics()
    {
        ExecuteDiagonalMovement();
    }
    
    void MonsterRoundPurple::ExecuteDiagonalMovement()
    {
        if (!m_rigidbody) return;
        
        // 대각선 등속 이동
        engine::Vector3 velocity = m_moveDirectionVector * m_moveSpeed;
        velocity.y = 0.0f;  // Y축 이동 없음
        
        m_rigidbody->SetLinearVelocity(velocity);
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 행동 제한
    // ═══════════════════════════════════════════════════════════════
    bool MonsterRoundPurple::CanMove() const
    {
        std::string state = GetCurrentState();
        return state == "EngageMove";
    }
    
    bool MonsterRoundPurple::CanAttack() const
    {
        // 총알 공격 없음
        return false;
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 대각선 이동 헬퍼 함수
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundPurple::SetRandomDiagonalDirection()
    {
        // 4개 대각선 방향 중 랜덤 선택
        int randomIndex = std::rand() % 4;
        m_currentDiagonalDirection = static_cast<DiagonalDirection>(randomIndex);
        m_moveDirectionVector = GetDiagonalDirectionVector(m_currentDiagonalDirection);
        m_moveDirectionVector.Normalize();
    }
    
    engine::Vector3 MonsterRoundPurple::GetDiagonalDirectionVector(DiagonalDirection dir) const
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
    
    void MonsterRoundPurple::UpdateDiagonalDirectionFromVector()
    {
        // X, Z 부호로 방향 결정
        bool positiveX = m_moveDirectionVector.x >= 0.0f;
        bool positiveZ = m_moveDirectionVector.z >= 0.0f;
        
        if (positiveX && positiveZ)
        {
            m_currentDiagonalDirection = DiagonalDirection::PlusXPlusZ;
        }
        else if (positiveX && !positiveZ)
        {
            m_currentDiagonalDirection = DiagonalDirection::PlusXMinusZ;
        }
        else if (!positiveX && positiveZ)
        {
            m_currentDiagonalDirection = DiagonalDirection::MinusXPlusZ;
        }
        else
        {
            m_currentDiagonalDirection = DiagonalDirection::MinusXMinusZ;
        }
    }
    
    // ═══════════════════════════════════════════════════════════════
    // 반사 계산
    // ═══════════════════════════════════════════════════════════════
    engine::Vector3 MonsterRoundPurple::SnapNormalToAxis(const engine::Vector3& normal) const
    {
        // X축 또는 Z축 중 더 큰 쪽으로 스냅
        float absX = std::abs(normal.x);
        float absZ = std::abs(normal.z);
        
        if (absX >= absZ)
        {
            // X축 스냅
            return engine::Vector3(normal.x >= 0.0f ? 1.0f : -1.0f, 0.0f, 0.0f);
        }
        else
        {
            // Z축 스냅
            return engine::Vector3(0.0f, 0.0f, normal.z >= 0.0f ? 1.0f : -1.0f);
        }
    }
    
    engine::Vector3 MonsterRoundPurple::ReflectDirection(const engine::Vector3& direction, const engine::Vector3& normal) const
    {
        // 정반사 공식: R = D - 2(D·N)N
        float dot = direction.x * normal.x + direction.y * normal.y + direction.z * normal.z;
        
        engine::Vector3 reflected;
        reflected.x = direction.x - 2.0f * dot * normal.x;
        reflected.y = direction.y - 2.0f * dot * normal.y;
        reflected.z = direction.z - 2.0f * dot * normal.z;
        
        return reflected;
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void MonsterRoundPurple::OnGui()
    {
        ImGui::Text("=== MonsterRoundPurple ===");
        ImGui::TextColored(ImVec4(0.6f, 0.2f, 0.8f, 1.0f), "Tier: Purple");
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "[No Bullet Attack - Contact Damage + Split]");
        
        // 부모 클래스 OnGui 호출
        MonsterRoundType::OnGui();
        
        // Purple 전용 설정
        ImGui::Separator();
        ImGui::Text("=== Split Settings ===");
        ImGui::Text("Split Count: %d / %d", m_splitCount, kMaxSplitCount);
        ImGui::Text("Current Scale: %.2f", kScalePresets[m_splitCount]);
        
        if (m_splitCount < kMaxSplitCount)
        {
            ImGui::Text("Next Split HP: %.1f", m_maxHp * 0.5f);
            ImGui::Text("Next Split Scale: %.2f", kScalePresets[m_splitCount + 1]);
        }
        else
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Max split reached - will die on HP 0");
        }
        
        ImGui::Separator();
        ImGui::Text("=== Contact Damage ===");
        ImGui::DragFloat("Damage Cooldown", &m_damageCooldown, 0.1f, 0.0f, 5.0f, "%.1f sec");
        
        ImGui::Separator();
        ImGui::Text("=== Movement ===");
        const char* dirNames[] = { "+X+Z", "+X-Z", "-X+Z", "-X-Z" };
        int dirIndex = static_cast<int>(m_currentDiagonalDirection);
        ImGui::Text("Direction: %s", dirNames[dirIndex]);
        ImGui::Text("Direction Vector: (%.2f, %.2f, %.2f)", 
            m_moveDirectionVector.x, m_moveDirectionVector.y, m_moveDirectionVector.z);
    }

    void MonsterRoundPurple::Save(engine::json& j) const
    {
        MonsterRoundType::Save(j);
        
        // Purple 전용 데이터 저장
        j["SplitCount"] = m_splitCount;
        j["DamageCooldown"] = m_damageCooldown;
        j["PrefabName"] = m_prefabName;
    }

    void MonsterRoundPurple::Load(const engine::json& j)
    {
        MonsterRoundType::Load(j);
        
        // Purple 전용 데이터 로드
        m_splitCount = j.value("SplitCount", 0);
        m_damageCooldown = j.value("DamageCooldown", 1.0f);
        m_prefabName = j.value("PrefabName", std::string("Monster_RoundType_Purple"));
    }
}
