#include "GamePCH.h"
#include "BaseControllerScript.h"

#include <Framework/Object/Component/LogicFSM.h>
#include <Framework/Object/Component/AnimFSM.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/GameObject/GameObject.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void BaseControllerScript::Awake()
    {
        CacheComponents();
    }

    void BaseControllerScript::Start()
    {
        RegisterFSMCallbacks();
    }

    void BaseControllerScript::Update()
    {
        // 1. 입력 처리 → FSM 파라미터 설정
        ProcessInput();
        
        // 2. FSM 상태 전이 처리 (DeltaTime 기반)
        if (m_logicFSM) m_logicFSM->UpdateFSM();
        if (m_animFSM)  m_animFSM->UpdateFSM();
        
        // 3. 상태 기반 게임 로직 실행 (비물리: 타이머, 애니메이션 등)
        UpdateGameLogic();
    }

    void BaseControllerScript::FixedUpdate()
    {
        // 물리 기반 로직 실행 (FixedDeltaTime 기반)
        // - 이동, 회전 등 Rigidbody 조작
        UpdatePhysicsLogic();
    }

    // ═══════════════════════════════════════════════════════════════
    // 행동 제한 함수 (하이브리드 패턴 핵심)
    // - 기본 구현: 대부분의 상태에서 허용
    // - 자식에서 오버라이드하여 상태별 제한 정의
    // ═══════════════════════════════════════════════════════════════
    bool BaseControllerScript::CanMove() const
    {
        // 기본: Stunned, Dead 상태가 아니면 이동 가능
        // 자식에서 오버라이드하여 커스터마이징
        std::string state = GetCurrentState();
        return state != "Stunned" && state != "Dead";
    }

    bool BaseControllerScript::CanAttack() const
    {
        // 기본: Stunned, Dead, Reloading 상태가 아니면 공격 가능
        std::string state = GetCurrentState();
        return state != "Stunned" && state != "Dead" && state != "Reloading";
    }

    bool BaseControllerScript::CanInteract() const
    {
        // 기본: Stunned, Dead 상태가 아니면 상호작용 가능
        std::string state = GetCurrentState();
        return state != "Stunned" && state != "Dead";
    }

    // ═══════════════════════════════════════════════════════════════
    // 현재 상태 확인 유틸리티
    // ═══════════════════════════════════════════════════════════════
    std::string BaseControllerScript::GetCurrentState() const
    {
        return m_logicFSM ? m_logicFSM->GetCurrentState() : "";
    }

    bool BaseControllerScript::IsInState(const std::string& stateName) const
    {
        return GetCurrentState() == stateName;
    }

    bool BaseControllerScript::IsInAnyState(const std::initializer_list<std::string>& states) const
    {
        std::string current = GetCurrentState();
        for (const auto& state : states)
        {
            if (current == state) return true;
        }
        return false;
    }

    // ═══════════════════════════════════════════════════════════════
    // 방향 유틸리티
    // - SkeletalAnimator가 FBX 모델을 180도 회전시킴
    // - 결과적으로 Transform Forward (+Z) = 모델의 실제 앞 방향
    // ═══════════════════════════════════════════════════════════════
    engine::Vector3 BaseControllerScript::GetForwardDirection() const
    {
        // Transform의 Forward 방향 (+Z) = 모델의 실제 앞 방향
        if (!GetTransform()) return engine::Vector3(0.0f, 0.0f, 1.0f);

        engine::Vector3 forward = GetTransform()->GetForward();
        forward.y = 0.0f;
        
        if (forward.LengthSquared() < 0.0001f)
        {
            return engine::Vector3(0.0f, 0.0f, 1.0f);
        }
        
        forward.Normalize();
        return forward;
    }

    engine::Vector3 BaseControllerScript::GetForwardDirectionReverse() const
    {
        // 뒤쪽 방향 (-Z)
        return GetForwardDirection() * -1.0f;
    }

    // ═══════════════════════════════════════════════════════════════
    // 대상(Target) 유틸리티
    // ═══════════════════════════════════════════════════════════════
    float BaseControllerScript::GetDistanceToTarget(engine::GameObject* target) const
    {
        if (!target || !GetTransform()) return FLT_MAX;

        engine::Transform* targetTransform = target->GetTransform();
        if (!targetTransform) return FLT_MAX;

        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        engine::Vector3 targetPos = targetTransform->GetWorldPosition();
        
        return (targetPos - myPos).Length();
    }

    engine::Vector3 BaseControllerScript::CalculateDirectionToTarget(engine::GameObject* target) const
    {
        if (!target || !GetTransform()) return engine::Vector3::Zero;

        engine::Transform* targetTransform = target->GetTransform();
        if (!targetTransform) return engine::Vector3::Zero;

        engine::Vector3 myPos = GetTransform()->GetWorldPosition();
        engine::Vector3 targetPos = targetTransform->GetWorldPosition();

        engine::Vector3 direction = targetPos - myPos;
        direction.y = 0.0f;
        
        if (direction.LengthSquared() < 0.0001f)
        {
            return engine::Vector3::Zero;
        }
        
        direction.Normalize();
        return direction;
    }

    bool BaseControllerScript::IsTargetInRange(engine::GameObject* target, float range) const
    {
        return GetDistanceToTarget(target) <= range;
    }

    // ═══════════════════════════════════════════════════════════════
    // 회전 유틸리티
    // ═══════════════════════════════════════════════════════════════
    void BaseControllerScript::RotateTowardsTarget(engine::GameObject* target, float deltaTime)
    {
        engine::Vector3 direction = CalculateDirectionToTarget(target);
        RotateToDirection(direction, deltaTime);
    }

    void BaseControllerScript::RotateToDirection(const engine::Vector3& targetDirection, float deltaTime)
    {
        if (!m_cachedRigidbody)
        {
            m_cachedRigidbody = GetGameObject() ? GetGameObject()->GetComponent<engine::Rigidbody>() : nullptr;
        }

        if (!m_cachedRigidbody || targetDirection.LengthSquared() < 0.0001f)
        {
            if (m_cachedRigidbody) m_cachedRigidbody->SetAngularVelocity(engine::Vector3::Zero);
            return;
        }

        // 현재 Forward 방향 (+Z)
        // SkeletalAnimator가 모델을 180도 회전시켜서 +Z가 실제 앞 방향
        engine::Vector3 currentForward = GetForwardDirection();

        engine::Vector3 targetDir = targetDirection;
        
        targetDir.y = 0.0f;        
        targetDir.Normalize();

        float dot = currentForward.Dot(targetDir);
        
        // Threshold 체크: 각도 차이가 작으면 회전 멈춤
        const float ROTATION_THRESHOLD = 1.0f * 3.14159f / 180.0f;
        const float dotThreshold = cosf(ROTATION_THRESHOLD);

        if (dot >= dotThreshold)
        {
            StopRotation();
            return;
        }

        engine::Vector3 cross = currentForward.Cross(targetDir); 
        float rotationSign = (cross.y >= 0.0f) ? -1.0f : 1.0f;

        float angleDiff = acosf(std::clamp(dot, -1.0f, 1.0f));
        
        const float rotationSpeed = 3.5f;  // 2.0f → 3.5f (75% 증가)
        float proportional = rotationSign * angleDiff * rotationSpeed;
        
        engine::Vector3 currentAngVel = m_cachedRigidbody->GetAngularVelocity();
        float derivative = -currentAngVel.y * 0.5f;
        
        float targetAngularVelocity = proportional + derivative;
        
        const float maxAngularSpeed = 8.0f;  // 5.0f → 8.0f (60% 증가)
        targetAngularVelocity = std::clamp(targetAngularVelocity, -maxAngularSpeed, maxAngularSpeed);

        m_cachedRigidbody->SetAngularVelocity(engine::Vector3(0.0f, targetAngularVelocity, 0.0f));
    }

    bool BaseControllerScript::IsLookingAtTarget(engine::GameObject* target) const
    {
        if (!target) return false;
        
        engine::Vector3 direction = CalculateDirectionToTarget(target);
        return IsLookingAtDirection(direction);
    }

    bool BaseControllerScript::IsLookingAtDirection(const engine::Vector3& targetDirection) const
    {
        if (targetDirection.LengthSquared() < 0.0001f) return true;

        // 현재 Forward 방향 (+Z)
        // SkeletalAnimator가 모델을 180도 회전시켜서 +Z가 실제 앞 방향
        engine::Vector3 currentForward = GetForwardDirection();

        engine::Vector3 targetDir = targetDirection;
        targetDir.y = 0.0f;
        targetDir.Normalize();

        float dot = currentForward.Dot(targetDir);
        // RotateToDirection과 동일한 threshold 사용
        const float ROTATION_THRESHOLD = 15.0f * 3.14159f / 180.0f;
        const float dotThreshold = cosf(ROTATION_THRESHOLD);
        
        return dot >= dotThreshold;
    }

    void BaseControllerScript::StopRotation()
    {
        if (!m_cachedRigidbody)
        {
            m_cachedRigidbody = GetGameObject() ? GetGameObject()->GetComponent<engine::Rigidbody>() : nullptr;
        }

        if (m_cachedRigidbody)
        {
            m_cachedRigidbody->SetAngularVelocity(engine::Vector3::Zero);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 컴포넌트 캐싱
    // ═══════════════════════════════════════════════════════════════
    void BaseControllerScript::CacheComponents()
    {
        CacheFSMComponents();
    }

    void BaseControllerScript::CacheFSMComponents()
    {
        if (!GetGameObject()) return;
        
        m_logicFSM = GetGameObject()->GetComponent<engine::LogicFSM>();
        m_animFSM = GetGameObject()->GetComponent<engine::AnimFSM>();
    }

    // ═══════════════════════════════════════════════════════════════
    // FSM 콜백 등록
    // ═══════════════════════════════════════════════════════════════
    void BaseControllerScript::RegisterFSMCallbacks()
    {
        if (!m_logicFSM) return;
        
        m_logicFSM->RegisterStateChangeCallback([this](const std::string& oldState, const std::string& newState)
        {
            OnStateChanged(oldState, newState);
        });
        
        m_logicFSM->RegisterStateEnterCallback([this](const std::string& state)
        {
            OnStateEntered(state);
        });
        
        m_logicFSM->RegisterStateExitCallback([this](const std::string& state)
        {
            OnStateExited(state);
        });
    }

    // ═══════════════════════════════════════════════════════════════
    // FSM 초기화 헬퍼
    // ═══════════════════════════════════════════════════════════════
    void BaseControllerScript::AddFSMState(const std::string& stateName, bool isDefault)
    {
        if (!m_logicFSM) return;

        engine::FSMState state;
        state.name = stateName;
        state.isDefault = isDefault;
        m_logicFSM->AddState(state);
    }

    void BaseControllerScript::AddFSMTransition(
        const std::string& fromState,
        const std::string& toState,
        const std::string& parameterName,
        FSMTransitionType conditionType)
    {
        if (!m_logicFSM) return;

        engine::FSMTransition transition;
        transition.fromState = fromState;
        transition.toState = toState;
        transition.conditionParameter = parameterName;
        transition.conditionType = conditionType;
        
        m_logicFSM->AddTransition(fromState, transition);
    }
}
