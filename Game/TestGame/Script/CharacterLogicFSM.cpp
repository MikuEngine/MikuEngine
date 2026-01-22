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
        case CharacterState::Test1:  return "Test1";
        case CharacterState::Test2:  return "Test2";
        case CharacterState::Test3:  return "Test3";
        case CharacterState::Test4:  return "Test4";
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
        OnEnterState(m_currentState);
        NotifyListenersEnter();
    }

    void CharacterLogicFSM::Update()
    {
        m_stateTimer += engine::Time::DeltaTime();
        
        ProcessInput();
        UpdateState();
        NotifyListenersUpdate();
    }

    void CharacterLogicFSM::ChangeState(CharacterState newState)
    {
        if (m_currentState == newState)
        {
            return;
        }

        // Exit
        m_context.previousState = m_currentState;
        NotifyListenersExit();
        OnExitState(m_currentState);

        // 상태 변경
        m_currentState = newState;
        m_context.currentState = newState;
        m_stateTimer = 0.0f;

        // Enter
        OnEnterState(m_currentState);
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
        // 베이스: 애니메이션 종료 시 Idle로 복귀
        // 자식에서 오버라이드하여 다른 처리 가능
        if (m_currentState == finishedState)
        {
            ChangeState(CharacterState::Idle);
        }
    }

    void CharacterLogicFSM::CacheComponents()
    {
        m_inputBinding = GetGameObject()->GetComponent<game::InputBinding>();
        m_animFSM = GetGameObject()->GetComponent<CharacterAnimationFSM>();
        
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
        
        m_context.moveDirection = GetMoveInputDirection();
        m_context.moveSpeed = m_moveSpeed;
    }

    void CharacterLogicFSM::UpdateState()
    {
        UpdateCurrentState();
    }

    void CharacterLogicFSM::OnEnterState(CharacterState state)
    {
        // 베이스: 기본 상태별 처리
        switch (state)
        {
        case CharacterState::Idle:
            m_context.moveSpeed = 0.0f;
            break;
        case CharacterState::Walk:
            m_context.moveSpeed = m_moveSpeed;
            break;
        case CharacterState::Attack:
            // 공격 시작
            break;
        default:
            // 자식에서 처리
            break;
        }
    }

    void CharacterLogicFSM::OnExitState(CharacterState state)
    {
        // 베이스: 기본 처리 없음
        // 자식에서 오버라이드
    }

    void CharacterLogicFSM::UpdateCurrentState()
    {
        // 베이스: 기본 상태별 업데이트
        switch (m_currentState)
        {
        case CharacterState::Idle:
            // 이동 입력 -> Walk
            if (IsMoving())
            {
                ChangeState(CharacterState::Walk);
                return;
            }
            // 공격 입력 -> Attack
            if (IsAttackPressed())
            {
                ChangeState(CharacterState::Attack);
                return;
            }
            break;
            
        case CharacterState::Walk:
            // 정지 -> Idle
            if (!IsMoving())
            {
                ChangeState(CharacterState::Idle);
                return;
            }
            // 공격 -> Attack
            if (IsAttackPressed())
            {
                ChangeState(CharacterState::Attack);
                return;
            }
            // 이동 적용
            {
                engine::Vector3 pos = GetTransform()->GetLocalPosition();
                pos += m_context.moveDirection * m_moveSpeed * engine::Time::DeltaTime();
                GetTransform()->SetLocalPosition(pos);
            }
            break;
            
        case CharacterState::Attack:
            // 공격은 애니메이션 종료(OnAnimationFinished)에서 처리
            break;
            
        default:
            // Test1~Test4 등은 자식에서 처리
            break;
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백 (베이스: 빈 구현)
    // ═══════════════════════════════════════════════════════════════
    void CharacterLogicFSM::OnCollisionEnter(const engine::CollisionInfo& info) {}
    void CharacterLogicFSM::OnCollisionStay(const engine::CollisionInfo& info) {}
    void CharacterLogicFSM::OnCollisionExit(const engine::CollisionInfo& info) {}
    void CharacterLogicFSM::OnTriggerEnter(const engine::CollisionInfo& info) {}
    void CharacterLogicFSM::OnTriggerStay(const engine::CollisionInfo& info) {}
    void CharacterLogicFSM::OnTriggerExit(const engine::CollisionInfo& info) {}

    // ═══════════════════════════════════════════════════════════════
    // 유틸리티
    // ═══════════════════════════════════════════════════════════════
    engine::Vector3 CharacterLogicFSM::GetMoveInputDirection() const
    {
        if (!m_inputBinding)
        {
            return engine::Vector3::Zero;
        }

        engine::Vector3 direction = engine::Vector3::Zero;

        if (m_inputBinding->IsHeld(m_inputMoveUp))    direction.y += 1.0f;
        if (m_inputBinding->IsHeld(m_inputMoveDown))  direction.y -= 1.0f;
        if (m_inputBinding->IsHeld(m_inputMoveLeft))  direction.x -= 1.0f;
        if (m_inputBinding->IsHeld(m_inputMoveRight)) direction.x += 1.0f;

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
        return m_inputBinding && m_inputBinding->IsPressed(m_inputAttack);
    }

    void CharacterLogicFSM::NotifyListenersEnter()
    {
        for (auto* listener : m_listeners)
        {
            if (listener) listener->OnStateEnter(m_context);
        }
    }

    void CharacterLogicFSM::NotifyListenersExit()
    {
        for (auto* listener : m_listeners)
        {
            if (listener) listener->OnStateExit(m_context);
        }
    }

    void CharacterLogicFSM::NotifyListenersUpdate()
    {
        for (auto* listener : m_listeners)
        {
            if (listener) listener->OnStateUpdate(m_context);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void CharacterLogicFSM::OnGui()
    {
        ImGui::Text("Current State: %s", CharacterStateToString(m_currentState));
        ImGui::Text("State Timer: %.2f", m_stateTimer);
        ImGui::Text("Move Direction: (%.2f, %.2f, %.2f)", 
            m_context.moveDirection.x, m_context.moveDirection.y, m_context.moveDirection.z);
        
        ImGui::Separator();
        ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.1f, 0.0f, 20.0f);
        
        if (ImGui::CollapsingHeader("Input Binding Names"))
        {
            char buf[64];
            
            strcpy_s(buf, m_inputMoveUp.c_str());
            if (ImGui::InputText("Move Up", buf, 64)) m_inputMoveUp = buf;
            
            strcpy_s(buf, m_inputMoveDown.c_str());
            if (ImGui::InputText("Move Down", buf, 64)) m_inputMoveDown = buf;
            
            strcpy_s(buf, m_inputMoveLeft.c_str());
            if (ImGui::InputText("Move Left", buf, 64)) m_inputMoveLeft = buf;
            
            strcpy_s(buf, m_inputMoveRight.c_str());
            if (ImGui::InputText("Move Right", buf, 64)) m_inputMoveRight = buf;
            
            strcpy_s(buf, m_inputAttack.c_str());
            if (ImGui::InputText("Attack", buf, 64)) m_inputAttack = buf;
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
}
