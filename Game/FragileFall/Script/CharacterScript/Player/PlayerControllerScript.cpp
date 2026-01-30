#include "GamePCH.h"
#include "PlayerControllerScript.h"

#include "Script/AimPointer.h"
#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/CharacterScript/Player/BulletPlayer.h"

#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/System/SystemManager.h>
#include <Framework/Physics/PhysicsSystem.h>
#include <Framework/Physics/PhysicsLayer.h>
#include <Framework/Physics/CollisionTypes.h>
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

		// Dynamic Rigidbody의 경우의 초기화 추가
		if (m_rigidbody && m_rigidbody->IsDynamic())
		{
			m_rigidbody->SetAngularVelocity(engine::Vector3::Zero);  // 각속도 초기화
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
		// BulletFactory: 같은 오브젝트에서 검색
		// ─────────────────────────────────────────────
		m_bulletFactory = GetGameObject()->GetComponent<BulletFactory>();

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
		// 대쉬 중에는 일반 이동 불가 (대쉬 방향으로만 이동)
		if (IsInState("Dash"))
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
		// 대쉬 중에는 공격 불가
		if (IsInState("Dash"))
		{
			return false;
		}
		return true;
	}

	void PlayerControllerScript::CheckForwardBack(engine::Vector3& forward, engine::Vector3& aimDir)
	{
		engine::Vector3 fo = forward;
		engine::Vector3 aim = aimDir;

		float dot = fo.Dot(aim);
		m_isBackward = dot < 0.01f ? true : false;
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

		// Dash 상태에서는 입력 무시 (대쉬 방향 고정)
		if (IsInState("Dash"))
		{
			return;
		}

		// ─────────────────────────────────────────────
		// 1. 이동 입력 → FSM 파라미터 + 마우스 조작에 의한 캐릭터 상하체 로테이션
		// ─────────────────────────────────────────────

		// 입력된 키에 따라 단위벡터를 전달받지만, 이동에 따른 트랜스폼의 회전방향은 여기서 계산하지 않는다.
		// 오로지 이동할지 말지, 어느 방향으로 이동할지만 결정. 이동 방향 변경으로 인한 트랜스폼 회전은 다른곳에서
		m_inputMoveDir = GetMoveInputDirection();
		engine::Vector3 inputDir = m_inputMoveDir;

		// 이 함수에서 m_currentLogicalMoveVector를 업데이트하고, 바로 아래에서 이 변수로 조건체크한다.
		// 입력이 변경되든 말든, 현재 프레임에서는 한번 유효성을 판단한 변수로만 이동 여부를 업데이트
		UpdateLogicalMoveExistence(inputDir);

		m_isMoving = m_currentLogicalMoveVector.LengthSquared() > 0.0001f ? true : false;

		//다음프레임까지 동일한 값의 변수로 조건 판단(굳이 이렇게까지 해야하나 싶긴 한데)
		bool isMoving = m_isMoving;
		engine::Vector3 fixedInputDir = m_currentLogicalMoveVector;

		m_logicFSM->SetParameter("IsMoving", isMoving);

		if (isMoving)
		{
			m_logicFSM->SetParameter("MoveX", fixedInputDir.x);
			m_logicFSM->SetParameter("MoveZ", fixedInputDir.z);
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

		// ─────────────────────────────────────────────
		// 3. 대쉬 입력 (좌측 Shift)
		// - 이동 중일 때만 대쉬 가능
		// - 쿨다운이 끝났을 때만 대쉬 가능
		// ─────────────────────────────────────────────
		bool isShiftPressed = engine::Input::IsKeyPressed(engine::Keys::LeftShift);
		bool isSpaceBarPressed = engine::Input::IsKeyPressed(engine::Keys::Space);

		bool isDashAble = m_logicFSM->GetCurrentState() != "Execution" && m_logicFSM->GetCurrentState() != "Dash" && m_logicFSM->GetCurrentState() != "Dead";

		if ((isShiftPressed || isSpaceBarPressed) && isMoving && m_dashCooldownTimer <= 0.0f)
		{
			// 대쉬 시작 트리거
			m_logicFSM->SetTrigger("StartDash");
			StartDash();
		}
	}


	// ═══════════════════════════════════════════════════════════════
	// 게임 로직 - 비물리 (상태 확인 후 행동 실행)
	// Update()에서 호출됨 (DeltaTime 기반)
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::UpdateGameLogic()
	{
		float deltaTime = engine::Time::DeltaTime();
		
		// 이동관련 변수는 ProcessInput에서,
		// 플레이어의 로직상 좌표와 회전 관련 요소는 여기서 업데이트한다.

		UpdateLogicalPosition();
		m_objectLogicalFoward = GetTransform()->GetForward();
		m_objectLogicalFoward.y = 0;
		m_objectLogicalFoward.Normalize();

		engine::Vector3 playerPos = m_objectLogicalPosition;
		engine::Vector3 aimDir = m_aimPointer->GetDirectionFrom(playerPos);
		
		if (aimDir.LengthSquared() > 0.0001f)
		{
			CheckForwardBack(m_objectLogicalFoward, aimDir);

			engine::Vector3 finalDir = aimDir;

			finalDir = m_isBackward ? finalDir * -1.0 : finalDir * 1.0;

			CalcLogicalRotateAngle(finalDir);
		}
		engine::Vector3 rv = m_angleToRotateLogical;

		RotateToDirection(rv, m_rotationSpeed);

		// ─────────────────────────────────────────────
		// 대쉬 쿨다운 타이머 (항상 실행)
		// ─────────────────────────────────────────────
		if (m_dashCooldownTimer > 0.0f)
		{
			m_dashCooldownTimer -= deltaTime;
		}

		// Execution 상태에서는 행동 로직 스킵
		if (IsInState("Execution"))
		{
			return;
		}

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

		// Dash 상태에서는 대쉬 처리
		if (IsInState("Dash"))
		{
			HandleDash();
			// 대쉬 중에는 회전 안함 (방향 고정)
			return;
		}

		// ═══════════════════════════════════════════════════════════════
		// SphereCast 기반 지형 파고들기 방지 (Walk, WalkShoot 상태)
		// ═══════════════════════════════════════════════════════════════
		m_environmentBlockDetected = false;
		
		if (CanMove() && m_isMoving)
		{
			// 현재 이동 방향으로 SphereCast
			engine::Vector3 wallNormal;
			if (CheckEnvironmentBlock(m_currentLogicalMoveVector, wallNormal))
			{
				m_environmentBlockDetected = true;
				m_lastBlockNormal = wallNormal;
				
				// 벽 슬라이딩 방향 계산
				engine::Vector3 slideDir = CalculateSlidingDirection(m_currentLogicalMoveVector, wallNormal);
				
				if (slideDir.LengthSquared() > 0.0001f)
				{
					// 슬라이딩 방향으로 이동 (벽과 평행)
					m_currentLogicalMoveVector = slideDir;
				}
				else
				{
					// 정면 충돌 - 이동 불가
					m_currentLogicalMoveVector = engine::Vector3::Zero;
				}
			}
		}

		// 물리 기반 행동 실행
		if (CanMove())
		{
			HandleMovement(m_moveSpeed);
		}
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

			// 대쉬 중이었다면 대쉬 강제 종료
			if (m_isDashing)
			{
				m_isDashing = false;
				m_dashElapsedTime = 0.0f;
				m_dashCollisionDecayBoost = 1.0f;
			}
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// 충돌 콜백 - 대쉬 충돌 감쇠
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::OnCollisionStay(const engine::CollisionInfo& info)
	{
		// 대쉬 중이 아니면 무시
		if (!m_isDashing)
		{
			return;
		}

		// 이미 감쇠 부스트가 적용되었으면 스킵 (중복 적용 방지)
		if (m_dashCollisionDecayBoost > 1.0f)
		{
			return;
		}

		// 접촉점에서 노말 정보 추출
		if (info.contacts.empty())
		{
			return;
		}

		// 첫 번째 접촉점의 노말 사용 (여러 접촉점이 있으면 평균을 낼 수도 있음)
		engine::Vector3 collisionNormal = info.contacts[0].normal;
		collisionNormal.y = 0.0f;  // 수평 방향만 고려
		
		if (collisionNormal.LengthSquared() < 0.0001f)
		{
			return;
		}
		collisionNormal.Normalize();

		// 대쉬 방향과 충돌 노말의 dot product 계산
		// 노말은 충돌면에서 바깥쪽을 향하므로, -normal과 대쉬 방향을 비교
		// dot > threshold면 대쉬 방향으로 벽을 향해 돌진하는 상황
		float dot = m_dashDirection.Dot(-collisionNormal);

		if (dot > m_dashCollisionDotThreshold)
		{
			// 정면 충돌 감지 → 감쇠 부스트 적용
			m_dashCollisionDecayBoost = m_dashCollisionDecayMultiplier;
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
		AddFSMState("Dash");         // 대쉬 상태
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
		// 대쉬 전이 (Walk, WalkShoot -> Dash)
		// Idle, IdleShoot에서는 대쉬 불가 (이동 입력 필요)
		// ─────────────────────────────────────────────
		AddFSMTransition("Walk", "Dash", "StartDash", Trigger());
		AddFSMTransition("WalkShoot", "Dash", "StartDash", Trigger());

		// Dash -> Walk (대쉬 종료)
		AddFSMTransition("Dash", "Walk", "DashComplete", Trigger());

		// ─────────────────────────────────────────────
		// 처형 전이 (모든 상태 -> Execution)
		// ─────────────────────────────────────────────
		AddFSMTransition("Idle", "Execution", "ExecuteMonster", Trigger());
		AddFSMTransition("Walk", "Execution", "ExecuteMonster", Trigger());
		AddFSMTransition("IdleShoot", "Execution", "ExecuteMonster", Trigger());
		AddFSMTransition("WalkShoot", "Execution", "ExecuteMonster", Trigger());
		AddFSMTransition("Dash", "Execution", "ExecuteMonster", Trigger());

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
		m_animFSM->AddSplitState("Idle", m_animName_Idle, true, "", false, 0.0f, 0.1f);
		m_animFSM->AddSplitState("WalkForward", m_animName_WalkForward, true, "", false, 0.0f, 0.1f);
		m_animFSM->AddSplitState("WalkBackward", m_animName_WalkBackward, true, "", false, 0.0f, 0.1f);

		// 공격 상태: 상체 레이어 비활성화 (Fire할 때 직접 Fire 애니메이션 재생)
		// 상체 웨이트 0 → Fire 시 PlayUpperBodyAnimation으로 활성화
		m_animFSM->AddSplitState("IdleShoot", m_animName_Idle, true, "", false, 0.0f, 0.1f);
		m_animFSM->AddSplitState("WalkForwardShoot", m_animName_WalkForward, true, "", false, 0.0f, 0.1f);
		m_animFSM->AddSplitState("WalkBackwardShoot", m_animName_WalkBackward, true, "", false, 0.0f, 0.1f);
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
	// 대쉬 처리
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::StartDash()
	{
		// 현재 이동 방향을 대쉬 방향으로 고정
		engine::Vector3 moveDir = GetMoveInputDirection();
		if (moveDir.LengthSquared() < 0.0001f)
		{
			// 이동 입력이 없으면 대쉬 불가 (안전 장치)
			return;
		}

		moveDir.y = 0.0f;
		moveDir.Normalize();

		m_dashDirection = moveDir;
		m_isDashing = true;
		m_dashElapsedTime = 0.0f;
		m_dashCollisionDecayBoost = 1.0f;  // 충돌 감쇠 배율 초기화

		// ═══════════════════════════════════════════════════════════════
		// SphereCast로 대쉬 방향에 벽이 있는지 확인
		// 벽이 있으면 초기 속도를 감소시킴
		// ═══════════════════════════════════════════════════════════════
		engine::Vector3 wallNormal;
		m_dashWallDetectedOnStart = CheckEnvironmentBlock(m_dashDirection, wallNormal);
	}

	void PlayerControllerScript::EndDash()
	{
		m_isDashing = false;
		m_dashElapsedTime = 0.0f;
		m_dashCooldownTimer = m_dashCooldown;  // 쿨다운 시작
		m_dashCollisionDecayBoost = 1.0f;      // 충돌 감쇠 배율 리셋
		m_dashWallDetectedOnStart = false;     // 벽 감지 상태 리셋

		// FSM 상태 전이 (Dash → Walk)
		if (m_logicFSM)
		{
			m_logicFSM->SetTrigger("DashComplete");
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// SphereCast 기반 지형 파고들기 방지
	// QueryFilter를 사용하여 자기 자신 제외 + Environment 레이어만 감지
	// ═══════════════════════════════════════════════════════════════
	bool PlayerControllerScript::CheckEnvironmentBlock(const engine::Vector3& direction, engine::Vector3& outNormal)
	{
		// 유효하지 않은 방향이면 리턴
		if (direction.LengthSquared() < 0.0001f)
		{
			return false;
		}

		// 이동 방향 정규화 (XZ 평면)
		engine::Vector3 checkDir = direction;
		checkDir.y = 0.0f;
		checkDir.Normalize();

		// 플레이어 위치 (허리 높이에서 캐스트)
		engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
		playerPos.y += 1.0f;

		// ═══════════════════════════════════════════════════════════════
		// QueryFilter 설정:
		// - 자기 자신(플레이어) 제외
		// - Environment 레이어만 감지
		// - 바닥/천장 무시 (벽만 감지)
		// ═══════════════════════════════════════════════════════════════
		engine::QueryFilter filter;
		filter.ignoreObject = GetGameObject();                         // 자기 자신 제외
		filter.layerMask = engine::PhysicsLayer::Mask::EnvironmentMask; // Environment만
		filter.ignoreFloor = true;                                     // 바닥 무시
		filter.ignoreCeiling = true;                                   // 천장 무시
		filter.floorCeilingThreshold = 0.5f;

		// SphereCast 수행
		engine::RaycastHit hit;
		engine::PhysicsSystem& physicsSystem = engine::SystemManager::Get().GetPhysicsSystem();
		
		bool hasHit = physicsSystem.SphereCast(
			playerPos,
			m_sphereCastRadius,
			checkDir,
			m_sphereCastDistance,
			hit,
			filter
		);

		// 디버그 (필요시 주석 해제)
		// if (hasHit && hit.gameObject)
		// {
		// 	LOG_PRINT("[SphereCast] Hit: {} | Normal: ({:.2f}, {:.2f}, {:.2f}) | Distance: {:.3f}",
		// 		hit.gameObject->GetName(), hit.normal.x, hit.normal.y, hit.normal.z, hit.distance);
		// }

		if (hasHit)
		{
			// 벽의 노말 저장 (XZ 평면)
			outNormal = hit.normal;
			outNormal.y = 0.0f;
			if (outNormal.LengthSquared() > 0.0001f)
			{
				outNormal.Normalize();
			}
			else
			{
				// XZ 평면에서 유효한 노말이 없으면 무시
				return false;
			}
			
			return true;
		}

		return false;
	}

	engine::Vector3 PlayerControllerScript::CalculateSlidingDirection(
		const engine::Vector3& moveDir, 
		const engine::Vector3& wallNormal)
	{
		// 벽 슬라이딩: 이동 방향에서 벽에 수직인 성분만 제거
		// slideDir = moveDir - (moveDir · wallNormal) × wallNormal
		
		engine::Vector3 normalizedMove = moveDir;
		normalizedMove.y = 0.0f;
		if (normalizedMove.LengthSquared() < 0.0001f)
		{
			return engine::Vector3::Zero;
		}
		normalizedMove.Normalize();

		engine::Vector3 normalizedWall = wallNormal;
		normalizedWall.y = 0.0f;
		if (normalizedWall.LengthSquared() < 0.0001f)
		{
			return normalizedMove;  // 노말이 없으면 원래 방향 유지
		}
		normalizedWall.Normalize();

		// 벽을 향하는 성분 계산
		float dot = normalizedMove.Dot(normalizedWall);
		
		// 벽에서 멀어지는 방향이면 슬라이딩 불필요
		if (dot >= 0.0f)
		{
			return normalizedMove;
		}

		// 벽에 수직인 성분 제거
		engine::Vector3 slideDir = normalizedMove - normalizedWall * dot;
		
		if (slideDir.LengthSquared() > 0.0001f)
		{
			slideDir.Normalize();
		}
		else
		{
			// 정면 충돌 시 슬라이딩 불가
			slideDir = engine::Vector3::Zero;
		}

		return slideDir;
	}

	float PlayerControllerScript::CalculateDashSpeed() const
	{
		// ═══════════════════════════════════════════════════════════════
		// 지수 감쇠 공식: v(t) = v_initial × e^(-decay × t)
		// 
		// 조건: 1초 후에 정확히 기본 속도(m_moveSpeed)로 돌아와야 함
		// v(1) = v_initial × e^(-decay) = m_moveSpeed
		// 
		// v_initial = m_moveSpeed × multiplier
		// e^(-decay) = 1 / multiplier
		// decay = ln(multiplier)
		// 
		// 충돌 감쇠: decay에 m_dashCollisionDecayBoost를 곱해서 빠르게 감속
		// 벽 감지: 대쉬 시작 시 벽이 감지되면 초기 속도 감소
		// ═══════════════════════════════════════════════════════════════

		float multiplier = m_dashInitialSpeedMultiplier;
		
		// 대쉬 시작 시 벽이 감지되었으면 초기 속도 배율 감소
		if (m_dashWallDetectedOnStart)
		{
			multiplier *= m_dashWallSpeedMultiplier;
			// multiplier가 1보다 작으면 decay가 음수가 되어 속도가 증가하므로 최소 1.01로 제한
			multiplier = std::max(multiplier, 1.01f);
		}
		
		float decay = logf(multiplier);  // ln(multiplier)

		// 충돌 시 감쇠 배율 적용 (벽/적에 부딪히면 빠르게 감속)
		decay *= m_dashCollisionDecayBoost;

		// 시간 정규화 (0~1 범위로)
		float normalizedTime = m_dashElapsedTime / m_dashDuration;
		normalizedTime = std::clamp(normalizedTime, 0.0f, 1.0f);

		// 지수 감쇠 속도 계산
		float initialSpeed = m_moveSpeed * multiplier;
		float currentSpeed = initialSpeed * expf(-decay * normalizedTime);

		// 최소 속도는 기본 이동 속도
		return std::max(currentSpeed, m_moveSpeed);
	}

	void PlayerControllerScript::HandleDash()
	{
		if (!m_isDashing || !m_rigidbody || !m_rigidbody->IsDynamic())
		{
			return;
		}

		float fixedDelta = engine::Time::FixedDeltaTime();

		// 대쉬 경과 시간 업데이트
		m_dashElapsedTime += fixedDelta;

		// 대쉬 종료 조건: 지속 시간 초과
		if (m_dashElapsedTime >= m_dashDuration)
		{
			EndDash();
			return;
		}

		// ═══════════════════════════════════════════════════════════════
		// 대쉬 중 SphereCast로 벽 감지 - 히트 시 속도 감쇠
		// ═══════════════════════════════════════════════════════════════
		engine::Vector3 wallNormal;
		if (CheckEnvironmentBlock(m_dashDirection, wallNormal))
		{
			// 벽에 충돌 - 속도를 m_dashWallSpeedMultiplier 비율로 감쇠
			float reducedSpeed = CalculateDashSpeed() * m_dashWallSpeedMultiplier;
			engine::Vector3 currentVel = m_rigidbody->GetLinearVelocity();
			engine::Vector3 reducedVelocity = m_dashDirection * reducedSpeed;
			reducedVelocity.y = currentVel.y;
			m_rigidbody->SetLinearVelocity(reducedVelocity);
			return;
		}

		// 현재 대쉬 속도 계산 (지수 감쇠)
		float dashSpeed = CalculateDashSpeed();

		// 대쉬 방향으로 속도 설정
		// SetLinearVelocity 대신 목표 속도로 즉시 설정 (대쉬는 즉각적인 이동)
		engine::Vector3 currentVel = m_rigidbody->GetLinearVelocity();
		engine::Vector3 dashVelocity = m_dashDirection * dashSpeed;
		dashVelocity.y = currentVel.y;  // Y축(중력) 속도 유지

		m_rigidbody->SetLinearVelocity(dashVelocity);
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
		

	// ═══════════════════════════════════════════════════════════════
	// 애니메이션 제어
	// ═══════════════════════════════════════════════════════════════
	

	std::string PlayerControllerScript::GetAnimationState() const
	{
		bool isMoving = m_currentLogicalMoveVector.LengthSquared() > 0.0001f;
		bool isShooting = engine::Input::IsMouseHeld(engine::Input::Buttons::LEFT);
		bool isBackward = m_isBackward;

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
		bool isBackward = m_isBackward;

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

		// 대쉬 설정
		ImGui::Separator();
		ImGui::Text("Dash:");
		ImGui::DragFloat("Dash Duration (sec)", &m_dashDuration, 0.1f, 0.1f, 3.0f);
		ImGui::DragFloat("Dash Speed Multiplier", &m_dashInitialSpeedMultiplier, 0.1f, 1.5f, 10.0f);
		ImGui::DragFloat("Dash Cooldown (sec)", &m_dashCooldown, 0.1f, 0.0f, 10.0f);

		// 대쉬 충돌 감쇠 설정
		ImGui::Separator();
		ImGui::Text("Dash Collision Decay:");
		ImGui::DragFloat("Collision Decay Multiplier", &m_dashCollisionDecayMultiplier, 0.5f, 1.0f, 20.0f);
		ImGui::DragFloat("Collision Dot Threshold", &m_dashCollisionDotThreshold, 0.05f, 0.0f, 1.0f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("0.0 = 90 deg, 0.3 = ~72 deg, 0.5 = 60 deg, 0.7 = ~45 deg");
		}

		// SphereCast 설정 (지형 파고들기 방지)
		ImGui::Separator();
		ImGui::Text("SphereCast (Environment Block):");
		ImGui::DragFloat("SphereCast Radius", &m_sphereCastRadius, 0.05f, 0.1f, 2.0f);
		ImGui::DragFloat("SphereCast Distance", &m_sphereCastDistance, 0.05f, 0.1f, 2.0f);
		ImGui::DragFloat("Dash Wall Speed %%", &m_dashWallSpeedMultiplier, 0.01f, 0.0001f, 1.0f, "%.2f");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Wall detected at dash start: initial speed multiplier (0.1 = 10%%)");
		}
		
		// SphereCast 런타임 상태
		if (m_environmentBlockDetected)
		{
			ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Environment Block: DETECTED");
			ImGui::Text("  Normal: (%.2f, %.2f)", m_lastBlockNormal.x, m_lastBlockNormal.z);
		}
		else
		{
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Environment Block: Clear");
		}

		// 대쉬 런타임 정보
		ImGui::Separator();
		ImGui::Text("Dash State: %s", m_isDashing ? "DASHING" : "Ready");
		if (m_dashCooldownTimer > 0.0f)
		{
			ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Cooldown: %.1f sec", m_dashCooldownTimer);
		}
		else
		{
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Cooldown: Ready");
		}
		if (m_isDashing)
		{
			float dashSpeed = CalculateDashSpeed();
			ImGui::Text("Current Dash Speed: %.1f", dashSpeed);
			ImGui::Text("Dash Progress: %.0f%%", (m_dashElapsedTime / m_dashDuration) * 100.0f);
			
			// 벽 감지 상태 표시
			if (m_dashWallDetectedOnStart)
			{
				ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Wall Detected at Start: Speed reduced to %.0f%%", 
					m_dashWallSpeedMultiplier * 100.0f);
			}
			
			// 충돌 감쇠 상태 표시
			if (m_dashCollisionDecayBoost > 1.0f)
			{
				ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Collision Decay Active: %.1fx", m_dashCollisionDecayBoost);
			}
		}

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
		ImGui::Text("Is Moving Backward: %s", m_isBackward ? "Yes" : "No");
		//ImGui::Text("Forward Aim Dir Angle: %.2f deg", GetForwardAimDirAngle());


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

		// 대쉬 설정
		j["DashDuration"] = m_dashDuration;
		j["DashInitialSpeedMultiplier"] = m_dashInitialSpeedMultiplier;
		j["DashCooldown"] = m_dashCooldown;

		// 대쉬 충돌 감쇠 설정
		j["DashCollisionDecayMultiplier"] = m_dashCollisionDecayMultiplier;
		j["DashCollisionDotThreshold"] = m_dashCollisionDotThreshold;

		// SphereCast 설정 (지형 파고들기 방지)
		j["SphereCastRadius"] = m_sphereCastRadius;
		j["SphereCastDistance"] = m_sphereCastDistance;
		j["DashWallSpeedMultiplier"] = m_dashWallSpeedMultiplier;
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

		// 대쉬 설정
		if (j.contains("DashDuration"))
			m_dashDuration = j["DashDuration"].get<float>();
		if (j.contains("DashInitialSpeedMultiplier"))
			m_dashInitialSpeedMultiplier = j["DashInitialSpeedMultiplier"].get<float>();
		if (j.contains("DashCooldown"))
			m_dashCooldown = j["DashCooldown"].get<float>();

		// 대쉬 충돌 감쇠 설정
		if (j.contains("DashCollisionDecayMultiplier"))
			m_dashCollisionDecayMultiplier = j["DashCollisionDecayMultiplier"].get<float>();
		if (j.contains("DashCollisionDotThreshold"))
			m_dashCollisionDotThreshold = j["DashCollisionDotThreshold"].get<float>();

		// SphereCast 설정 (지형 파고들기 방지)
		if (j.contains("SphereCastRadius"))
			m_sphereCastRadius = j["SphereCastRadius"].get<float>();
		if (j.contains("SphereCastDistance"))
			m_sphereCastDistance = j["SphereCastDistance"].get<float>();
		if (j.contains("DashWallSpeedMultiplier"))
			m_dashWallSpeedMultiplier = j["DashWallSpeedMultiplier"].get<float>();
	}
}
