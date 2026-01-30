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
    void BaseControllerScript::RotateTowardsTarget(engine::GameObject* target, float rotationSpeed)
    {
        engine::Vector3 direction = CalculateDirectionToTarget(target);
        RotateToDirection(direction, rotationSpeed);
    }

    void BaseControllerScript::RotateToDirection(const engine::Vector3& targetDirection, float rotationSpeed)
    {
        // ═══════════════════════════════════════════════════════════════
        // 목표 방향으로 Y축 회전 (Z+가 전방)
        // - CalcLogicalRotateAngle()에서 계산된 멤버변수 활용
        // - Kinematic: Transform 직접 회전
        // - Dynamic: ForceSetRotation으로 직접 회전 (Kinematic처럼 동작)
        // ═══════════════════════════════════════════════════════════════
        
        // CalcLogicalRotateAngle에서 회전이 불필요하다고 판단했으면 리턴
        if (!m_isToRotate)
        {
            return;
        }

        // 유효하지 않은 방향이면 리턴
        if (targetDirection.LengthSquared() < 0.0001f)
        {
            return;
        }

        // 목표 방향 정규화 (XZ 평면)
        engine::Vector3 targetDir = targetDirection;
        targetDir.y = 0.0f;
        targetDir.Normalize();

        // 현재 Forward 방향 (Z+가 전방)
        engine::Vector3 currentForward = m_objectLogicalFoward;

        // 남은 각도 계산 (오버슈트 방지용 - 매 프레임 계산 필요)
        float dot = currentForward.Dot(targetDir);
        dot = std::clamp(dot, -1.0f, 1.0f);
        float angleDiff = acosf(dot);
        
        // 거의 같은 방향이면 회전 완료
        const float THRESHOLD_ANGLE = 0.017f;  // 약 1도 (라디안)
        if (angleDiff < THRESHOLD_ANGLE)
        {
            return;
        }

        // 프레임당 회전량 계산
        float deltaTime = engine::Time::DeltaTime();
        float rotationStep = rotationSpeed * deltaTime;
        
        // 오버슈트 방지: 남은 각도보다 더 돌지 않음
        if (rotationStep > angleDiff)
        {
            rotationStep = angleDiff;
        }
        
        // 최종 회전량 (CalcLogicalRotateAngle에서 계산된 m_rotateSign 사용)
        // 부호 반전: ToEuler/CreateFromYawPitchRoll과 외적 기반 rotateSign의 좌표계 차이 보정
        float finalRotation = m_rotateSign * rotationStep;

        // 현재 회전에서 Y축 회전만 변경
        engine::Quaternion currentRot = GetTransform()->GetWorldRotation();
        engine::Vector3 euler = currentRot.ToEuler();
        euler.y += finalRotation;
        
        engine::Quaternion newRot = engine::Quaternion::CreateFromYawPitchRoll(euler.y, euler.x, euler.z);

        // ═══════════════════════════════════════════════════════════════
        // Rigidbody 유무 및 타입에 따라 회전 적용
        // ═══════════════════════════════════════════════════════════════
        if (!m_cachedRigidbody)
        {
            m_cachedRigidbody = GetGameObject() ? GetGameObject()->GetComponent<engine::Rigidbody>() : nullptr;
        }

        if (m_cachedRigidbody && m_cachedRigidbody->IsDynamic())
        {
            // Dynamic Rigidbody: ForceSetTransform 사용 (위치 유지, 회전만 변경)
            engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
            m_cachedRigidbody->ForceSetTransform(currentPos, newRot, false, true);  // 각속도만 리셋
        }
        else
        {
            // Kinematic 또는 Rigidbody 없음: Transform 직접 설정
            GetTransform()->SetLocalRotation(newRot);
        }
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

    void BaseControllerScript::UpdateLogicalPosition()
    {
        engine::Vector3 pos = GetTransform()->GetWorldPosition();        
        pos.y = 0.0f;         
        m_objectLogicalPosition = pos;         
    }



    void BaseControllerScript::UpdateLogicalMoveExistence(engine::Vector3& inputMoveDir)
    {        
        if (inputMoveDir.LengthSquared() < 0.0001f)
        {
            m_currentLogicalMoveVector = engine::Vector3::Zero;
            return;
        }

        engine::Vector3 tVec = inputMoveDir;
        tVec.y = 0.0f;
        tVec.Normalize();
       
        m_currentLogicalMoveVector = tVec;      
    }

    void BaseControllerScript::CalcLogicalRotateAngle(engine::Vector3& target)
    {
        engine::Vector3 newVec = target;

        engine::Vector3 mFoward = m_objectLogicalFoward;

        mFoward.Normalize();

        float dot = mFoward.Dot(target);

        if (dot > 0.9f)
        {
            m_angleToRotateLogical = engine::Vector3::Zero;
            m_rotateSign = 0.0f;
            m_isToRotate = false;
            return;
        }
        
        m_angleToRotateLogical = newVec;

        engine::Vector3 t = mFoward.Cross(newVec);
        float ty = t.y;

        m_rotateSign =  ty > 0.00f ? 1.0f : -1.0f;

        m_isToRotate = true;
    }

    void BaseControllerScript::HandleMovement(float maxSpeed)
    {
        // 이동 입력이 없으면 감속
        if (!m_isMoving)
        {
            ApplyBrakingForce();
            return;
        }

        // m_currentLogicalMoveVector는 ProcessInput에서 이미 계산된 정규화된 이동 방향
        // ApplyMovementForce로 Rigidbody 기반 이동 (충돌 응답 보존)
        ApplyMovementForce(m_currentLogicalMoveVector, maxSpeed);
    }


    //void BaseControllerScript::MoveStopProtocol()
    //{
    //    m_lowerBodyLogicalForward 
    //    m_currentLogicalMoveDir
    //}


    void BaseControllerScript::StopRotation()
    {
        if (!m_cachedRigidbody)
        {
            m_cachedRigidbody = GetGameObject() ? GetGameObject()->GetComponent<engine::Rigidbody>() : nullptr;
        }

        // Kinematic은 각속도가 없으므로 Dynamic만 처리
        if (m_cachedRigidbody && m_cachedRigidbody->IsDynamic())
        {
            m_cachedRigidbody->SetAngularVelocity(engine::Vector3::Zero);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 이동 유틸리티 (AddForce 기반)
    // - SetLinearVelocity 대신 AddForce 사용으로 충돌 응답 보존
    // ═══════════════════════════════════════════════════════════════
    
    void BaseControllerScript::ApplyMovementForce(const engine::Vector3& direction, float maxSpeed)
    {
        if (!m_cachedRigidbody)
        {
            m_cachedRigidbody = GetGameObject() ? GetGameObject()->GetComponent<engine::Rigidbody>() : nullptr;
        }
        
        if (!m_cachedRigidbody || !m_cachedRigidbody->IsDynamic())
        {
            return;
        }
        
        // 현재 수평 속도 (Y축 무시)
        engine::Vector3 currentVel = m_cachedRigidbody->GetLinearVelocity();
        engine::Vector3 horizontalVel(currentVel.x, 0.0f, currentVel.z);
        
        // 목표 속도
        engine::Vector3 targetVel = direction * maxSpeed;
        
        // 속도 차이 기반 가속
        engine::Vector3 velDiff = targetVel - horizontalVel;
        
        // Acceleration 모드로 힘 적용 (질량 무시)
        // 높은 가속도로 빠르게 목표 속도에 도달
        m_cachedRigidbody->AddForce(velDiff * m_movementAcceleration, engine::ForceMode::Acceleration);
        
        // 최대 속도 초과 시 부드러운 브레이크
        ClampToMaxSpeed(maxSpeed);
    }
    
    void BaseControllerScript::ApplyBrakingForce()
    {
        if (!m_cachedRigidbody)
        {
            m_cachedRigidbody = GetGameObject() ? GetGameObject()->GetComponent<engine::Rigidbody>() : nullptr;
        }
        
        if (!m_cachedRigidbody || !m_cachedRigidbody->IsDynamic())
        {
            return;
        }
        
        // 현재 수평 속도 (Y축 무시)
        engine::Vector3 currentVel = m_cachedRigidbody->GetLinearVelocity();
        engine::Vector3 horizontalVel(currentVel.x, 0.0f, currentVel.z);
        
        if (horizontalVel.LengthSquared() < 0.01f)
        {
            // 이미 거의 정지 상태면 완전 정지
            m_cachedRigidbody->SetLinearVelocity(engine::Vector3(0.0f, currentVel.y, 0.0f));
            return;
        }
        
        // 현재 속도의 반대 방향으로 감속 힘 적용
        engine::Vector3 brakeForce = -horizontalVel * m_movementDeceleration;
        m_cachedRigidbody->AddForce(brakeForce, engine::ForceMode::Acceleration);
    }
    
    void BaseControllerScript::ClampToMaxSpeed(float maxSpeed)
    {
        if (!m_cachedRigidbody)
        {
            return;
        }
        
        engine::Vector3 currentVel = m_cachedRigidbody->GetLinearVelocity();
        float horizontalSpeedSq = currentVel.x * currentVel.x + currentVel.z * currentVel.z;
        float maxSpeedSq = maxSpeed * maxSpeed;
        
        // 최대 속도의 110%를 초과하면 부드럽게 브레이크
        if (horizontalSpeedSq > maxSpeedSq * 1.21f)  // 1.1^2 = 1.21
        {
            float horizontalSpeed = sqrtf(horizontalSpeedSq);
            float overSpeed = horizontalSpeed - maxSpeed;
            
            // 초과 속도에 비례한 브레이크 힘
            engine::Vector3 brakeDir(-currentVel.x, 0.0f, -currentVel.z);
            brakeDir.Normalize();
            
            m_cachedRigidbody->AddForce(brakeDir * overSpeed * m_maxSpeedBrakeFactor, engine::ForceMode::Acceleration);
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

    // ═══════════════════════════════════════════════════════════════
    // 직렬화
    // ═══════════════════════════════════════════════════════════════
    
    void BaseControllerScript::OnGui()
    {
        ImGui::Separator();
        ImGui::Text("=== Movement Physics (Base) ===");
        ImGui::DragFloat("Movement Accel", &m_movementAcceleration, 1.0f, 1.0f, 200.0f);
        ImGui::DragFloat("Movement Decel", &m_movementDeceleration, 1.0f, 1.0f, 200.0f);
        ImGui::DragFloat("Max Speed Brake", &m_maxSpeedBrakeFactor, 0.5f, 1.0f, 50.0f);
    }
    
    void BaseControllerScript::Save(engine::json& j) const
    {
        // 부모 클래스의 Save 호출 (Object::Save → Type, Active 저장)
        engine::Object::Save(j);
        
        // BaseControllerScript 고유 데이터
        j["MovementAcceleration"] = m_movementAcceleration;
        j["MovementDeceleration"] = m_movementDeceleration;
        j["MaxSpeedBrakeFactor"] = m_maxSpeedBrakeFactor;
    }
    
    void BaseControllerScript::Load(const engine::json& j)
    {
        // 부모 클래스의 Load 호출 (Object::Load → Active 로드)
        engine::Object::Load(j);
        
        // BaseControllerScript 고유 데이터 (선택적 로드 - 기존 씬 파일 호환)
        if (j.contains("MovementAcceleration"))
            m_movementAcceleration = j["MovementAcceleration"].get<float>();
        if (j.contains("MovementDeceleration"))
            m_movementDeceleration = j["MovementDeceleration"].get<float>();
        if (j.contains("MaxSpeedBrakeFactor"))
            m_maxSpeedBrakeFactor = j["MaxSpeedBrakeFactor"].get<float>();
    }
}
