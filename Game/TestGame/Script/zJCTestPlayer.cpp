#include "GamePCH.h"
#include "zJCTestPlayer.h"

#include <Framework/Object/Component/LogicFSM.h>
#include <Framework/Object/Component/AnimFSM.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/Object/GameObject/GameObject.h>
#include <Core/System/MyTime.h>


namespace game
{
	void zJCTestPlayer::Awake()
	{
		// BaseControllerScript의 Awake() 호출 (CacheComponents() 호출)
		BaseControllerScript::Awake();
	}

	void zJCTestPlayer::Start()
	{
		// BaseControllerScript의 Start() 호출 (FSM 콜백 등록)
		BaseControllerScript::Start();

		// FSM 초기화 (한 번만)
		// 씬 파일에 상태가 없으면 코드에서 추가
		if (!m_fsmInitialized && m_logicFSM)
		{
			InitializeFSM();
			m_fsmInitialized = true;
		}
	}

	void zJCTestPlayer::Update()
	{
		// BaseControllerScript의 Update() 호출
		// (입력 처리, FSM 업데이트, 게임 로직 업데이트)
		BaseControllerScript::Update();
	}

	// ═══════════════════════════════════════════════════════════════
	// 입력 처리
	// ═══════════════════════════════════════════════════════════════
	void zJCTestPlayer::ProcessInput()
	{
		if (!m_logicFSM) return;

		// WASD 입력을 Bool 파라미터로 설정
		bool moveUp = engine::Input::IsKeyHeld(engine::Keys::W);
		bool moveDown = engine::Input::IsKeyHeld(engine::Keys::S);
		bool moveLeft = engine::Input::IsKeyHeld(engine::Keys::A);
		bool moveRight = engine::Input::IsKeyHeld(engine::Keys::D);

		m_logicFSM->SetParameter("MoveUp", moveUp);
		m_logicFSM->SetParameter("MoveDown", moveDown);
		m_logicFSM->SetParameter("MoveLeft", moveLeft);
		m_logicFSM->SetParameter("MoveRight", moveRight);
	}

	// ═══════════════════════════════════════════════════════════════
	// 게임 로직 업데이트
	// ═══════════════════════════════════════════════════════════════
	void zJCTestPlayer::UpdateGameLogic()
	{
		if (!m_rigidbody || !m_logicFSM) return;

		std::string currentState = m_logicFSM->GetCurrentState();
		engine::Vector3 moveDirection(0.0f, 0.0f, 0.0f);

		// 현재 스테이트에 따라 이동 방향 결정
		if (currentState == "MoveUp")
		{
			moveDirection.z = m_moveSpeed;
		}
		else if (currentState == "MoveDown")
		{
			moveDirection.z = -m_moveSpeed;
		}
		else if (currentState == "MoveLeft")
		{
			moveDirection.x = -m_moveSpeed;
		}
		else if (currentState == "MoveRight")
		{
			moveDirection.x = m_moveSpeed;
		}
		// Idle 상태는 moveDirection이 (0, 0, 0)으로 유지됨

		// 키네마틱 Rigidbody는 SetLinearVelocity()가 작동하지 않음
		// MovePosition()을 사용하여 이동 (속도 * DeltaTime)
		if (moveDirection.LengthSquared() > 0.0f)
		{
			engine::Transform* transform = GetGameObject()->GetTransform();
			if (transform)
			{
				engine::Vector3 currentPos = transform->GetLocalPosition();
				float deltaTime = engine::Time::DeltaTime();
				engine::Vector3 newPos = currentPos + moveDirection * deltaTime;
				GetTransform()->SetLocalPosition(newPos);
			}
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// 상태 진입 콜백
	// ═══════════════════════════════════════════════════════════════
	void zJCTestPlayer::OnStateEntered(const std::string& state)
	{
		LOG_PRINT("[zJCTestPlayer] State Entered: {}", state);
	}

	// ═══════════════════════════════════════════════════════════════
	// 상태 종료 콜백
	// ═══════════════════════════════════════════════════════════════
	void zJCTestPlayer::OnStateExited(const std::string& state)
	{
		LOG_PRINT("[zJCTestPlayer] State Exited: {}", state);
	}

	// ═══════════════════════════════════════════════════════════════
	// 컴포넌트 캐싱
	// ═══════════════════════════════════════════════════════════════
	void zJCTestPlayer::CacheComponents()
	{
		// 부모 클래스의 CacheComponents() 호출 (FSM 컴포넌트 찾기)
		BaseControllerScript::CacheComponents();
		
		// 추가 컴포넌트 찾기
		if (!GetGameObject()) return;
		
		m_rigidbody = GetGameObject()->GetComponent<engine::Rigidbody>();
	}

	// ═══════════════════════════════════════════════════════════════
	// FSM 초기화 (스테이트 및 전이 설정)
	// ═══════════════════════════════════════════════════════════════
	void zJCTestPlayer::InitializeFSM()
	{
		if (!m_logicFSM) return;
		if (m_fsmInitialized) return;

		// 기존 상태 초기화
		m_logicFSM->ClearStates();

		// ─────────────────────────────────────────────
		// 1. 상태 추가
		// ─────────────────────────────────────────────
		AddFSMState("Idle", true);      // 기본 상태
		AddFSMState("MoveUp");
		AddFSMState("MoveDown");
		AddFSMState("MoveLeft");
		AddFSMState("MoveRight");

		// 모든 상태 추가 후 m_stateMap 업데이트 (포인터 무효화 방지)
		m_logicFSM->UpdateStateMap();

		// 기본 상태 설정 및 초기화
		m_logicFSM->SetDefaultState("Idle");
		m_logicFSM->InitializeCurrentState();

		// ─────────────────────────────────────────────
		// 2. 전이 추가: Idle -> 각 이동 상태
		// ─────────────────────────────────────────────
		AddFSMTransition("Idle", "MoveUp", "MoveUp", BoolTrue());
		AddFSMTransition("Idle", "MoveDown", "MoveDown", BoolTrue());
		AddFSMTransition("Idle", "MoveLeft", "MoveLeft", BoolTrue());
		AddFSMTransition("Idle", "MoveRight", "MoveRight", BoolTrue());

		// ─────────────────────────────────────────────
		// 3. 전이 추가: 각 이동 상태 -> Idle
		// ─────────────────────────────────────────────
		AddFSMTransition("MoveUp", "Idle", "MoveUp", BoolFalse());
		AddFSMTransition("MoveDown", "Idle", "MoveDown", BoolFalse());
		AddFSMTransition("MoveLeft", "Idle", "MoveLeft", BoolFalse());
		AddFSMTransition("MoveRight", "Idle", "MoveRight", BoolFalse());
	}

	// ═══════════════════════════════════════════════════════════════
	// 에디터 GUI
	// ═══════════════════════════════════════════════════════════════
	void zJCTestPlayer::OnGui()
	{
		ImGui::Text("zJCTestPlayer Script");

		ImGui::Separator();

		// 에디터에서도 컴포넌트를 찾을 수 있도록 보장
		if (!m_logicFSM)
		{
			CacheComponents();

			if (!m_fsmInitialized)
			{
				InitializeFSM();
				m_fsmInitialized = true;
			}
		}

		// FSM 초기화 버튼 (에디터 모드에서도 사용 가능)
		if (m_logicFSM)
		{
			//OnGui에서는, 리-이니셜라이즈만 하고, 처음 진입시 !m_fsmInitialized이면 이니셜라이즈 한다.
			if (m_fsmInitialized)
			{
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "FSM initialized");

				// 재초기화 버튼 (필요시)
				if (ImGui::Button("Re-initialize FSM"))
				{
					m_fsmInitialized = false;
					CacheComponents();  // 컴포넌트 다시 찾기
					InitializeFSM();
					m_fsmInitialized = true;
				}
			}

			// 상태가 비어있고 아직 초기화되지 않았으면 초기화 버튼 표시
			//if (!m_fsmInitialized)
			//{
			//    if (ImGui::Button("Initialize FSM (Editor)"))
			//    {
			//        InitializeFSM();
			//        m_fsmInitialized = true;
			//    }
			//    ImGui::SameLine();
			//    ImGui::TextColored(ImVec4(1, 1, 0, 1), "Click to initialize FSM states");
			//}
			//else
			//{
			//    ImGui::TextColored(ImVec4(0, 1, 0, 1), "FSM initialized");
			//    
			//    // 재초기화 버튼 (필요시)
			//    if (ImGui::Button("Re-initialize FSM"))
			//    {
			//        m_fsmInitialized = false;
			//        CacheFSMComponents();  // 컴포넌트 다시 찾기
			//        InitializeFSM();
			//        m_fsmInitialized = true;
			//    }
			//}
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "LogicFSM component not found!");
			ImGui::Text("Make sure LogicFSM component is attached to this GameObject.");
		}

		ImGui::Separator();

		// 이동 속도 설정
		ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.1f, 0.0f, 100.0f);
	}

	// ═══════════════════════════════════════════════════════════════
	// 직렬화
	// ═══════════════════════════════════════════════════════════════
	void zJCTestPlayer::Save(engine::json& j) const
	{
		BaseControllerScript::Save(j);

		j["MoveSpeed"] = m_moveSpeed;
		j["FSMInitialized"] = m_fsmInitialized;
	}

	void zJCTestPlayer::Load(const engine::json& j)
	{
		BaseControllerScript::Load(j);

		// 기본 정보 로드
		if (j.contains("MoveSpeed"))
		{
			m_moveSpeed = j["MoveSpeed"].get<float>();
		}
		if (j.contains("FSMInitialized"))
		{
			m_fsmInitialized = j["FSMInitialized"].get<bool>();
		}

		// 주의: Load() 시점에는 같은 GameObject의 다른 컴포넌트들이
		// 아직 로드되지 않았을 수 있으므로, 여기서는 컴포넌트를 찾지 않음
		// 컴포넌트는 Awake() 또는 OnGui()에서 찾도록 함
	}
}
