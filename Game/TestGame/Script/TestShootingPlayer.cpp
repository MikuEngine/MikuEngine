#include "GamePCH.h"
#include "TestShootingPlayer.h"

#include "AimPointer.h"
#include "BulletFactory.h"

#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Engine/Core/System/Input.h>
#include <Engine/Core/System/MyTime.h>

namespace game
{
	// ═══════════════════════════════════════════════════════════════
	// 생명주기
	// ═══════════════════════════════════════════════════════════════
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
		BaseControllerScript::CacheComponents();

		if (!GetGameObject()) return;

		m_rigidbody = GetGameObject()->GetComponent<engine::Rigidbody>();
		m_aimPointer = GetGameObject()->GetComponent<AimPointer>();

		// BulletFactory: 같은 오브젝트 또는 씬에서 검색
		m_bulletFactory = GetGameObject()->GetComponent<BulletFactory>();
		if (!m_bulletFactory)
		{
			auto* scene = engine::SceneManager::Get().GetScene();
			if (scene)
			{
				if (auto* factoryGO = scene->FindGameObject("BulletFactory"))
				{
					m_bulletFactory = factoryGO->GetComponent<BulletFactory>();
				}
			}
		}
		// 참고: m_fireRate, m_bulletSpeed 등 파라미터 값은 
		// 여기서 설정하지 않음. Load()에서 읽어온 값 또는 
		// 헤더의 초기값이 사용됨.
	}

	// ═══════════════════════════════════════════════════════════════
	// 행동 제한 (하이브리드 패턴 핵심)
	// ═══════════════════════════════════════════════════════════════
	bool TestShootingPlayer::CanMove() const
	{
		// 현재 구현: 모든 상태에서 이동 가능
		// 필요시 특정 상태에서 이동 제한 가능
		// 예: return !IsInAnyState({"Stunned", "Dead", "Casting"});
		return true;
	}

	bool TestShootingPlayer::CanAttack() const
	{
		// 현재 구현: 모든 상태에서 공격 가능
		// 필요시 특정 상태에서 공격 제한 가능
		// 예: return !IsInAnyState({"Stunned", "Dead", "Reloading"});
		return true;
	}

	// ═══════════════════════════════════════════════════════════════
	// 입력 처리 (입력 → FSM 파라미터)
	// ═══════════════════════════════════════════════════════════════
	void TestShootingPlayer::ProcessInput()
	{
		if (!m_logicFSM) return;

		// ─────────────────────────────────────────────
		// 1. 이동 입력 → FSM 파라미터
		// ─────────────────────────────────────────────
		engine::Vector3 moveDir = GetMoveInputDirection();
		bool isMoving = moveDir.LengthSquared() > 0.001f;

		m_logicFSM->SetParameter("IsMoving", isMoving);
		if (isMoving)
		{
			m_logicFSM->SetParameter("MoveX", moveDir.x);
			m_logicFSM->SetParameter("MoveZ", moveDir.z);
		}

		// ─────────────────────────────────────────────
		// 2. 공격 입력 → FSM 파라미터
		// ─────────────────────────────────────────────
		bool isMousePressed = engine::Input::IsMousePressed(engine::Input::Buttons::LEFT);
		bool isMouseHeld = engine::Input::IsMouseHeld(engine::Input::Buttons::LEFT);

		// Pressed → 트리거 (상태 전이용)
		if (isMousePressed)
		{
			m_logicFSM->SetTrigger("Attack");
		}

		// Held → 파라미터 (상태 유지용)
		m_logicFSM->SetParameter("IsShooting", isMouseHeld);
	}

	// ═══════════════════════════════════════════════════════════════
	// 게임 로직 (상태 확인 후 행동 실행)
	// ═══════════════════════════════════════════════════════════════
	void TestShootingPlayer::UpdateGameLogic()
	{
		float deltaTime = engine::Time::DeltaTime();

		// 행동 실행 (제한 함수로 허용 여부 확인)
		if (CanMove())    HandleMovement(deltaTime);
		if (CanAttack())  HandleShooting(deltaTime);
	}

	// ═══════════════════════════════════════════════════════════════
	// 상태 변화 콜백
	// ═══════════════════════════════════════════════════════════════
	void TestShootingPlayer::OnStateEntered(const std::string& state)
	{
		// 하이브리드 패턴에서는 FSM 상태를 주로 애니메이션 트리거로 사용
		// 필요시 상태별 초기화 로직 추가
	}

	// ═══════════════════════════════════════════════════════════════
	// FSM 초기화
	// ═══════════════════════════════════════════════════════════════
	void TestShootingPlayer::InitializeFSM()
	{
		if (!m_logicFSM || m_fsmInitialized) return;

		m_logicFSM->ClearStates();

		// ─────────────────────────────────────────────
		// 상태 정의
		// ─────────────────────────────────────────────
		AddFSMState("Idle", true);   // 기본 상태
		AddFSMState("Walk");
		AddFSMState("IdleShoot");
		AddFSMState("WalkShoot");

		m_logicFSM->UpdateStateMap();
		m_logicFSM->SetDefaultState("Idle");
		m_logicFSM->InitializeCurrentState();

		// ─────────────────────────────────────────────
		// 전이 정의
		// ─────────────────────────────────────────────
		// Idle <-> Walk (이동 입력)
		AddFSMTransition("Idle", "Walk", "IsMoving", BoolTrue());
		AddFSMTransition("Walk", "Idle", "IsMoving", BoolFalse());

		// Idle/Walk -> Shoot (공격 트리거)
		AddFSMTransition("Idle", "IdleShoot", "Attack", Trigger());
		AddFSMTransition("Walk", "WalkShoot", "Attack", Trigger());

		// IdleShoot <-> WalkShoot (이동 입력)
		AddFSMTransition("IdleShoot", "WalkShoot", "IsMoving", BoolTrue());
		AddFSMTransition("WalkShoot", "IdleShoot", "IsMoving", BoolFalse());

		// Shoot -> 비Shoot (마우스 Released)
		AddFSMTransition("IdleShoot", "Idle", "IsShooting", BoolFalse());
		AddFSMTransition("WalkShoot", "Walk", "IsShooting", BoolFalse());
	}

	// ═══════════════════════════════════════════════════════════════
	// 입력 유틸리티
	// ═══════════════════════════════════════════════════════════════
	engine::Vector3 TestShootingPlayer::GetMoveInputDirection() const
	{
		engine::Vector3 dir = engine::Vector3::Zero;

		if (engine::Input::IsKeyHeld(engine::Keys::W)) dir.z += 1.0f;
		if (engine::Input::IsKeyHeld(engine::Keys::S)) dir.z -= 1.0f;
		if (engine::Input::IsKeyHeld(engine::Keys::A)) dir.x -= 1.0f;
		if (engine::Input::IsKeyHeld(engine::Keys::D)) dir.x += 1.0f;

		if (dir.LengthSquared() > 0.0f)
		{
			dir.Normalize();
		}

		return dir;
	}

	// ═══════════════════════════════════════════════════════════════
	// 액션 함수
	// ═══════════════════════════════════════════════════════════════
	void TestShootingPlayer::HandleMovement(float deltaTime)
	{
		engine::Vector3 moveDir = GetMoveInputDirection();
		if (moveDir.LengthSquared() < 0.001f) return;

		engine::Transform* transform = GetGameObject()->GetTransform();
		if (!transform) return;

		engine::Vector3 currentPos = transform->GetLocalPosition();
		engine::Vector3 newPos = currentPos + moveDir * m_moveSpeed * deltaTime;
		transform->SetLocalPosition(newPos);
	}

	void TestShootingPlayer::HandleShooting(float deltaTime)
	{
		// ─────────────────────────────────────────────
		// 쿨다운 타이머 감소 (항상 실행)
		// ─────────────────────────────────────────────
		if (m_fireTimer > 0.0f)
		{
			m_fireTimer -= deltaTime;
		}

		// ─────────────────────────────────────────────
		// 발사 조건: 마우스 누름 + 쿨다운 완료
		// ─────────────────────────────────────────────
		bool isMouseHeld = engine::Input::IsMouseHeld(engine::Input::Buttons::LEFT);

		if (isMouseHeld && m_fireTimer <= 0.0f)
		{
			// 발사!
			if (m_bulletFactory && m_aimPointer)
			{
				engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
				engine::Vector3 direction = m_aimPointer->GetDirectionFrom(playerPos);

				// BulletParams로 발사
				BulletParams params;
				params.type = BulletType::Linear;
				params.speed = m_bulletSpeed;
				params.lifetime = m_bulletLifetime;
				params.damage = 10;  // TODO: 멤버 변수로 관리

				m_bulletFactory->Fire(playerPos, direction, params);

				// 쿨다운 재설정 (단순 대입으로 확실하게)
				m_fireTimer = m_fireRate;
			}
		}

		// 참고: 마우스를 떼도 타이머를 초기화하지 않음
		// 연타로 쿨다운을 우회하는 것을 방지
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
		if (!m_aimPointer) return 0.0f;

		engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
		engine::Vector3 aimPos = m_aimPointer->GetTransform()->GetWorldPosition();

		engine::Vector3 toAim = aimPos - playerPos;
		toAim.y = 0.0f;

		if (toAim.LengthSquared() < 0.001f) return 0.0f;

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

		// 이동
		ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.1f, 0.0f, 100.0f);

		// 발사 설정
		ImGui::Separator();
		ImGui::Text("Shooting:");
		ImGui::DragFloat("Fire Rate (sec)", &m_fireRate, 0.2, 0.01f, 2.0f);
		ImGui::DragFloat("Bullet Speed", &m_bulletSpeed, 1.0f, 1.0f, 100.0f);
		ImGui::DragFloat("Bullet Lifetime", &m_bulletLifetime, 3.0, 0.5f, 10.0f);

		// 기타
		ImGui::Separator();
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
		j["FireRate"] = m_fireRate;
		j["BulletSpeed"] = m_bulletSpeed;
		j["BulletLifetime"] = m_bulletLifetime;
		j["EnableUpperBodyAim"] = m_enableUpperBodyAim;
		j["FSMInitialized"] = m_fsmInitialized;
	}

	void TestShootingPlayer::Load(const engine::json& j)
	{
		BaseControllerScript::Load(j);

		if (j.contains("MoveSpeed"))
			m_moveSpeed = j["MoveSpeed"].get<float>();
		if (j.contains("FireRate"))
			m_fireRate = j["FireRate"].get<float>();
		if (j.contains("BulletSpeed"))
			m_bulletSpeed = j["BulletSpeed"].get<float>();
		if (j.contains("BulletLifetime"))
			m_bulletLifetime = j["BulletLifetime"].get<float>();
		if (j.contains("EnableUpperBodyAim"))
			m_enableUpperBodyAim = j["EnableUpperBodyAim"].get<bool>();
		if (j.contains("FSMInitialized"))
			m_fsmInitialized = j["FSMInitialized"].get<bool>();
	}
}
