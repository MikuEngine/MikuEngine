#include "GamePCH.h"
#include "CharacterLogicFSM.h"
#include "CharacterAnimationFSM.h"
#include "InputBinding.h"

#include <Framework/Object/GameObject/GameObject.h>

namespace game
{
    const char* CharacterStateToString(CharacterState state)
    {
        switch (state)
        {
        case CharacterState::Idle:   return "Idle";
        case CharacterState::Walk:   return "Walk";
        case CharacterState::Attack: return "Attack";

        // === 확장용 상태 (예시) ===
        // case CharacterState::Run:    return "Run";
        // case CharacterState::Jump:   return "Jump";
        // case CharacterState::Fall:   return "Fall";
        // case CharacterState::Hit:    return "Hit";
        // case CharacterState::Dead:   return "Dead";
        default:                     return "Unknown";
        }
    }

    void CharacterLogicFSM::Awake()
    {
        CacheComponents();
    }

    void CharacterLogicFSM::Start()
    {
        // 초기 상태 진입
        m_context.currentState = m_currentState;
        OnEnterIdle();
        NotifyListenersEnter();
    }

    void CharacterLogicFSM::Update()
    {
        m_stateTimer += engine::Time::DeltaTime();
        
        // 입력 처리
        ProcessInput();
        
        // 현재 상태 업데이트
        UpdateState();
        
        // 리스너에게 업데이트 알림
        NotifyListenersUpdate();
    }

    void CharacterLogicFSM::ChangeState(CharacterState newState)
    {
        if (m_currentState == newState)
        {
            return;
        }

        // Exit 콜백
        m_context.previousState = m_currentState;
        NotifyListenersExit();
        
        switch (m_currentState)
        {
        case CharacterState::Idle:   OnExitIdle();   break;
        case CharacterState::Walk:   OnExitWalk();   break;
        case CharacterState::Attack: OnExitAttack(); break;
        // === 확장용 상태 (주석 해제하여 사용) ===
        // case CharacterState::Run:    OnExitRun();    break;
        // case CharacterState::Jump:   OnExitJump();   break;
        // case CharacterState::Fall:   OnExitFall();   break;
        // case CharacterState::Hit:    OnExitHit();    break;
        // case CharacterState::Dead:   OnExitDead();   break;
        default: break;
        }

        // 상태 변경
        m_currentState = newState;
        m_context.currentState = newState;
        m_stateTimer = 0.0f;

        // Enter 콜백
        switch (m_currentState)
        {
        case CharacterState::Idle:   OnEnterIdle();   break;
        case CharacterState::Walk:   OnEnterWalk();   break;
        case CharacterState::Attack: OnEnterAttack(); break;
        // === 확장용 상태 (주석 해제하여 사용) ===
        // case CharacterState::Run:    OnEnterRun();    break;
        // case CharacterState::Jump:   OnEnterJump();   break;
        // case CharacterState::Fall:   OnEnterFall();   break;
        // case CharacterState::Hit:    OnEnterHit();    break;
        // case CharacterState::Dead:   OnEnterDead();   break;
        default: break;
        }
        
        NotifyListenersEnter();
    }

    void CharacterLogicFSM::AddListener(ILogicFSMListener* listener)
    {
        if (listener && std::find(m_listeners.begin(), m_listeners.end(), listener) == m_listeners.end())
        {
            m_listeners.push_back(listener);
        }
    }

    void CharacterLogicFSM::RemoveListener(ILogicFSMListener* listener)
    {
        auto it = std::find(m_listeners.begin(), m_listeners.end(), listener);
        if (it != m_listeners.end())
        {
            m_listeners.erase(it);
        }
    }

    void CharacterLogicFSM::OnAnimationFinished(CharacterState finishedState)
    {
        // 공격 애니메이션 종료 시 Idle로 복귀
        if (finishedState == CharacterState::Attack && m_currentState == CharacterState::Attack)
        {
            ChangeState(CharacterState::Idle);
        }
        // === 확장용 (주석 해제하여 사용) ===
        // if (finishedState == CharacterState::Hit && m_currentState == CharacterState::Hit)
        // {
        //     ChangeState(CharacterState::Idle);
        // }
    }

    // === 확장용 외부 이벤트 (주석 해제하여 사용) ===
    /*
    void CharacterLogicFSM::OnHit(const engine::Vector3& hitDirection, float damage)
    {
        if (m_currentState == CharacterState::Dead)
        {
            return;
        }
        m_context.hitDirection = hitDirection;
        m_context.hitDamage = damage;
        ChangeState(CharacterState::Hit);
    }

    void CharacterLogicFSM::OnDeath()
    {
        ChangeState(CharacterState::Dead);
    }

    void CharacterLogicFSM::OnGrounded(bool isGrounded)
    {
        bool wasGrounded = m_context.isGrounded;
        m_context.isGrounded = isGrounded;

        if (!wasGrounded && isGrounded)
        {
            if (m_currentState == CharacterState::Fall || m_currentState == CharacterState::Jump)
            {
                ChangeState(CharacterState::Idle);
            }
        }
        else if (wasGrounded && !isGrounded)
        {
            if (m_currentState != CharacterState::Jump && m_currentState != CharacterState::Attack)
            {
                ChangeState(CharacterState::Fall);
            }
        }
    }

    void CharacterLogicFSM::OnAnimationEvent(const std::string& eventName)
    {
        LOG_PRINT("[CharacterLogicFSM] Animation Event: {}", eventName);
    }
    */

    void CharacterLogicFSM::CacheComponents()
    {
        m_inputBinding = GetGameObject()->GetComponent<game::InputBinding>();
        m_animFSM = GetGameObject()->GetComponent<CharacterAnimationFSM>();
        
        // AnimationFSM을 리스너로 등록
        if (m_animFSM)
        {
            AddListener(m_animFSM);
        }
    }

    void CharacterLogicFSM::ProcessInput()
    {
        if (!m_inputBinding)
        {
            return;
        }
        
        // 이동 방향 계산
        m_context.moveDirection = GetMoveInputDirection();
        m_context.moveSpeed = m_moveSpeed;
    }

    void CharacterLogicFSM::UpdateState()
    {
        switch (m_currentState)
        {
        case CharacterState::Idle:   UpdateIdle();   break;
        case CharacterState::Walk:   UpdateWalk();   break;
        case CharacterState::Attack: UpdateAttack(); break;
        // === 확장용 상태 (예시) ===
        // case CharacterState::Run:    UpdateRun();    break;
        // case CharacterState::Jump:   UpdateJump();   break;
        // case CharacterState::Fall:   UpdateFall();   break;
        // case CharacterState::Hit:    UpdateHit();    break;
        // case CharacterState::Dead:   UpdateDead();   break;
        default: break;
        }
    }

    // === State Enter ===
    void CharacterLogicFSM::OnEnterIdle()
    {
        m_context.moveSpeed = 0.0f;
    }

    void CharacterLogicFSM::OnEnterWalk()
    {
        m_context.moveSpeed = m_moveSpeed;
    }

    void CharacterLogicFSM::OnEnterAttack()
    {
        // 공격 시작
    }

    // ===함수 예시 ===
    /*
    void CharacterLogicFSM::OnEnterRun()
    {
        m_context.moveSpeed = m_runSpeed;
    }

    void CharacterLogicFSM::OnEnterJump()
    {
        if (m_rigidbody)
        {
            engine::Vector3 velocity = m_rigidbody->GetLinearVelocity();
            velocity.y = m_jumpForce;
            m_rigidbody->SetLinearVelocity(velocity);
        }
    }

    void CharacterLogicFSM::OnEnterFall() {}
    void CharacterLogicFSM::OnEnterHit() {}
    void CharacterLogicFSM::OnEnterDead() {}
    */

    // === State Exit ===
    void CharacterLogicFSM::OnExitIdle() {}
    void CharacterLogicFSM::OnExitWalk() {}
    void CharacterLogicFSM::OnExitAttack() {}

    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백 (Push 방식)
    // 파생 클래스에서 오버라이드하여 구체적인 처리 구현
    // ═══════════════════════════════════════════════════════════════
    void CharacterLogicFSM::OnCollisionEnter(const engine::CollisionInfo& info)
    {
        // 기본 구현: 로그만 출력 (파생 클래스에서 오버라이드)
        LOG_PRINT("[CharacterLogicFSM] OnCollisionEnter: {} <-> {}", 
            GetGameObject()->GetName(),
            info.gameObject ? info.gameObject->GetName() : "null");
    }

    void CharacterLogicFSM::OnCollisionStay(const engine::CollisionInfo& info)
    {
        // 기본 구현: 아무것도 안 함
    }

    void CharacterLogicFSM::OnCollisionExit(const engine::CollisionInfo& info)
    {
        // 기본 구현: 아무것도 안 함
    }

    void CharacterLogicFSM::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        // 기본 구현: 로그만 출력 (파생 클래스에서 오버라이드)
        LOG_PRINT("[CharacterLogicFSM] OnTriggerEnter: {} <-> {}", 
            GetGameObject()->GetName(),
            info.gameObject ? info.gameObject->GetName() : "null");
    }

    void CharacterLogicFSM::OnTriggerStay(const engine::CollisionInfo& info)
    {
        // 기본 구현: 아무것도 안 함
    }

    void CharacterLogicFSM::OnTriggerExit(const engine::CollisionInfo& info)
    {
        // 기본 구현: 아무것도 안 함
    }
   

    // === State Update ===
    void CharacterLogicFSM::UpdateIdle()
    {
        // 이동 입력 체크
        if (IsMoving())
        {
            ChangeState(CharacterState::Walk);
            return;
        }

        // 공격 입력 체크
        if (IsAttackPressed())
        {
            ChangeState(CharacterState::Attack);
            return;
        }
    }

    void CharacterLogicFSM::UpdateWalk()
    {
        // 정지
        if (!IsMoving())
        {
            ChangeState(CharacterState::Idle);
            return;
        }

        // 공격
        if (IsAttackPressed())
        {
            ChangeState(CharacterState::Attack);
            return;
        }

        // 이동 적용
        engine::Vector3 currentPos = GetTransform()->GetLocalPosition();
        currentPos += m_context.moveDirection * m_moveSpeed * engine::Time::DeltaTime();
        GetTransform()->SetLocalPosition(currentPos);
    }

    void CharacterLogicFSM::UpdateAttack()
    {
        // 공격 상태에서는 기본적으로 아무것도 안 함
        // 공격 종료는 OnAnimationFinished에서 처리하거나
        // 상속 클래스에서 타이머로 처리
    }

    // === 확장용 Update 함수 (주석 해제하여 사용) ===
    /*
    void CharacterLogicFSM::UpdateRun()
    {
        if (!IsMoving())
        {
            ChangeState(CharacterState::Idle);
            return;
        }
        if (!IsRunning())
        {
            ChangeState(CharacterState::Walk);
            return;
        }
        if (IsJumpPressed() && m_context.isGrounded)
        {
            ChangeState(CharacterState::Jump);
            return;
        }
        if (IsAttackPressed())
        {
            ChangeState(CharacterState::Attack);
            return;
        }

        if (m_rigidbody)
        {
            engine::Vector3 velocity = m_context.moveDirection * m_runSpeed;
            velocity.y = m_rigidbody->GetLinearVelocity().y;
            m_rigidbody->SetLinearVelocity(velocity);
        }
    }

    void CharacterLogicFSM::UpdateJump()
    {
        if (m_rigidbody && m_rigidbody->GetLinearVelocity().y < 0.0f)
        {
            ChangeState(CharacterState::Fall);
            return;
        }
    }

    void CharacterLogicFSM::UpdateFall()
    {
        // 착지는 OnGrounded에서 처리
    }

    void CharacterLogicFSM::UpdateHit()
    {
        // 피격 애니메이션 종료는 OnAnimationFinished에서 처리
    }

    void CharacterLogicFSM::UpdateDead()
    {
        // 사망 상태에서는 아무것도 안 함
    }
    */

    // === Utility ===
    engine::Vector3 CharacterLogicFSM::GetMoveInputDirection() const
    {
        if (!m_inputBinding)
        {
            return engine::Vector3::Zero;
        }

        engine::Vector3 direction = engine::Vector3::Zero;

        if (m_inputBinding->IsHeld(m_inputMoveUp))
        {
            direction.y += 1.0f;  // 탑다운: Y축이 위
        }
        if (m_inputBinding->IsHeld(m_inputMoveDown))
        {
            direction.y -= 1.0f;
        }
        if (m_inputBinding->IsHeld(m_inputMoveLeft))
        {
            direction.x -= 1.0f;
        }
        if (m_inputBinding->IsHeld(m_inputMoveRight))
        {
            direction.x += 1.0f;
        }

        if (direction.LengthSquared() > 0.0f)
        {
            direction.Normalize();
        }

        return direction;
    }

    bool CharacterLogicFSM::IsMoving() const
    {
        return m_context.moveDirection.LengthSquared() > 0.001f;
    }

    bool CharacterLogicFSM::IsAttackPressed() const
    {
        if (!m_inputBinding)
        {
            return false;
        }
        return m_inputBinding->IsPressed(m_inputAttack);
    }

    // === 확장용 유틸리티 (주석 해제하여 사용) ===
    /*
    bool CharacterLogicFSM::IsRunning() const
    {
        if (!m_inputBinding)
        {
            return false;
        }
        return m_inputBinding->IsHeld(m_inputRun);
    }

    bool CharacterLogicFSM::IsJumpPressed() const
    {
        if (!m_inputBinding)
        {
            return false;
        }
        return m_inputBinding->IsPressed(m_inputJump);
    }
    */

    void CharacterLogicFSM::NotifyListenersEnter()
    {
        for (auto* listener : m_listeners)
        {
            if (listener)
            {
                listener->OnStateEnter(m_context);
            }
        }
    }

    void CharacterLogicFSM::NotifyListenersExit()
    {
        for (auto* listener : m_listeners)
        {
            if (listener)
            {
                listener->OnStateExit(m_context);
            }
        }
    }

    void CharacterLogicFSM::NotifyListenersUpdate()
    {
        for (auto* listener : m_listeners)
        {
            if (listener)
            {
                listener->OnStateUpdate(m_context);
            }
        }
    }

    void CharacterLogicFSM::OnGui()
    {
        // 현재 상태 표시
        ImGui::Text("Current State: %s", CharacterStateToString(m_currentState));
        ImGui::Text("State Timer: %.2f", m_stateTimer);
        ImGui::Text("Move Direction: (%.2f, %.2f, %.2f)", 
            m_context.moveDirection.x, m_context.moveDirection.y, m_context.moveDirection.z);
        
        ImGui::Separator();
        
        // 이동 설정
        ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.1f, 0.0f, 20.0f);
        
        // 입력 바인딩 이름 설정
        if (ImGui::CollapsingHeader("Input Binding Names"))
        {
            char buf[64];
            
            strcpy_s(buf, m_inputMoveUp.c_str());
            if (ImGui::InputText("Move Up", buf, 64))
                m_inputMoveUp = buf;
            
            strcpy_s(buf, m_inputMoveDown.c_str());
            if (ImGui::InputText("Move Down", buf, 64))
                m_inputMoveDown = buf;
            
            strcpy_s(buf, m_inputMoveLeft.c_str());
            if (ImGui::InputText("Move Left", buf, 64))
                m_inputMoveLeft = buf;
            
            strcpy_s(buf, m_inputMoveRight.c_str());
            if (ImGui::InputText("Move Right", buf, 64))
                m_inputMoveRight = buf;
            
            strcpy_s(buf, m_inputAttack.c_str());
            if (ImGui::InputText("Attack", buf, 64))
                m_inputAttack = buf;
        }
    }

    void CharacterLogicFSM::Save(engine::json& j) const
    {
        Object::Save(j);
        
        j["MoveSpeed"] = m_moveSpeed;
        
        j["InputMoveUp"] = m_inputMoveUp;
        j["InputMoveDown"] = m_inputMoveDown;
        j["InputMoveLeft"] = m_inputMoveLeft;
        j["InputMoveRight"] = m_inputMoveRight;
        j["InputAttack"] = m_inputAttack;
    }

    void CharacterLogicFSM::Load(const engine::json& j)
    {
        Object::Load(j);
        
        engine::JsonGet(j, "MoveSpeed", m_moveSpeed);
        
        engine::JsonGet(j, "InputMoveUp", m_inputMoveUp);
        engine::JsonGet(j, "InputMoveDown", m_inputMoveDown);
        engine::JsonGet(j, "InputMoveLeft", m_inputMoveLeft);
        engine::JsonGet(j, "InputMoveRight", m_inputMoveRight);
        engine::JsonGet(j, "InputAttack", m_inputAttack);
    }

    std::string CharacterLogicFSM::GetType() const
    {
        return "CharacterLogicFSM";
    }
}
