#include "GamePCH.h"
#include "TestShootingPlayer.h"

#include "AimPointer.h"
#include "TempBulletFactory.h"

#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Engine/Core/System/Input.h>
#include <Engine/Core/System/MyTime.h>

namespace game
{
    void TestShootingPlayer::Awake()
    {
        BaseControllerScript::Awake();
    }

    void TestShootingPlayer::Start()
    {
        BaseControllerScript::Start();

        // FSM 초기화 (한 번만)
        if (!m_fsmInitialized && m_logicFSM)
        {
            InitializeFSM();
            m_fsmInitialized = true;
        }

        // Procedural Aim 활성화
        if (m_animFSM && m_enableUpperBodyAim)
        {
            m_animFSM->SetProceduralAimEnabled(true);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 컴포넌트 캐싱
    // ═══════════════════════════════════════════════════════════════
    void TestShootingPlayer::CacheComponents()
    {
        // 부모 클래스의 CacheComponents() 호출 (FSM 컴포넌트 찾기)
        BaseControllerScript::CacheComponents();

        // 추가 컴포넌트 찾기
        if (!GetGameObject()) return;

        m_rigidbody = GetGameObject()->GetComponent<engine::Rigidbody>();

        // AimPointer와 BulletFactory 찾기
        auto* scene = engine::SceneManager::Get().GetScene();
        if (scene)
        {
            
            m_aimPointer = GetGameObject()->GetComponent<AimPointer>();

            m_bulletFactory = GetGameObject()->GetComponent<TempBulletFactory>();
            if (!m_bulletFactory)
            {
                if (auto* factoryGO = scene->FindGameObject("BulletFactory"))
                {
                    m_bulletFactory = factoryGO->GetComponent<TempBulletFactory>();
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 입력 처리
    // ═══════════════════════════════════════════════════════════════
    void TestShootingPlayer::ProcessInput()
    {
        if (!m_logicFSM) return;

        // 1. 이동 입력 → Parameters
        engine::Vector3 moveDir = GetMoveInputDirection();
        bool isMoving = moveDir.LengthSquared() > 0.001f;
        
        m_logicFSM->SetParameter("IsMoving", isMoving);
        if (isMoving)
        {
            m_logicFSM->SetParameter("MoveX", moveDir.x);
            m_logicFSM->SetParameter("MoveZ", moveDir.z);
        }

        // 2. 공격 입력 처리
        bool isMousePressed = engine::Input::IsMousePressed(engine::Input::Buttons::LEFT);
        bool isMouseHeld = engine::Input::IsMouseHeld(engine::Input::Buttons::LEFT);
        bool isMouseReleased = engine::Input::IsMouseReleased(engine::Input::Buttons::LEFT);

        // 마우스 Pressed → 트리거 설정 (Idle/Walk → IdleShoot/WalkShoot 전이용)
        if (isMousePressed)
        {
            m_logicFSM->SetTrigger("Attack");
        }

        // 마우스 Held/Released → IsShooting 파라미터 설정 (Shoot 상태 유지/해제용)
        m_logicFSM->SetParameter("IsShooting", isMouseHeld);

        // 3. 상체 조준 업데이트 (매 프레임)
        //UpdateUpperBodyAim();
    }

    // ═══════════════════════════════════════════════════════════════
    // 게임 로직 업데이트
    // ═══════════════════════════════════════════════════════════════
    void TestShootingPlayer::UpdateGameLogic()
    {
        if (!m_rigidbody || !m_logicFSM) return;

        std::string currentState = m_logicFSM->GetCurrentState();

        // 이동 처리 (Walk, WalkShoot 상태에서)
        if (currentState == "Walk" || currentState == "WalkShoot")
        {
            engine::Vector3 moveDir = GetMoveInputDirection();
            if (moveDir.LengthSquared() > 0.001f)
            {
                engine::Transform* transform = GetGameObject()->GetTransform();
                if (transform)
                {
                    engine::Vector3 currentPos = transform->GetLocalPosition();
                    float deltaTime = engine::Time::DeltaTime();
                    engine::Vector3 newPos = currentPos + moveDir * m_moveSpeed * deltaTime;
                    transform->SetLocalPosition(newPos);
                }
            }
        }

        // 공격 상태에서 연사속도에 맞춰 총알 발사 (IdleShoot, WalkShoot)
        if (currentState == "IdleShoot" || currentState == "WalkShoot")
        {
            m_fireTimer += engine::Time::DeltaTime();
            
            // 연사속도에 맞춰 발사
            if (m_fireTimer >= m_fireRate)
            {
                HandleShooting();
                m_fireTimer = 0.0f;
            }
        }
        else
        {
            // Shoot 상태가 아니면 타이머 리셋
            //m_fireTimer = 0.0f;
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 상태 변화 콜백
    // ═══════════════════════════════════════════════════════════════
    void TestShootingPlayer::OnStateEntered(const std::string& state)
    {
        // Shoot 상태 진입 시 발사 타이머 리셋 (즉시 첫 발사)
        if (state == "IdleShoot" || state == "WalkShoot")
        {
            m_fireTimer = m_fireRate;  // 즉시 발사되도록 타이머를 최대값으로 설정
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // FSM 초기화
    // ═══════════════════════════════════════════════════════════════
    void TestShootingPlayer::InitializeFSM()
    {
        if (!m_logicFSM) return;
        if (m_fsmInitialized) return;

        // 기존 상태 초기화
        m_logicFSM->ClearStates();

        // ─────────────────────────────────────────────
        // 1. 상태 추가
        // ─────────────────────────────────────────────
        AddFSMState("Idle", true);      // 기본 상태
        AddFSMState("IdleShoot");
        AddFSMState("Walk");
        AddFSMState("WalkShoot");

        // 모든 상태 추가 후 m_stateMap 업데이트
        m_logicFSM->UpdateStateMap();

        // 기본 상태 설정 및 초기화
        m_logicFSM->SetDefaultState("Idle");
        m_logicFSM->InitializeCurrentState();

        // ─────────────────────────────────────────────
        // 2. 전이 추가: Idle <-> Walk (이동 입력)
        // ─────────────────────────────────────────────
        AddFSMTransition("Idle", "Walk", "IsMoving", BoolTrue());
        AddFSMTransition("Walk", "Idle", "IsMoving", BoolFalse());

        // ─────────────────────────────────────────────
        // 3. 전이 추가: Idle -> IdleShoot (공격 트리거)
        // ─────────────────────────────────────────────
        AddFSMTransition("Idle", "IdleShoot", "Attack", Trigger());

        // ─────────────────────────────────────────────
        // 4. 전이 추가: Walk -> WalkShoot (공격 트리거)
        // ─────────────────────────────────────────────
        AddFSMTransition("Walk", "WalkShoot", "Attack", Trigger());

        // ─────────────────────────────────────────────
        // 5. 전이 추가: IdleShoot <-> WalkShoot (이동 입력, Shoot 상태 유지)
        // ─────────────────────────────────────────────
        // Shoot 상태에서도 이동 입력에 따라 전이 가능
        AddFSMTransition("IdleShoot", "WalkShoot", "IsMoving", BoolTrue());
        AddFSMTransition("WalkShoot", "IdleShoot", "IsMoving", BoolFalse());

        // ─────────────────────────────────────────────
        // 6. 전이 추가: IdleShoot -> Idle (마우스 Released)
        // ─────────────────────────────────────────────
        // 마우스를 떼면 Idle로 복귀
        AddFSMTransition("IdleShoot", "Idle", "IsShooting", BoolFalse());

        // ─────────────────────────────────────────────
        // 7. 전이 추가: WalkShoot -> Walk (마우스 Released)
        // ─────────────────────────────────────────────
        // 마우스를 떼면 일단 Walk로 가고, 다음 프레임에 IsMoving에 따라 Idle로 전이됨
        AddFSMTransition("WalkShoot", "Walk", "IsShooting", BoolFalse());
    }

    // ═══════════════════════════════════════════════════════════════
    // 플레이어 전용 입력 함수
    // ═══════════════════════════════════════════════════════════════
    engine::Vector3 TestShootingPlayer::GetMoveInputDirection() const
    {
        engine::Vector3 direction = engine::Vector3::Zero;

        if (engine::Input::IsKeyHeld(engine::Keys::W)) direction.z += 1.0f;
        if (engine::Input::IsKeyHeld(engine::Keys::S)) direction.z -= 1.0f;
        if (engine::Input::IsKeyHeld(engine::Keys::A)) direction.x -= 1.0f;
        if (engine::Input::IsKeyHeld(engine::Keys::D)) direction.x += 1.0f;

        if (direction.LengthSquared() > 0.0f)
        {
            direction.Normalize();
        }

        return direction;
    }

    // ═══════════════════════════════════════════════════════════════
    // 플레이어 전용 액션
    // ═══════════════════════════════════════════════════════════════
    void TestShootingPlayer::HandleShooting()
    {
        if (m_bulletFactory && m_aimPointer)
        {
            engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
            engine::Vector3 direction = m_aimPointer->GetDirectionFrom(playerPos);

            m_bulletFactory->Fire(playerPos, direction);
        }
    }

    void TestShootingPlayer::UpdateUpperBodyAim()
    {
        if (!m_enableUpperBodyAim || !m_animFSM || !m_aimPointer)
        {
            return;
        }

        float yaw = CalculateAimYaw();
        m_animFSM->SetUpperBodyYaw(yaw);
    }

    float TestShootingPlayer::CalculateAimYaw() const
    {
        if (!m_aimPointer)
        {
            return 0.0f;
        }

        engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
        engine::Vector3 aimPos = m_aimPointer->GetTransform()->GetWorldPosition();

        engine::Vector3 toAim = aimPos - playerPos;
        toAim.y = 0.0f;

        if (toAim.LengthSquared() < 0.001f)
        {
            return 0.0f;
        }

        toAim.Normalize();

        // 플레이어 전방 (+Z)
        engine::Vector3 forward = engine::Vector3::UnitZ;
        engine::Quaternion playerRot = GetTransform()->GetWorldRotation();
        forward = engine::Vector3::Transform(forward, playerRot);
        forward.y = 0.0f;
        forward.Normalize();

        // 상대 각도 계산
        float dotProduct = forward.Dot(toAim);
        engine::Vector3 crossProduct = forward.Cross(toAim);

        float angleRad = atan2f(crossProduct.y, dotProduct);
        float angleDeg = engine::ToDegree(angleRad);

        return angleDeg;
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void TestShootingPlayer::OnGui()
    {
        BaseControllerScript::OnGui();

        ImGui::Separator();
        ImGui::Text("TestShootingPlayer:");
        ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.1f, 0.0f, 100.0f);
        ImGui::Checkbox("Enable Upper Body Aim", &m_enableUpperBodyAim);

        ImGui::Separator();
        ImGui::Text("References:");
        ImGui::Text("  AimPointer: %s", m_aimPointer ? "Found" : "NOT FOUND");
        ImGui::Text("  BulletFactory: %s", m_bulletFactory ? "Found" : "NOT FOUND");
        ImGui::Text("  AnimFSM: %s", m_animFSM ? "Found" : "NOT FOUND");
        ImGui::Text("  Rigidbody: %s", m_rigidbody ? "Found" : "NOT FOUND");

        if (m_aimPointer)
        {
            float yaw = CalculateAimYaw();
            ImGui::Text("  Aim Yaw: %.1f deg", yaw);
        }
    }

    void TestShootingPlayer::Save(engine::json& j) const
    {
        BaseControllerScript::Save(j);
        j["MoveSpeed"] = m_moveSpeed;
        j["EnableUpperBodyAim"] = m_enableUpperBodyAim;
        j["FSMInitialized"] = m_fsmInitialized;
    }

    void TestShootingPlayer::Load(const engine::json& j)
    {
        BaseControllerScript::Load(j);

        if (j.contains("MoveSpeed"))
        {
            m_moveSpeed = j["MoveSpeed"].get<float>();
        }
        if (j.contains("EnableUpperBodyAim"))
        {
            m_enableUpperBodyAim = j["EnableUpperBodyAim"].get<bool>();
        }
        if (j.contains("FSMInitialized"))
        {
            m_fsmInitialized = j["FSMInitialized"].get<bool>();
        }
    }
}
