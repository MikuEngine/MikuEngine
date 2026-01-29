#include "GamePCH.h"
#include "PlayerControllerScript.h"

#include "Script/AimPointer.h"
#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/CharacterScript/Player/BulletPlayer.h"

#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Engine/Core/System/Input.h>
#include <Engine/Core/System/MyTime.h>


namespace game
{
	// ═══════════════════════════════════════════════════════════════
	// 생명주기
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::Awake()
	{
		BaseControllerScript::Awake();
	}

	void PlayerControllerScript::Start()
	{
		BaseControllerScript::Start();

		// 애니메이션 초기화 (SkeletalAnimator에 애니메이션 등록)
		InitializeAnimations();

		// LogicFSM 초기화 (한 번만)
		if (!m_fsmInitialized && m_logicFSM)
		{
			InitializeFSM();
			m_fsmInitialized = true;
		}

		// AnimFSM 초기화 (상/하체 분리 상태 매핑)
		if (m_animFSM)
		{
			InitializeAnimFSM();
			
			// Procedural Aim 활성화
			if (m_enableUpperBodyAim)
			{
				m_animFSM->SetProceduralAimEnabled(true);
			}
		}

		// 초기 회전 설정 (-Z 방향 모델이므로 +180도 보정)
		if (GetTransform())
		{
			m_currentRotationAngle = atan2f(m_lastMoveDirection.x, m_lastMoveDirection.z) + 3.14159f;
			
			engine::Quaternion initialRot = engine::Quaternion::CreateFromAxisAngle(
				engine::Vector3::UnitY, 
				m_currentRotationAngle
			);
			
			// Dynamic Rigidbody의 경우 초기 회전도 Rigidbody를 통해 설정
			if (m_rigidbody && m_rigidbody->IsDynamic())
			{
				GetTransform()->SetLocalRotation(initialRot);  // 초기화는 직접 설정 가능
				m_rigidbody->SetAngularVelocity(engine::Vector3::Zero);  // 각속도 초기화
			}
			else
			{
				GetTransform()->SetLocalRotation(initialRot);
			}
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// 컴포넌트 캐싱
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::CacheComponents()
	{
		BaseControllerScript::CacheComponents();

		if (!GetGameObject()) return;

		m_rigidbody = GetGameObject()->GetComponent<engine::Rigidbody>();
		m_skeletalAnimator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();

		// ─────────────────────────────────────────────
		// AimPointer: 씬에서 검색 (UI 팀과 공유 가능)
		// ─────────────────────────────────────────────
		// 먼저 같은 오브젝트에서 찾기 (하위 호환성)
		m_aimPointer = GetGameObject()->GetComponent<AimPointer>();
		if (!m_aimPointer)
		{
			auto* scene = engine::SceneManager::Get().GetScene();
			if (scene)
			{
				// 설정된 이름의 오브젝트에서 검색
				if (auto* aimGO = scene->FindGameObject(m_aimPointerObjectName))
				{
					m_aimPointer = aimGO->GetComponent<AimPointer>();
				}
				// 이름에 "AimPointer"가 포함된 오브젝트 검색 (폴백)
				if (!m_aimPointer)
				{
					for (const auto& go : scene->GetGameObjects())
					{
						if (go && go->GetName().find("AimPointer") != std::string::npos)
						{
							m_aimPointer = go->GetComponent<AimPointer>();
							if (m_aimPointer) break;
						}
					}
				}
			}
		}

		// ─────────────────────────────────────────────
		// BulletFactory: 같은 오브젝트 또는 씬에서 검색
		// ─────────────────────────────────────────────
		m_bulletFactory = GetGameObject()->GetComponent<BulletFactory>();
		//if (!m_bulletFactory)
		//{
		//	auto* scene = engine::SceneManager::Get().GetScene();
		//	if (scene)
		//	{
		//		if (auto* factoryGO = scene->FindGameObject("BulletFactory"))
		//		{
		//			m_bulletFactory = factoryGO->GetComponent<BulletFactory>();
		//		}
		//	}
		//}
		// 참고: m_fireRate, m_bulletSpeed 등 파라미터 값은 
		// 여기서 설정하지 않음. Load()에서 읽어온 값 또는 
		// 헤더의 초기값이 사용됨.
	}

	// ═══════════════════════════════════════════════════════════════
	// 행동 제한 (하이브리드 패턴 핵심)
	// ═══════════════════════════════════════════════════════════════
	bool PlayerControllerScript::CanMove() const
	{
		// 처형 중에는 이동 불가
		if (IsInState("Execution"))
		{
			return false;
		}
		return true;
	}

	bool PlayerControllerScript::CanAttack() const
	{
		// 처형 중에는 공격 불가
		if (IsInState("Execution"))
		{
			return false;
		}
		return true;
	}

	// ═══════════════════════════════════════════════════════════════
	// 입력 처리 (입력 → FSM 파라미터)
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::ProcessInput()
	{
		if (!m_logicFSM) return;

		// Execution 상태에서는 모든 입력 무시
		if (IsInState("Execution"))
		{
			return;
		}

		// ─────────────────────────────────────────────
		// 1. 이동 입력 → FSM 파라미터
		// ─────────────────────────────────────────────
		engine::Vector3 moveDir = GetMoveInputDirection();
		bool isMoving = moveDir.LengthSquared() > 0.0001f;

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
	// 게임 로직 - 비물리 (상태 확인 후 행동 실행)
	// Update()에서 호출됨 (DeltaTime 기반)
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::UpdateGameLogic()
	{
		// Execution 상태에서는 행동 로직 스킵
		if (IsInState("Execution"))
		{
			return;
		}

		float deltaTime = engine::Time::DeltaTime();

		// 비물리 행동 실행 (타이머, 애니메이션 등)
		if (CanAttack())  HandleShooting(deltaTime);

		// 애니메이션 업데이트 (비물리)
		UpdateAnimation();
		// 주의: UpdateUpperBodyAim()은 FixedUpdate(UpdatePhysicsLogic)로 이동됨
	}

	// ═══════════════════════════════════════════════════════════════
	// 물리 로직 - FixedUpdate()에서 호출됨 (FixedDeltaTime 기반)
	// Rigidbody 이동, 회전 등
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::UpdatePhysicsLogic()
	{
		// Execution 상태에서는 물리 로직 스킵 (이동, 회전 모두 막음)
		if (IsInState("Execution"))
		{
			return;
		}

		// 물리 기반 행동 실행
		if (CanMove())    HandleMovement();
		UpdateLowerBodyRotation();  // 매 FixedUpdate 호출 (이동 방향에 따라 회전)
		UpdateUpperBodyAim();       // FixedUpdate로 이동 (물리와 싱크)
	}

	// ═══════════════════════════════════════════════════════════════
	// 상태 변화 콜백
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::OnStateEntered(const std::string& state)
	{
		// 하이브리드 패턴에서는 FSM 상태를 주로 애니메이션 트리거로 사용
		// 필요시 상태별 초기화 로직 추가
		
		if (state == "Execution")
		{
			// 처형 상태 진입 시 이동 정지
			if (m_rigidbody)
			{
				m_rigidbody->SetLinearVelocity(engine::Vector3::Zero);
				m_rigidbody->SetAngularVelocity(engine::Vector3::Zero);
			}
		}
	}

	void PlayerControllerScript::StartExecution(engine::GameObject* targetMonster)
	{
		if (!targetMonster || !m_logicFSM) return;

		// 1. ExecuteMonster 트리거 설정 → Execution 스테이트로 전이
		m_logicFSM->SetTrigger("ExecuteMonster");

		// 2. 몬스터 위치로 플레이어 순간이동
		engine::Transform* monsterTransform = targetMonster->GetTransform();
		if (monsterTransform)
		{
			engine::Vector3 targetPos = monsterTransform->GetWorldPosition();
			
			// Dynamic Rigidbody가 있으면 ForceSetPosition 사용 (물리 엔진 즉시 적용)
			if (m_rigidbody && m_rigidbody->IsDynamic())
			{
				m_rigidbody->ForceSetPosition(targetPos, true);  // 속도도 리셋
			}
			else if (GetTransform())
			{
				// Rigidbody가 없거나 Kinematic인 경우 Transform 직접 설정
				GetTransform()->SetLocalPosition(targetPos);
			}
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// FSM 초기화
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::InitializeFSM()
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
		AddFSMState("Execution");    // 처형 상태

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

		// ─────────────────────────────────────────────
		// 처형 전이 (모든 상태 -> Execution)
		// ─────────────────────────────────────────────
		AddFSMTransition("Idle", "Execution", "ExecuteMonster", Trigger());
		AddFSMTransition("Walk", "Execution", "ExecuteMonster", Trigger());
		AddFSMTransition("IdleShoot", "Execution", "ExecuteMonster", Trigger());
		AddFSMTransition("WalkShoot", "Execution", "ExecuteMonster", Trigger());

		// Execution -> Idle (처형 완료 트리거)
		AddFSMTransition("Execution", "Idle", "ExecutionComplete", Trigger());
	}

	void PlayerControllerScript::InitializeAnimFSM()
	{
		if (!m_animFSM) return;

		// 기존 상태 클리어
		m_animFSM->ClearStates();

		// ─────────────────────────────────────────────
		// 상/하체 분리 애니메이션 상태 등록
		// AddSplitState(상태명, 하체애니, 하체루프, 상체애니, 상체루프, 상체웨이트, 크로스페이드)
		// 상체웨이트가 0이면 상체 레이어 비활성화 (하체가 전체에 적용)
		// 
		// animState에 사용되는 명칭:
		//   - Idle, WalkForward, WalkBackward
		//   - IdleShoot, WalkForwardShoot, WalkBackwardShoot
		// ─────────────────────────────────────────────

		// 비공격 상태: 상체 레이어 비활성화 (하체 애니메이션이 전체에 적용)
		m_animFSM->AddSplitState("Idle",           m_animName_Idle,         true,  "",      false, 0.0f, 0.1f);
		m_animFSM->AddSplitState("WalkForward",    m_animName_WalkForward,  true,  "",      false, 0.0f, 0.1f);
		m_animFSM->AddSplitState("WalkBackward",   m_animName_WalkBackward, true,  "",      false, 0.0f, 0.1f);

		// 공격 상태: 상체 레이어 비활성화 (Fire할 때 직접 Fire 애니메이션 재생)
		// 상체 웨이트 0 → Fire 시 PlayUpperBodyAnimation으로 활성화
		m_animFSM->AddSplitState("IdleShoot",          m_animName_Idle,         true,  "",      false, 0.0f, 0.1f);
		m_animFSM->AddSplitState("WalkForwardShoot",   m_animName_WalkForward,  true,  "",      false, 0.0f, 0.1f);
		m_animFSM->AddSplitState("WalkBackwardShoot",  m_animName_WalkBackward, true,  "",      false, 0.0f, 0.1f);
	}

	void PlayerControllerScript::InitializeAnimations()
	{
		if (!m_skeletalAnimator) return;

		// ─────────────────────────────────────────────
		// SkeletalAnimator에 애니메이션 등록
		// 규격화된 양식:
		//   1. Idle: 대기 애니메이션
		//   2. WalkForward: 전진 애니메이션
		//   3. WalkBackward: 후진 애니메이션
		//   4. Fire: 발사 애니메이션 (상체)
		// ─────────────────────────────────────────────
		
		// 기존 애니메이션 클리어는 하지 않음 (이미 로드된 애니메이션 유지)
		// m_skeletalAnimator->ClearAnimations();
		
		// 애니메이션 등록 (필요한 경우 파일 경로에서 로드)
		// 주의: 실제 구현에서는 SkeletalAnimator의 API에 맞게 수정 필요
		// 예시: m_skeletalAnimator->LoadAnimation(m_animName_Idle, "path/to/idle.anim");
		
		// 현재는 이미 SkeletalAnimator에 애니메이션이 로드되어 있다고 가정하고,
		// 멤버 변수로 설정된 이름을 사용하여 참조만 함
		
		// 필요시 여기서 애니메이션 검증 가능
		// if (!m_skeletalAnimator->HasAnimation(m_animName_Idle))
		// {
		//     LOG_ERROR("Animation '{}' not found in SkeletalAnimator", m_animName_Idle);
		// }
	}

	// ═══════════════════════════════════════════════════════════════
	// 입력 유틸리티
	// ═══════════════════════════════════════════════════════════════
	engine::Vector3 PlayerControllerScript::GetMoveInputDirection() const
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
	// 액션 함수 - 물리 (FixedUpdate에서 호출)
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::HandleMovement()
	{
		// Dynamic Rigidbody 전용 (Kinematic은 KinematicHelperScript 사용)
		if (!m_rigidbody || !m_rigidbody->IsDynamic())
		{
			return;
		}

		engine::Vector3 moveDir = GetMoveInputDirection();

		if (moveDir.LengthSquared() < 0.0001f)
		{
			// 입력 없으면 감속 (AddForce 기반, 충돌 응답 보존)
			ApplyBrakingForce();
			return;
		}

		moveDir.y = 0.0f;
		moveDir.Normalize();
		m_lastMoveDirection = moveDir;

		// AddForce 기반 이동 (충돌 응답 보존)
		ApplyMovementForce(moveDir, m_moveSpeed);
	}

	void PlayerControllerScript::HandleShooting(float deltaTime)
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



				// 발사 애니메이션 재생 (발사할 때마다)
				if (m_animFSM)
				{
					m_animFSM->PlayUpperBodyAnimation(m_animName_Fire, false);
				}

				// 쿨다운 재설정 (단순 대입으로 확실하게)
				m_fireTimer = m_fireRate;
			}
		}

		// 참고: 마우스를 떼도 타이머를 초기화하지 않음
		// 연타로 쿨다운을 우회하는 것을 방지
	}

	void PlayerControllerScript::UpdateUpperBodyAim()
	{
		if (!m_enableUpperBodyAim || !m_animFSM || !m_aimPointer)
		{
			return;
		}

		// ═══════════════════════════════════════════════════════════════
		// 상체 회전 로직 (월드 기준, FixedUpdate에서 호출)
		// 
		// 핵심: 하체 회전량 보정 없이, 매 프레임 트랜스폼 포워드와 에임 방향의
		//       차이를 직접 계산하여 상체 Yaw로 설정
		// → 모든 계산이 같은 시점(FixedUpdate)에서 이루어져 싱크 문제 해결
		// ═══════════════════════════════════════════════════════════════

		// 1. 에임 방향 계산 (월드 기준)
		engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
		engine::Vector3 aimDir = m_aimPointer->GetDirectionFrom(playerPos);
		aimDir.y = 0.0f;
		if (aimDir.LengthSquared() < 0.0001f) return;
		aimDir.Normalize();

		// 2. 현재 트랜스폼 포워드 (하체 방향)
		engine::Vector3 forward = engine::Vector3::UnitZ;
		engine::Quaternion playerRot = GetTransform()->GetWorldRotation();
		forward = engine::Vector3::Transform(forward, playerRot);
		forward.y = 0.0f;
		forward.Normalize();

		// 3. 트랜스폼 포워드와 에임 방향의 각도 차이 계산 (상체 Yaw)
		float dotProduct = forward.Dot(aimDir);
		engine::Vector3 crossProduct = forward.Cross(aimDir);
		float yawRad = atan2f(crossProduct.y, dotProduct);
		float yaw = engine::ToDegree(yawRad);

		// 4. 상체 회전 데드존 (미세한 진동 방지)
		// ±2도 이내면 0으로 처리
		constexpr float UPPER_BODY_DEADZONE = 2.0f;
		if (std::abs(yaw) < UPPER_BODY_DEADZONE)
		{
			yaw = 0.0f;
		}

		// 5. 상체 회전 범위 제한 (±90도)
		// 이 범위를 벗어나면 하체가 회전해야 함 (UpdateLowerBodyRotation에서 처리)
		// 여기서는 범위 내로 클램핑하지 않고, 그대로 전달
		// (하체가 제대로 회전하면 자연스럽게 ±90도 이내가 됨)
		
		m_animFSM->SetUpperBodyYaw(yaw);
	}

	float PlayerControllerScript::CalculateAimYaw() const
	{
		if (!m_aimPointer) return 0.0f;

		engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
		engine::Vector3 aimPos = m_aimPointer->GetWorldPosition();

		engine::Vector3 toAim = aimPos - playerPos;
		toAim.y = 0.0f;

		if (toAim.LengthSquared() < 0.0001f) return 0.0f;

		toAim.Normalize();
		
		//
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

	float PlayerControllerScript::GetForwardAimDirAngle() const
	{
		// ═══════════════════════════════════════════════════════════════
		// 오브젝트 트랜스폼 y축 회전 + 상체 스켈레톤 회전 → degree 반환
		// 스크립트 레벨 함수 사용하지 않고 직접 계산
		// ═══════════════════════════════════════════════════════════════

		// 1. 오브젝트 트랜스폼의 y축 회전 (라디안 → degree)
		float transformYawDeg = engine::ToDegree(m_currentRotationAngle);

		// 2. 상체 스켈레톤 회전 (에임포인터 방향과 하체 방향의 차이) - 직접 계산
		if (!m_aimPointer || !GetTransform())
		{
			return transformYawDeg;
		}

		engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
		engine::Vector3 aimPos = m_aimPointer->GetWorldPosition();

		engine::Vector3 toAim = aimPos - playerPos;
		toAim.y = 0.0f;

		if (toAim.LengthSquared() < 0.0001f)
		{
			return transformYawDeg;
		}

		toAim.Normalize();

		// 하체 forward 방향 (트랜스폼 회전 적용)
		engine::Vector3 forward = engine::Vector3::UnitZ;
		engine::Quaternion playerRot = GetTransform()->GetWorldRotation();
		forward = engine::Vector3::Transform(forward, playerRot);
		forward.y = 0.0f;
		forward.Normalize();

		// 상대 각도 계산 (하체 기준 상체 회전)
		float dotProduct = forward.Dot(toAim);
		engine::Vector3 crossProduct = forward.Cross(toAim);
		float upperBodyYawRad = atan2f(crossProduct.y, dotProduct);
		float upperBodyYawDeg = engine::ToDegree(upperBodyYawRad);

		// 합산하여 반환
		return transformYawDeg + upperBodyYawDeg;
	}

	// ═══════════════════════════════════════════════════════════════
	// 애니메이션 제어
	// ═══════════════════════════════════════════════════════════════
	bool PlayerControllerScript::IsMovingBackward() const
	{
		if (!m_aimPointer) return false;

		engine::Vector3 moveDir = GetMoveInputDirection();
		if (moveDir.LengthSquared() < 0.0001f) return false;

		// 에임포인터 방향 벡터 (플레이어 기준)
		engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
		engine::Vector3 aimDir = m_aimPointer->GetDirectionFrom(playerPos);
		aimDir.y = 0.0f;
		aimDir.Normalize();

		// 이동 방향 벡터
		moveDir.y = 0.0f;
		moveDir.Normalize();

		// 내적을 이용한 각도 판정
		// dot >= 0: 90도 이하 (앞으로 걷기)
		// dot < 0: 90도 초과 (뒤로 걷기)
		float dot = aimDir.Dot(moveDir);
		
		// 90도 초과하면 뒤로 걷기
		return dot < 0.0f;
	}

	std::string PlayerControllerScript::GetAnimationState() const
	{
		bool isMoving = GetMoveInputDirection().LengthSquared() > 0.0001f;
		bool isShooting = engine::Input::IsMouseHeld(engine::Input::Buttons::LEFT);
		bool isBackward = IsMovingBackward();

		if (!isMoving && !isShooting)
			return "Idle";
		if (!isMoving && isShooting)
			return "IdleFire";
		if (isMoving && !isShooting && !isBackward)
			return "MoveForward";
		if (isMoving && !isShooting && isBackward)
			return "MoveBackward";
		if (isMoving && isShooting && !isBackward)
			return "MoveForwardFire";
		if (isMoving && isShooting && isBackward)
			return "MoveBackwardFire";

		return "Idle";
	}

	void PlayerControllerScript::UpdateAnimation()
	{
		if (!m_animFSM) return;

		// ─────────────────────────────────────────────
		// LogicFSM 상태 + 방향 정보 → AnimFSM 상태 결정
		// ─────────────────────────────────────────────
		std::string logicState = m_logicFSM ? m_logicFSM->GetCurrentState() : "Idle";
		bool isMoving = GetMoveInputDirection().LengthSquared() > 0.0001f;
		bool isShooting = (logicState == "IdleShoot" || logicState == "WalkShoot");
		bool isBackward = IsMovingBackward();

		// AnimFSM 상태 결정 (InitializeAnimFSM에서 등록한 상태명과 일치해야 함)
		std::string animState;

		if (isShooting)
		{
			// 공격 상태
			if (!isMoving)
			{
				animState = "IdleShoot";
			}
			else if (isBackward)
			{
				animState = "WalkBackwardShoot";
			}
			else
			{
				animState = "WalkForwardShoot";
			}
		}
		else
		{
			// 비공격 상태
			if (!isMoving)
			{
				animState = "Idle";
			}
			else if (isBackward)
			{
				animState = "WalkBackward";
			}
			else
			{
				animState = "WalkForward";
			}
		}

		// AnimFSM에 상태 전달 (AnimFSM이 PlayStateAnimation 호출)
		m_animFSM->SetAnimState(animState);
	}

	void PlayerControllerScript::UpdateLowerBodyRotation()
	{
		// Dynamic Rigidbody 전용 (Kinematic은 KinematicHelperScript 사용)
		if (!GetGameObject() || !GetTransform() || !m_aimPointer) return;
		if (!m_rigidbody || !m_rigidbody->IsDynamic()) return;

		// ═══════════════════════════════════════════════════════════════
		// 키보드 이동방향 기반 회전
		// 
		// 로직:
		//   1. 트랜스폼은 키보드 이동방향으로 빠르게 회전 (ForwardAimDir과 무관)
		//   2. 단, ForwardAimDir과 이동방향이 90도 이상 차이나면
		//      → 뒷걸음질 상태 → 트랜스폼을 180도 뒤집어서 회전
		// ═══════════════════════════════════════════════════════════════

		// 1. ForwardAimDir (에임포인터 방향) 계산
		engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
		engine::Vector3 aimDir = m_aimPointer->GetDirectionFrom(playerPos);
		aimDir.y = 0.0f;
		if (aimDir.LengthSquared() < 0.0001f) return;
		aimDir.Normalize();

		// 2. 키보드 이동 방향 확인
		engine::Vector3 moveDir = GetMoveInputDirection();
		bool isMoving = moveDir.LengthSquared() > 0.0001f;

		engine::Vector3 targetDir;

		if (isMoving)
		{
			// ─────────────────────────────────────────────
			// 이동 중: 연속 방향 기반 회전
			// 
			// 로직:
			//   1. 입력 벡터를 그대로 이동 방향으로 사용 
			//   2. 이동 방향과 에임 방향의 각도 차이 계산
			//   3. 90도 이하: 이동 방향 = 트랜스폼 포워드 (WalkForward)
			//   4. 90도 초과: 이동 방향의 반대 = 트랜스폼 포워드 (WalkBackward)
			//   5. 에임 회전방향으로 하체 회전
			// ─────────────────────────────────────────────
			
			// 이동 시작 시 정지 상태 목표 리셋
			m_idleTargetValid = false;
			
			moveDir.y = 0.0f;
			moveDir.Normalize();

			// 이동 방향과 에임 방향의 각도 차이 확인
			float dotMoveAim = moveDir.x * aimDir.x + moveDir.z * aimDir.z;
			
			// 90도 이하 (dot >= 0): 이동 방향 = 트랜스폼 포워드 (앞으로 걷기)
			// 90도 초과 (dot < 0): 이동 방향의 반대 = 트랜스폼 포워드 (뒤로 걷기)
			if (dotMoveAim >= 0.0f)
			{
				// 90도 이하: 이동 방향이 트랜스폼 포워드
				targetDir = moveDir;
			}
			else
			{
				// 90도 초과: 이동 방향의 반대가 트랜스폼 포워드
				targetDir = -moveDir;
			}

			// ─────────────────────────────────────────────
			// 90도 이상 회전 필요 시 추가 검증 (Angular Velocity 방식 전용)
			// 에임이 급격히 바뀌었으면 (캐릭터가 에임을 지나칠 때) 
			// 이번 프레임 회전 스킵 (빙글 도는 현상 방지)
			// ForceSetRotation 방식에서는 관성이 없으므로 이 검증 불필요
			// ─────────────────────────────────────────────
			if (!m_useForceSetRotation)
			{
				engine::Vector3 currentDir(
					sinf(m_currentRotationAngle),
					0.0f,
					cosf(m_currentRotationAngle)
				);
				float dotCurrentTarget = currentDir.x * targetDir.x + currentDir.z * targetDir.z;
				
				// 90도 이상 회전이 필요한 경우 (dot < 0)
				if (dotCurrentTarget < 0.0f)
				{
					// 에임 방향 변화량 체크 (이전 프레임 대비)
					float dotAimChange = m_prevAimDirection.x * aimDir.x + m_prevAimDirection.z * aimDir.z;
					
					// 에임이 급격히 바뀌었으면 (60도 이상 변화, dot < 0.5) 이번 프레임 스킵
					constexpr float RAPID_AIM_CHANGE_THRESHOLD = 0.5f;  // ~60도
					if (dotAimChange < RAPID_AIM_CHANGE_THRESHOLD)
					{
						// 에임 변화가 급격함 → 이번 프레임은 회전 안 함
						m_rigidbody->SetAngularVelocity(engine::Vector3::Zero);
						// 에임 방향만 업데이트하고 리턴
						m_prevAimDirection = aimDir;
						return;
					}
				}
			}

			// 에임 회전방향 추적 (하체 회전방향 결정용)
			UpdateAimTracking();
		}
		else
		{
			// ─────────────────────────────────────────────
			// 정지 상태:
			// - 트랜스폼 포워드는 연속된 값을 가질 수 있음
			// - 에임포워드와 트랜스폼 포워드의 각도 차이가 90도 초과 시:
			//   → 목표 방향을 업데이트 (현재 기준 ±90도)
			// - 90도 이하: 기존 목표 방향 유지
			// - 목표 방향으로 계속 회전
			// ─────────────────────────────────────────────
			
			// 현재 트랜스폼 포워드
			engine::Vector3 currentDir(
				sinf(m_currentRotationAngle),
				0.0f,
				cosf(m_currentRotationAngle)
			);
			
			// 에임방향과 트랜스폼 포워드의 각도 차이 확인
			float dotAimCurrent = aimDir.x * currentDir.x + aimDir.z * currentDir.z;
			float angleToAimRad = acosf(std::clamp(dotAimCurrent, -1.0f, 1.0f));
			float angleToAimDeg = engine::ToDegree(angleToAimRad);
			
			// 90도 초과 시에만 새 목표 방향 계산
			constexpr float IDLE_ROTATION_THRESHOLD = 90.0f;
			if (angleToAimDeg > IDLE_ROTATION_THRESHOLD)
			{
				// 현재 트랜스폼 기준으로 에임이 왼쪽인지 오른쪽인지 확인
				float crossY = currentDir.z * aimDir.x - currentDir.x * aimDir.z;
				
				// 현재 트랜스폼의 90도 회전 방향 계산
				if (crossY > 0.0f)
				{
					// 에임이 오른쪽 → 시계방향 90도 회전
					m_idleTargetDir = engine::Vector3(
						currentDir.z,   // cos(θ-90) = sin(θ)
						0.0f,
						-currentDir.x   // sin(θ-90) = -cos(θ)
					);
				}
				else
				{
					// 에임이 왼쪽 → 반시계방향 90도 회전
					m_idleTargetDir = engine::Vector3(
						-currentDir.z,  // cos(θ+90) = -sin(θ)
						0.0f,
						currentDir.x    // sin(θ+90) = cos(θ)
					);
				}
				m_idleTargetValid = true;
			}
			
			// 목표가 유효하고, 현재 목표에 도달하지 않았으면 계속 회전
			if (m_idleTargetValid)
			{
				float dotToIdleTarget = currentDir.x * m_idleTargetDir.x + currentDir.z * m_idleTargetDir.z;
				
				// 목표 도달 확인 (0.999 ≈ 2.5도)
				if (dotToIdleTarget > 0.999f)
				{
					// 목표 도달 → 정지
					if (!m_useForceSetRotation)
					{
						m_rigidbody->SetAngularVelocity(engine::Vector3::Zero);
					}
					m_idleTargetValid = false;  // 목표 완료
					return;
				}
				
				// 목표 방향으로 회전
				targetDir = m_idleTargetDir;
			}
			else
			{
				// 목표 없음 → 회전 안함
				if (!m_useForceSetRotation)
				{
					m_rigidbody->SetAngularVelocity(engine::Vector3::Zero);
				}
				return;
			}
		}

		// 5. 현재 캐릭터 방향 벡터
		engine::Vector3 currentDir(
			sinf(m_currentRotationAngle),
			0.0f,
			cosf(m_currentRotationAngle)
		);

		// 6. 목표 도달 확인 (내적으로)
		float dotToTarget = currentDir.x * targetDir.x + currentDir.z * targetDir.z;

		// 데드존: 거의 같은 방향이면 회전 정지 (0.999 ≈ 2.5도)
		if (dotToTarget > 0.999f)
		{
			if (!m_useForceSetRotation)
			{
				m_rigidbody->SetAngularVelocity(engine::Vector3::Zero);
			}
			return;
		}

		// 7. 회전 방향 결정 (최단 경로)
		float targetCross = currentDir.z * targetDir.x - currentDir.x * targetDir.z;
		// DirectX 왼손좌표계에서 +가 시계방향
		float rotationSign = (targetCross > 0.0f) ? 1.0f : -1.0f;

		// 8. 회전 적용
		if (m_useForceSetRotation)
		{
			// ═══════════════════════════════════════════════════════════════
			// ForceSetRotation 기반 (직접 트랜스폼 제어)
			// - 물리 엔진의 각속도/관성 무시
			// - 정확하고 즉각적인 회전
			// ═══════════════════════════════════════════════════════════════
			
			float fixedDelta = engine::Time::FixedDeltaTime();
			float rotationStep = rotationSign * m_rotationSpeed * fixedDelta;
			
			// 목표까지 남은 각도 계산 (라디안)
			float angleToTarget = acosf(std::clamp(dotToTarget, -1.0f, 1.0f));
			
			// 오버슈트 방지: 남은 각도보다 회전량이 크면 목표에 정확히 맞춤
			if (std::abs(rotationStep) > angleToTarget)
			{
				// 목표 방향으로 직접 설정
				m_currentRotationAngle = atan2f(targetDir.x, targetDir.z);
			}
			else
			{
				m_currentRotationAngle += rotationStep;
			}
			
			// 각도 정규화 (-π ~ π)
			while (m_currentRotationAngle > 3.14159f) m_currentRotationAngle -= 6.28318f;
			while (m_currentRotationAngle < -3.14159f) m_currentRotationAngle += 6.28318f;
			
			// 회전 적용
			engine::Quaternion newRot = engine::Quaternion::CreateFromAxisAngle(
				engine::Vector3::UnitY,
				m_currentRotationAngle
			);
			m_rigidbody->ForceSetRotation(newRot, true);  // 각속도 리셋
		}
		else
		{
			// ═══════════════════════════════════════════════════════════════
			// Angular Velocity 기반 (물리 기반)
			// - 권장 설정: m_rotationSpeed = 12.0f, AngularDamping = 10
			// - 물리적 관성/감쇠 적용
			// ═══════════════════════════════════════════════════════════════
			
			// 에임 회전방향 우선, 없으면 최단 경로
			float aimCross = GetAimRotationDirection();
			constexpr float AIM_MOVE_THRESHOLD = 0.001f;

			if (isMoving && std::abs(aimCross) > AIM_MOVE_THRESHOLD)
			{
				// 이동 중 + 에임이 움직이면: 에임 회전방향 따라감
				// PhysX(오른손좌표계)와 DirectX(왼손좌표계) 차이로 부호 반전
				rotationSign = (aimCross > 0.0f) ? -1.0f : 1.0f;
			}
			else
			{
				// 정지 또는 에임 멈춤: 최단 경로
				// PhysX(오른손좌표계)와 DirectX(왼손좌표계) 차이로 부호 반전
				rotationSign = (targetCross > 0.0f) ? -1.0f : 1.0f;
			}

			// 목표까지 남은 각도 계산 (라디안)
			float angleToTarget = acosf(std::clamp(dotToTarget, -1.0f, 1.0f));
			
			// 감쇠: 목표 근처에서 속도 줄이기
			constexpr float SMOOTH_THRESHOLD = 0.6f;
			constexpr float MIN_SPEED_SCALE = 0.05f;
			
			float speedScale = 1.0f;
			if (angleToTarget < SMOOTH_THRESHOLD)
			{
				speedScale = angleToTarget / SMOOTH_THRESHOLD;  // 0~1 범위
				speedScale = std::max(speedScale, MIN_SPEED_SCALE);
			}
			
			float angularVelocity = rotationSign * m_rotationSpeed * speedScale;
			m_rigidbody->SetAngularVelocity(engine::Vector3(0.0f, angularVelocity, 0.0f));

			// 각도 동기화
			engine::Quaternion currentRot = GetTransform()->GetWorldRotation();
			engine::Vector3 euler = currentRot.ToEuler();
			m_currentRotationAngle = euler.y;
		}

	
	}

	// ═══════════════════════════════════════════════════════════════
	// 에임 추적 유틸리티
	// ═══════════════════════════════════════════════════════════════
	
	void PlayerControllerScript::UpdateAimTracking()
	{
		if (!m_aimPointer || !GetTransform()) return;
		
		// ═══════════════════════════════════════════════════════════════
		// 벡터 기반 에임 추적 (각도 래핑 문제 회피)
		// 외적(Cross Product)을 사용하여 회전 방향 결정
		// 스무딩 없이 즉시 반응
		// ═══════════════════════════════════════════════════════════════
		
		engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
		engine::Vector3 currentAimDir = m_aimPointer->GetDirectionFrom(playerPos);
		currentAimDir.y = 0.0f;
		
		if (currentAimDir.LengthSquared() < 0.001f) return;
		currentAimDir.Normalize();
		
		if (!m_aimTrackingInitialized)
		{
			m_prevAimDirection = currentAimDir;
			m_aimCrossProductSmoothed = 0.0f;
			m_aimTrackingInitialized = true;
			return;
		}
		
		// ─────────────────────────────────────────────
		// 외적으로 회전 방향 계산 (Y 성분만 사용)
		// 정확한 외적 공식: (prev × current).y = prev.z * curr.x - prev.x * curr.z
		//   양수 = current가 prev의 시계 방향 (오른쪽으로 회전)
		//   음수 = current가 prev의 반시계 방향 (왼쪽으로 회전)
		// ─────────────────────────────────────────────
		float crossY = m_prevAimDirection.z * currentAimDir.x - m_prevAimDirection.x * currentAimDir.z;
		
		// 스무딩 없이 직접 저장 (데드존은 사용처에서 처리)
		m_aimCrossProductSmoothed = crossY;
		
		m_prevAimDirection = currentAimDir;
	}

	float PlayerControllerScript::GetAimRotationDirection() const
	{
		// 양수: 에임이 시계 방향으로 이동 중 (CW, 오른쪽, 각도 증가)
		// 음수: 에임이 반시계 방향으로 이동 중 (CCW, 왼쪽, 각도 감소)
		return m_aimCrossProductSmoothed;
	}

	// ═══════════════════════════════════════════════════════════════
	// 에디터 검증
	// ═══════════════════════════════════════════════════════════════
	bool PlayerControllerScript::ValidateComponents() const
	{
		bool isValid = true;

		if (!m_rigidbody)
		{
			isValid = false;
		}
		if (!m_skeletalAnimator)
		{
			isValid = false;
		}
		if (!m_aimPointer)
		{
			isValid = false;
		}
		if (!m_bulletFactory)
		{
			isValid = false;
		}
		if (!m_animFSM)
		{
			isValid = false;
		}
		if (!m_logicFSM)
		{
			isValid = false;
		}

		return isValid;
	}

	// ═══════════════════════════════════════════════════════════════
	// GUI / 직렬화
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::OnGui()
	{
		ImGui::Indent();
		
		ImGui::Text("PlayerControllerScript:");

		// ─────────────────────────────────────────────
		// 컴포넌트 검증 (에디터 화면에서도 체크)
		// ─────────────────────────────────────────────
		ImGui::Separator();
		ImGui::Text("=== Component Validation ===");
		
		// 에디터 모드를 위한 실시간 컴포넌트 검색 (같은 GameObject 내에서만 검색)
		engine::Rigidbody* rigidbody = m_rigidbody ? m_rigidbody : (GetGameObject() ? GetGameObject()->GetComponent<engine::Rigidbody>() : nullptr);
		engine::SkeletalAnimator* skeletalAnimator = m_skeletalAnimator ? m_skeletalAnimator : (GetGameObject() ? GetGameObject()->GetComponent<engine::SkeletalAnimator>() : nullptr);
		engine::AnimFSM* animFSM = m_animFSM ? m_animFSM : (GetGameObject() ? GetGameObject()->GetComponent<engine::AnimFSM>() : nullptr);
		engine::LogicFSM* logicFSM = m_logicFSM ? m_logicFSM : (GetGameObject() ? GetGameObject()->GetComponent<engine::LogicFSM>() : nullptr);
		AimPointer* aimPointer = m_aimPointer ? m_aimPointer : (GetGameObject() ? GetGameObject()->GetComponent<AimPointer>() : nullptr);
		BulletFactory* bulletFactory = m_bulletFactory ? m_bulletFactory : (GetGameObject() ? GetGameObject()->GetComponent<BulletFactory>() : nullptr);
		
		// 전체 유효성 검사
		bool allValid = rigidbody && skeletalAnimator && aimPointer && bulletFactory && animFSM && logicFSM;
		
		if (allValid)
		{
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "[OK] All components are valid!");
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "[ERROR] Some components are missing!");
		}

		// 개별 컴포넌트 상태 표시
		ImGui::Indent();
		ImGui::Text("Rigidbody:         %s", rigidbody ? "[OK]" : "[MISSING]");
		if (!rigidbody) ImGui::SameLine(); if (!rigidbody) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");
		
		ImGui::Text("SkeletalAnimator:  %s", skeletalAnimator ? "[OK]" : "[MISSING]");
		if (!skeletalAnimator) ImGui::SameLine(); if (!skeletalAnimator) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");
		
		ImGui::Text("AimPointer:        %s", aimPointer ? "[OK]" : "[MISSING]");
		if (!aimPointer) ImGui::SameLine(); if (!aimPointer) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");
		
		ImGui::Text("BulletFactory:     %s", bulletFactory ? "[OK]" : "[MISSING]");
		if (!bulletFactory) ImGui::SameLine(); if (!bulletFactory) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");
		
		ImGui::Text("AnimFSM:           %s", animFSM ? "[OK]" : "[MISSING]");
		if (!animFSM) ImGui::SameLine(); if (!animFSM) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");
		
		ImGui::Text("LogicFSM:          %s", logicFSM ? "[OK]" : "[MISSING]");
		if (!logicFSM) ImGui::SameLine(); if (!logicFSM) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");
		ImGui::Unindent();

		// 이동
		ImGui::Separator();
		ImGui::Text("Movement:");
		ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("Rotation Speed (rad/s)", &m_rotationSpeed, 0.5f, 1.0f, 30.0f);

		// 발사 설정
		ImGui::Separator();
		ImGui::Text("Shooting:");
		ImGui::DragFloat("Fire Rate (sec)", &m_fireRate, 0.2f, 0.01f, 2.0f);
		ImGui::DragFloat("Bullet Speed", &m_bulletSpeed, 1.0f, 1.0f, 100.0f);
		ImGui::DragFloat("Bullet Lifetime", &m_bulletLifetime, 3.0f, 0.5f, 10.0f);

		// 처형 설정
		ImGui::Separator();
		ImGui::Text("Execution:");
		ImGui::DragFloat("Execution Range", &m_executionRange, 0.5f, 1.0f, 50.0f);

		// 애니메이션 설정
		ImGui::Separator();
		ImGui::Text("Animation Names:");
		ImGui::InputText("Idle", &m_animName_Idle);
		ImGui::InputText("WalkForward", &m_animName_WalkForward);
		ImGui::InputText("WalkBackward", &m_animName_WalkBackward);
		ImGui::InputText("Fire", &m_animName_Fire);

		// 참조 설정
		ImGui::Separator();
		ImGui::Text("References:");
		ImGui::InputText("AimPointer Object", &m_aimPointerObjectName);
		
		// 기타
		ImGui::Separator();
		ImGui::Checkbox("Enable Upper Body Aim", &m_enableUpperBodyAim);

		ImGui::Separator();
		ImGui::Text("Runtime Info:");
		ImGui::Text("Animation State: %s", GetAnimationState().c_str());
		ImGui::Text("Is Moving Backward: %s", IsMovingBackward() ? "Yes" : "No");
		ImGui::Text("Forward Aim Dir Angle: %.2f deg", GetForwardAimDirAngle());

		if (m_aimPointer)
		{
			ImGui::Separator();
			ImGui::Text("=== Rotation Debug ===");
			
			// 에임 외적 (회전 방향 결정에 사용)
			float aimCross = GetAimRotationDirection();
			
			// 큰 글씨로 현재 상태 표시
			if (std::abs(aimCross) > 0.001f)
			{
				if (aimCross > 0.0f)
					ImGui::TextColored(ImVec4(0,1,0,1), ">>> CW (시계방향) >>>");
				else
					ImGui::TextColored(ImVec4(1,1,0,1), "<<< CCW (반시계) <<<");
			}
			else
			{
				ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1), "--- NEUTRAL ---");
			}
			
			ImGui::Text("Aim Cross: %.4f", aimCross);
		}
		
		ImGui::Unindent();
		
		// BaseControllerScript의 OnGui 호출 (이동 물리 설정)
		BaseControllerScript::OnGui();
		
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
		ImGui::Separator();
		ImGui::PopStyleColor();
		ImGui::Spacing();
		ImGui::Spacing();
	}

	void PlayerControllerScript::Save(engine::json& j) const
	{
		BaseControllerScript::Save(j);
		j["MoveSpeed"] = m_moveSpeed;
		j["RotationSpeed"] = m_rotationSpeed;
		j["FireRate"] = m_fireRate;
		j["BulletSpeed"] = m_bulletSpeed;
		j["BulletLifetime"] = m_bulletLifetime;
		j["AimPointerObjectName"] = m_aimPointerObjectName;
		j["EnableUpperBodyAim"] = m_enableUpperBodyAim;
		j["FSMInitialized"] = m_fsmInitialized;
		
		// 애니메이션 이름 저장
		j["AnimName_Idle"] = m_animName_Idle;
		j["AnimName_WalkForward"] = m_animName_WalkForward;
		j["AnimName_WalkBackward"] = m_animName_WalkBackward;
		j["AnimName_Fire"] = m_animName_Fire;
		
		// 처형 설정
		j["ExecutionRange"] = m_executionRange;
	}

	void PlayerControllerScript::Load(const engine::json& j)
	{
		BaseControllerScript::Load(j);

		if (j.contains("MoveSpeed"))
			m_moveSpeed = j["MoveSpeed"].get<float>();
		if (j.contains("RotationSpeed"))
			m_rotationSpeed = j["RotationSpeed"].get<float>();
		if (j.contains("FireRate"))
			m_fireRate = j["FireRate"].get<float>();
		if (j.contains("BulletSpeed"))
			m_bulletSpeed = j["BulletSpeed"].get<float>();
		if (j.contains("BulletLifetime"))
			m_bulletLifetime = j["BulletLifetime"].get<float>();
		if (j.contains("AimPointerObjectName"))
			m_aimPointerObjectName = j["AimPointerObjectName"].get<std::string>();
		if (j.contains("EnableUpperBodyAim"))
			m_enableUpperBodyAim = j["EnableUpperBodyAim"].get<bool>();
		if (j.contains("FSMInitialized"))
			m_fsmInitialized = j["FSMInitialized"].get<bool>();
		
		// 애니메이션 이름 로드
		if (j.contains("AnimName_Idle"))
			m_animName_Idle = j["AnimName_Idle"].get<std::string>();
		if (j.contains("AnimName_WalkForward"))
			m_animName_WalkForward = j["AnimName_WalkForward"].get<std::string>();
		if (j.contains("AnimName_WalkBackward"))
			m_animName_WalkBackward = j["AnimName_WalkBackward"].get<std::string>();
		if (j.contains("AnimName_Fire"))
			m_animName_Fire = j["AnimName_Fire"].get<std::string>();
		
		// 처형 설정
		if (j.contains("ExecutionRange"))
			m_executionRange = j["ExecutionRange"].get<float>();
	}
}
