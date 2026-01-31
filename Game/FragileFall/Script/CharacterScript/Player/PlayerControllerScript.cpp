#include "GamePCH.h"
#include "PlayerControllerScript.h"

#include <algorithm>  // std::remove_if
#include <cmath>      // expf

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
#include <Engine/Core/System/Input.h>
#include <Engine/Core/System/MyTime.h>


namespace game
{
	// ???????????????????????????????????????????????????????????????
	// ????
	// ???????????????????????????????????????????????????????????????
	void PlayerControllerScript::Awake()
	{
		BaseControllerScript::Awake();
	}

	void PlayerControllerScript::Start()
	{
		BaseControllerScript::Start();

		// ═══════════════════════════════════════════════════════════════
		// Dynamic Rigidbody 설정 (회전 제약)
		// - PhysX가 이동/충돌 담당
		// - 회전은 스크립트가 직접 제어
		// ═══════════════════════════════════════════════════════════════
		SetupDynamicRigidbody();

		// 애니메이션 초기화 (SkeletalAnimator에 애니메이션 등록)
		InitializeAnimations();

		// LogicFSM 초기화 (한 번만)
		if (!m_fsmInitialized && m_logicFSM)
		{
			InitializeFSM();
			m_fsmInitialized = true;
		}

		// AnimFSM 초기화 (상태/애니메이션 매핑 등록)
		if (m_animFSM)
		{
			InitializeAnimFSM();

			// Procedural Aim 활성화
			if (m_enableUpperBodyAim)
			{
				m_animFSM->SetProceduralAimEnabled(true);
			}
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// Dynamic Rigidbody 설정
	// - Y축 이동 동결: 탑다운 게임에서 바닥 아래로 떨어지지 않도록
	// - 회전 동결: 물리 엔진이 회전에 개입하지 않도록
	// - 스크립트에서 Transform.SetRotation()으로 직접 회전 제어
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::SetupDynamicRigidbody()
	{
		if (!m_rigidbody) return;
		
		// Dynamic Rigidbody 확인
		if (!m_rigidbody->IsDynamic())
		{
			// 경고: Dynamic이 아니면 물리 충돌 처리 불가
			return;
		}
		
		// Y축 이동 동결 + 모든 회전 동결
		// FreezePositionY (2) + FreezeRotation (56) = 58
		engine::RigidbodyConstraints constraints = static_cast<engine::RigidbodyConstraints>(
			static_cast<int>(engine::RigidbodyConstraints::FreezePositionY) |
			static_cast<int>(engine::RigidbodyConstraints::FreezeRotation)
		);
		m_rigidbody->SetConstraints(constraints);
	}

	// ═══════════════════════════════════════════════════════════════
	// 회전 속도 강제 0 (안전 장치)
	// - 매 FixedUpdate에서 호출하여 물리 회전 방지
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::ForceStopRotation()
	{
		if (m_rigidbody && m_rigidbody->IsDynamic())
		{
			m_rigidbody->SetAngularVelocity(engine::Vector3::Zero);
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// 충돌 콜백 (PhysX → CollisionSystem → Script)
	// - 매 프레임 갱신 방식 (댕글링/고스팅 방지)
	// - OnCollisionStay에서만 노말 수집 (Stay는 실제 충돌 중일 때만 호출)
	// - FixedUpdate 시작 시 클리어하므로 죽은 오브젝트 문제 없음
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::OnCollisionEnter(const engine::CollisionInfo& info)
	{
		// Enter는 무시 - Stay에서 처리
		// (Enter 직후 바로 Stay가 호출되므로 중복 방지)
	}

	void PlayerControllerScript::OnCollisionStay(const engine::CollisionInfo& info)
	{
		// 충돌 유지 중: 노말 수집
		for (const auto& contact : info.contacts)
		{
			// 노말 반전: A→B 방향이므로, 플레이어 입장에서는 반대 방향
			engine::Vector3 normal = -contact.normal;
			normal.y = 0.0f;  // 수평 성분만
			if (normal.LengthSquared() > 0.0001f)
			{
				normal.Normalize();
				
				// 비슷한 노말이 이미 있는지 확인 (중복 방지)
				bool found = false;
				for (const auto& existing : m_frameCollisionNormals)
				{
					if (existing.Dot(normal) > 0.9f)
					{
						found = true;
						break;
					}
				}
				if (!found)
				{
					m_frameCollisionNormals.push_back(normal);
				}
			}
		}
	}

	void PlayerControllerScript::OnCollisionExit(const engine::CollisionInfo& info)
	{
		// Exit는 무시 - 매 프레임 클리어 방식이므로 자동 처리됨
	}

	// ═══════════════════════════════════════════════════════════════
	// 충돌 기반 이동 제한
	// ═══════════════════════════════════════════════════════════════
	bool PlayerControllerScript::IsMovingIntoCollision(const engine::Vector3& moveDirection) const
	{
		if (!m_isColliding || m_frameCollisionNormals.empty())
		{
			return false;
		}
		
		engine::Vector3 moveDir = moveDirection;
		moveDir.y = 0.0f;
		if (moveDir.LengthSquared() < 0.0001f)
		{
			return false;
		}
		moveDir.Normalize();
		
		// 어느 하나의 충돌 노말 방향으로 이동하려 하면 true
		for (const auto& normal : m_frameCollisionNormals)
		{
			float dot = moveDir.Dot(normal);
			if (dot > 0.1f)  // 충돌 방향으로 이동 시도
			{
				return true;
			}
		}
		
		return false;
	}

	engine::Vector3 PlayerControllerScript::RemoveCollisionComponent(const engine::Vector3& velocity) const
	{
		if (!m_isColliding || m_frameCollisionNormals.empty())
		{
			return velocity;
		}
		
		engine::Vector3 result = velocity;
		
		// 각 충돌 노말 방향의 속도 성분 제거
		for (const auto& normal : m_frameCollisionNormals)
		{
			float dot = result.Dot(normal);
			if (dot > 0.0f)  // 충돌 방향으로 이동하는 성분만 제거
			{
				result -= normal * dot;
			}
		}
		
		return result;
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

		// ?????????????????????????????????????????????
		// AimPointer: ??? ?? (UI ?? ?? ??)
		// ?????????????????????????????????????????????
		// ?? ?? ?????? ?? (?? ???)
		m_aimPointer = GetGameObject()->GetComponent<AimPointer>();
		if (!m_aimPointer)
		{
			auto* scene = engine::SceneManager::Get().GetScene();
			if (scene)
			{
				// ??? ??? ?????? ??
				if (auto* aimGO = scene->FindGameObject(m_aimPointerObjectName))
				{
					m_aimPointer = aimGO->GetComponent<AimPointer>();
				}
				// ??? "AimPointer"? ??? ???? ?? (??)
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

		// ?????????????????????????????????????????????
		// BulletFactory: ?? ?????? ??
		// ?????????????????????????????????????????????
		m_bulletFactory = GetGameObject()->GetComponent<BulletFactory>();

	}

	// ???????????????????????????????????????????????????????????????
	// ?? ?? (????? ?? ??)
	// ???????????????????????????????????????????????????????????????
	bool PlayerControllerScript::CanMove() const
	{
		// ?? ??? ?? ??
		if (IsInState("Execution"))
		{
			return false;
		}
		// ?? ??? ?? ?? ?? (?? ????? ??)
		if (IsInState("Dash"))
		{
			return false;
		}
		return true;
	}

	bool PlayerControllerScript::CanAttack() const
	{
		// ?? ??? ?? ??
		if (IsInState("Execution"))
		{
			return false;
		}
		// ?? ??? ?? ??
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

	// ???????????????????????????????????????????????????????????????
	// ?? ?? (?? ? FSM ????)
	// ???????????????????????????????????????????????????????????????
	void PlayerControllerScript::ProcessInput()
	{
		if (!m_logicFSM) return;

		// Execution ????? ?? ?? ??
		if (IsInState("Execution"))
		{
			return;
		}

		// Dash ????? ?? ?? (?? ?? ??)
		if (IsInState("Dash"))
		{
			return;
		}

		// ?????????????????????????????????????????????
		// 1. ?? ?? ? FSM ???? + ??? ??? ?? ??? ??? ????
		// ?????????????????????????????????????????????

		// ??? ?? ?? ????? ?????, ??? ?? ????? ????? ??? ???? ???.
		// ??? ???? ??, ?? ???? ????? ??. ?? ?? ???? ?? ???? ??? ?????
		m_inputMoveDir = GetMoveInputDirection();
		engine::Vector3 inputDir = m_inputMoveDir;

		// ? ???? m_currentLogicalMoveVector? ??????, ?? ???? ? ??? ??????.
		// ??? ???? ??, ?? ?????? ?? ???? ??? ???? ?? ??? ????
		UpdateLogicalMoveExistence(inputDir);

		m_isMoving = m_currentLogicalMoveVector.LengthSquared() > 0.0001f ? true : false;

		//??????? ??? ?? ??? ?? ??(?? ????? ???? ?? ??)
		bool isMoving = m_isMoving;
		engine::Vector3 fixedInputDir = m_currentLogicalMoveVector;

		m_logicFSM->SetParameter("IsMoving", isMoving);

		if (isMoving)
		{
			m_logicFSM->SetParameter("MoveX", fixedInputDir.x);
			m_logicFSM->SetParameter("MoveZ", fixedInputDir.z);
		}	


		// ?????????????????????????????????????????????
		// 2. ?? ?? ? FSM ????
		// ?????????????????????????????????????????????
		bool isMousePressed = engine::Input::IsMousePressed(engine::Input::Buttons::LEFT);
		bool isMouseHeld = engine::Input::IsMouseHeld(engine::Input::Buttons::LEFT);

		// Pressed ? ??? (?? ???)
		if (isMousePressed)
		{
			m_logicFSM->SetTrigger("Attack");
		}

		// Held ? ???? (?? ???)
		m_logicFSM->SetParameter("IsShooting", isMouseHeld);

		// ?????????????????????????????????????????????
		// 3. ?? ?? (?? Shift)
		// - ?? ?? ?? ?? ??
		// - ???? ??? ?? ?? ??
		// - ?? ???? 0?? ? ?? ?? ??
		// ?????????????????????????????????????????????
		bool isShiftPressed = engine::Input::IsKeyPressed(engine::Keys::LeftShift);
		bool isSpaceBarPressed = engine::Input::IsKeyPressed(engine::Keys::Space);

		bool isDashAble = m_logicFSM->GetCurrentState() != "Execution" && m_logicFSM->GetCurrentState() != "Dash" && m_logicFSM->GetCurrentState() != "Dead";

		if ((isShiftPressed || isSpaceBarPressed) && isMoving && m_dashCooldownTimer <= 0.0f && m_CurrentDashCount > 0)
		{
			// ?? ?? ???
			m_logicFSM->SetTrigger("StartDash");
			StartDash();
		}
	}


	// ???????????????????????????????????????????????????????????????
	// ?? ?? - ??? (?? ?? ? ?? ??)
	// Update()?? ??? (DeltaTime ??)
	// ???????????????????????????????????????????????????????????????
	void PlayerControllerScript::UpdateGameLogic()
	{
		float deltaTime = engine::Time::DeltaTime();
		
		// ???? ??? ProcessInput??,
		// ????? ??? ??? ?? ?? ??? ??? ??????.

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

		// ?????????????????????????????????????????????
		// ?? ??? ??? (?? ??)
		// ?????????????????????????????????????????????
		if (m_dashCooldownTimer > 0.0f)
		{
			m_dashCooldownTimer -= deltaTime;
		}

		// ?????????????????????????????????????????????
		// ?? ??? ???
		// - ???? ??? ??? ?? ??
		// - ???? 0? ?? ??? 1 ?? ? ??? ??
		// - ???? ???? ?? ??? ???
		// ?????????????????????????????????????????????
		if (m_CurrentDashCount < m_MaxDashCount)
		{
			m_dashRechargeTimer -= deltaTime;
			if (m_dashRechargeTimer <= 0.0f)
			{
				m_CurrentDashCount++;
				if (m_CurrentDashCount < m_MaxDashCount)
				{
					// ?? ???? ??? ??? ???
					m_dashRechargeTimer = m_dashRechargeTime;
				}
				else
				{
					// ???? ???? ??? ??
					m_dashRechargeTimer = m_dashRechargeTime;
				}
			}
		}

		// Execution ????? ?? ?? ??
		if (IsInState("Execution"))
		{
			return;
		}

		// ??? ?? ?? (???, ????? ?)
		if (CanAttack())  HandleShooting(deltaTime);

		// ????? ???? (???)
		UpdateAnimation();
	}

	// ═══════════════════════════════════════════════════════════════
	// 물리 로직 - FixedUpdate()에서 호출됨 (FixedDeltaTime 사용)
	// Dynamic Rigidbody: PhysX가 충돌 처리, AddForce로 이동
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::UpdatePhysicsLogic()
	{
		// ═══════════════════════════════════════════════════════════════
		// 프레임 시작: 이전 프레임의 충돌 노말로 상태 결정 후 클리어
		// - OnCollisionStay는 FixedUpdate 후에 호출됨
		// - 이전 프레임에서 수집된 노말을 사용
		// ═══════════════════════════════════════════════════════════════
		m_isColliding = !m_frameCollisionNormals.empty();
		
		// Execution 상태에서는 물리 로직 스킵 (이동, 회전 모두 정지)
		if (IsInState("Execution"))
		{
			m_frameCollisionNormals.clear();  // 클리어
			return;
		}

		// Dash 상태에서는 대쉬 전용 처리
		if (IsInState("Dash"))
		{
			HandleDash();
			m_frameCollisionNormals.clear();  // 클리어
			return;
		}

		// ═══════════════════════════════════════════════════════════════
		// 충돌 기반 이동 제한 (이전 프레임의 충돌 노말 사용)
		// - 충돌 방향으로 이동 시도 시 해당 성분 제거
		// - 정지 판정: 밀어내기 힘 적용
		// - 슬라이딩 판정: 감속된 속도로 미끄러짐
		// ═══════════════════════════════════════════════════════════════
		bool isSliding = false;
		
		if (m_isColliding && m_isMoving)
		{
			// 충돌 방향 성분 제거
			engine::Vector3 adjustedMoveDir = RemoveCollisionComponent(m_currentLogicalMoveVector);
			
			// 조정된 방향이 거의 0이면 정지 판정
			if (adjustedMoveDir.LengthSquared() < 0.01f)
			{
				m_currentLogicalMoveVector = engine::Vector3::Zero;
				m_isMoving = false;
				
				// 정지 판정: 충돌 노말 방향으로 밀어내기 (벽에서 살짝 밀려남)
				if (m_rigidbody && m_rigidbody->IsDynamic() && m_collisionPushBackForce > 0.0f)
				{
					// 평균 충돌 노말 계산
					engine::Vector3 avgNormal = engine::Vector3::Zero;
					for (const auto& normal : m_frameCollisionNormals)
					{
						avgNormal += normal;
					}
					if (!m_frameCollisionNormals.empty())
					{
						avgNormal /= static_cast<float>(m_frameCollisionNormals.size());
						if (avgNormal.LengthSquared() > 0.0001f)
						{
							avgNormal.Normalize();
							// 충돌 반대 방향 (노말 방향)으로 약한 밀어내기
							engine::Vector3 pushBack = avgNormal * m_collisionPushBackForce;
							m_rigidbody->AddForce(pushBack, engine::ForceMode::Impulse);
						}
					}
				}
			}
			else
			{
				// 슬라이딩 판정: 방향 조정
				adjustedMoveDir.Normalize();
				m_currentLogicalMoveVector = adjustedMoveDir;
				isSliding = true;
			}
		}

		// ═══════════════════════════════════════════════════════════════
		// 이동 처리 (BaseControllerScript::HandleMovement)
		// - Dynamic: AddForce 기반, PhysX가 충돌 자동 처리
		// - 슬라이딩 시 감속된 속도로 이동
		// ═══════════════════════════════════════════════════════════════
		if (CanMove())
		{
			float effectiveSpeed = m_moveSpeed;
			if (isSliding)
			{
				effectiveSpeed *= m_slidingSpeedMultiplier;
			}
			HandleMovement(effectiveSpeed);
		}

		// ═══════════════════════════════════════════════════════════════
		// 회전 속도 강제 0 (안전 장치)
		// - FreezeRotation 제약이 있어도 외부 충격으로 회전 가능
		// - 매 프레임 각속도 0으로 강제
		// ═══════════════════════════════════════════════════════════════
		ForceStopRotation();
		
		// ═══════════════════════════════════════════════════════════════
		// 프레임 끝: 충돌 노말 클리어 (다음 OnCollisionStay에서 다시 수집)
		// ═══════════════════════════════════════════════════════════════
		m_frameCollisionNormals.clear();
	}

	// ═══════════════════════════════════════════════════════════════
	// 상태 진입 콜백
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::OnStateEntered(const std::string& state)
	{
		// ????? ????? FSM ??? ?? ????? ???? ??
		// ??? ??? ??? ?? ??

		if (state == "Execution")
		{
			// 처형 상태 진입 시 속도 초기화
			m_currentVelocity = engine::Vector3::Zero;
			
			// Dynamic Rigidbody 속도도 초기화
			if (m_rigidbody && m_rigidbody->IsDynamic())
			{
				m_rigidbody->SetLinearVelocity(engine::Vector3::Zero);
			}

			// 대쉬 중이었다면 대쉬 상태 초기화
			if (m_isDashing)
			{
				m_isDashing = false;
				m_dashElapsedTime = 0.0f;
			}
		}
	}

	void PlayerControllerScript::StartExecution(engine::GameObject* targetMonster)
	{
		if (!targetMonster || !m_logicFSM) return;

		// 1. ExecuteMonster ??? ?? ? Execution ????? ??
		m_logicFSM->SetTrigger("ExecuteMonster");

		// 2. 몬스터 위치로 플레이어 순간이동
		engine::Transform* monsterTransform = targetMonster->GetTransform();
		if (monsterTransform)
		{
			engine::Vector3 targetPos = monsterTransform->GetWorldPosition();

			// Rigidbody가 있으면 ForceSetPosition 사용 (Dynamic/Kinematic 모두)
			if (m_rigidbody)
			{
				m_rigidbody->ForceSetPosition(targetPos, true);  // 속도 리셋
			}
			else if (GetTransform())
			{
				// Rigidbody가 없는 경우 Transform 직접 설정
				GetTransform()->SetLocalPosition(targetPos);
			}
		}
	}

	// ???????????????????????????????????????????????????????????????
	// FSM ???
	// ???????????????????????????????????????????????????????????????
	void PlayerControllerScript::InitializeFSM()
	{
		if (!m_logicFSM || m_fsmInitialized) return;

		m_logicFSM->ClearStates();

		// ?????????????????????????????????????????????
		// ?? ??
		// ?????????????????????????????????????????????
		AddFSMState("Idle", true);   // ?? ??
		AddFSMState("Walk");
		AddFSMState("IdleShoot");
		AddFSMState("WalkShoot");
		AddFSMState("Dash");         // ?? ??
		AddFSMState("Execution");    // ?? ??

		m_logicFSM->UpdateStateMap();
		m_logicFSM->SetDefaultState("Idle");
		m_logicFSM->InitializeCurrentState();

		// ?????????????????????????????????????????????
		// ?? ??
		// ?????????????????????????????????????????????
		// Idle <-> Walk (?? ??)
		AddFSMTransition("Idle", "Walk", "IsMoving", BoolTrue());
		AddFSMTransition("Walk", "Idle", "IsMoving", BoolFalse());

		// Idle/Walk -> Shoot (?? ???)
		AddFSMTransition("Idle", "IdleShoot", "Attack", Trigger());
		AddFSMTransition("Walk", "WalkShoot", "Attack", Trigger());

		// IdleShoot <-> WalkShoot (?? ??)
		AddFSMTransition("IdleShoot", "WalkShoot", "IsMoving", BoolTrue());
		AddFSMTransition("WalkShoot", "IdleShoot", "IsMoving", BoolFalse());

		// Shoot -> ?Shoot (??? Released)
		AddFSMTransition("IdleShoot", "Idle", "IsShooting", BoolFalse());
		AddFSMTransition("WalkShoot", "Walk", "IsShooting", BoolFalse());

		// ?????????????????????????????????????????????
		// ?? ?? (Walk, WalkShoot -> Dash)
		// Idle, IdleShoot??? ?? ?? (?? ?? ??)
		// ?????????????????????????????????????????????
		AddFSMTransition("Walk", "Dash", "StartDash", Trigger());
		AddFSMTransition("WalkShoot", "Dash", "StartDash", Trigger());

		// Dash -> Walk (?? ??)
		AddFSMTransition("Dash", "Walk", "DashComplete", Trigger());

		// ?????????????????????????????????????????????
		// ?? ?? (?? ?? -> Execution)
		// ?????????????????????????????????????????????
		AddFSMTransition("Idle", "Execution", "ExecuteMonster", Trigger());
		AddFSMTransition("Walk", "Execution", "ExecuteMonster", Trigger());
		AddFSMTransition("IdleShoot", "Execution", "ExecuteMonster", Trigger());
		AddFSMTransition("WalkShoot", "Execution", "ExecuteMonster", Trigger());
		AddFSMTransition("Dash", "Execution", "ExecuteMonster", Trigger());

		// Execution -> Idle (?? ?? ???)
		AddFSMTransition("Execution", "Idle", "ExecutionComplete", Trigger());
	}

	void PlayerControllerScript::InitializeAnimFSM()
	{
		if (!m_animFSM) return;

		// ?? ?? ???
		m_animFSM->ClearStates();

		// ?????????????????????????????????????????????
		// ?/?? ?? ????? ?? ??
		// AddSplitState(???, ????, ????, ????, ????, ?????, ??????)
		// ?????? 0?? ?? ??? ???? (??? ??? ??)
		// 
		// animState? ???? ??:
		//   - Idle, WalkForward, WalkBackward
		//   - IdleShoot, WalkForwardShoot, WalkBackwardShoot
		// ?????????????????????????????????????????????

		// ??? ??: ?? ??? ???? (?? ?????? ??? ??)
		m_animFSM->AddSplitState("Idle", m_animName_Idle, true, "", false, 0.0f, 0.1f);
		m_animFSM->AddSplitState("WalkForward", m_animName_WalkForward, true, "", false, 0.0f, 0.1f);
		m_animFSM->AddSplitState("WalkBackward", m_animName_WalkBackward, true, "", false, 0.0f, 0.1f);

		// ?? ??: ?? ??? ???? (Fire? ? ?? Fire ????? ??)
		// ?? ??? 0 ? Fire ? PlayUpperBodyAnimation?? ???
		m_animFSM->AddSplitState("IdleShoot", m_animName_Idle, true, "", false, 0.0f, 0.1f);
		m_animFSM->AddSplitState("WalkForwardShoot", m_animName_WalkForward, true, "", false, 0.0f, 0.1f);
		m_animFSM->AddSplitState("WalkBackwardShoot", m_animName_WalkBackward, true, "", false, 0.0f, 0.1f);
	}

	void PlayerControllerScript::InitializeAnimations()
	{
		if (!m_skeletalAnimator) return;

		// ?????????????????????????????????????????????
		// SkeletalAnimator? ????? ??
		// ???? ??:
		//   1. Idle: ?? ?????
		//   2. WalkForward: ?? ?????
		//   3. WalkBackward: ?? ?????
		//   4. Fire: ?? ????? (??)
		// ?????????????????????????????????????????????

		// ?? ????? ???? ?? ?? (?? ??? ????? ??)
		// m_skeletalAnimator->ClearAnimations();

		// ????? ?? (??? ?? ?? ???? ??)
		// ??: ?? ????? SkeletalAnimator? API? ?? ?? ??
		// ??: m_skeletalAnimator->LoadAnimation(m_animName_Idle, "path/to/idle.anim");

		// ??? ?? SkeletalAnimator? ?????? ???? ??? ????,
		// ?? ??? ??? ??? ???? ??? ?

		// ??? ??? ????? ?? ??
		// if (!m_skeletalAnimator->HasAnimation(m_animName_Idle))
		// {
		//     LOG_ERROR("Animation '{}' not found in SkeletalAnimator", m_animName_Idle);
		// }
	}

	// ???????????????????????????????????????????????????????????????
	// ?? ????
	// ???????????????????????????????????????????????????????????????
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
	// 대쉬 처리 (Dynamic Rigidbody + Impulse)
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::StartDash()
	{
		// ?? ?? ??? ?? ???? ??
		engine::Vector3 moveDir = GetMoveInputDirection();
		if (moveDir.LengthSquared() < 0.0001f)
		{
			// ?? ??? ??? ?? ?? (?? ??)
			return;
		}

		moveDir.y = 0.0f;
		moveDir.Normalize();

		m_dashDirection = moveDir;
		m_isDashing = true;
		m_dashElapsedTime = 0.0f;

		// ═══════════════════════════════════════════════════════════════
		// 대쉬 카운트 관리
		// - 대쉬 사용 시 카운트 감소
		// - 최대치 3개일 때 사용하면 충전이 시작됨
		// ═══════════════════════════════════════════════════════════════
		m_CurrentDashCount--;
		if (m_CurrentDashCount < m_MaxDashCount)
		{
			// 대쉬를 처음 사용할 때만 충전 타이머 시작
			// (이미 충전 중이면 타이머를 리셋하지 않음)
			if (m_CurrentDashCount == m_MaxDashCount - 1)
			{
				m_dashRechargeTimer = m_dashRechargeTime;
			}
		}

		// ═══════════════════════════════════════════════════════════════
		// Dynamic Rigidbody: Impulse로 순간 가속
		// - PhysX가 충돌 자동 처리
		// - 별도의 SphereCast 불필요
		// ═══════════════════════════════════════════════════════════════
		if (m_rigidbody && m_rigidbody->IsDynamic())
		{
			// 대쉬 Impulse 계산: 방향 * 속도 * 배율
			float dashImpulse = m_moveSpeed * m_dashImpulseMultiplier;
			engine::Vector3 impulseForce = m_dashDirection * dashImpulse;
			
			m_rigidbody->AddForce(impulseForce, engine::ForceMode::Impulse);
		}
	}

	void PlayerControllerScript::EndDash()
	{
		m_isDashing = false;
		m_dashElapsedTime = 0.0f;
		m_dashCooldownTimer = m_dashCooldown;

		// FSM 상태 전이 (Dash → Walk)
		if (m_logicFSM)
		{
			m_logicFSM->SetTrigger("DashComplete");
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// 대쉬 처리 (Dynamic Rigidbody + Impulse 방식)
	// - StartDash()에서 Impulse 적용됨
	// - PhysX가 충돌/감속 자동 처리
	// - 여기서는 시간만 체크하고 종료
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::HandleDash()
	{
		if (!m_isDashing)
		{
			return;
		}

		float fixedDelta = engine::Time::FixedDeltaTime();

		// 대쉬 시간 업데이트
		m_dashElapsedTime += fixedDelta;

		// 대쉬 종료 조건: 지속 시간 초과
		if (m_dashElapsedTime >= m_dashDuration)
		{
			EndDash();
			return;
		}

		// ═══════════════════════════════════════════════════════════════
		// 대쉬 지수 감쇠 (매 프레임 속도 감소)
		// - 초반 급가속 후 빠르게 감속
		// - decayFactor = e^(-progress * decayRate)
		// ═══════════════════════════════════════════════════════════════
		if (m_rigidbody && m_rigidbody->IsDynamic() && m_dashDecayRate > 0.0f)
		{
			// 진행률 (0 → 1)
			float progress = m_dashElapsedTime / m_dashDuration;
			
			// 지수 감쇠 계수 (progress가 커질수록 decayFactor가 작아짐)
			// decayRate가 클수록 빠르게 감속
			float decayFactor = expf(-progress * m_dashDecayRate);
			
			// 현재 수평 속도 가져오기
			engine::Vector3 currentVel = m_rigidbody->GetLinearVelocity();
			engine::Vector3 horizontalVel(currentVel.x, 0.0f, currentVel.z);
			
			// 대쉬 방향으로의 속도만 감쇠 적용
			float speedInDashDir = horizontalVel.Dot(m_dashDirection);
			if (speedInDashDir > 0.0f)
			{
				// 이전 프레임 대비 감쇠량 계산
				float prevProgress = (m_dashElapsedTime - fixedDelta) / m_dashDuration;
				float prevDecay = expf(-prevProgress * m_dashDecayRate);
				float decayRatio = (prevDecay > 0.0001f) ? decayFactor / prevDecay : decayFactor;
				
				// 감쇠 적용 (대쉬 방향 성분만)
				engine::Vector3 dashVelComponent = m_dashDirection * speedInDashDir;
				engine::Vector3 otherVelComponent = horizontalVel - dashVelComponent;
				
				engine::Vector3 newHorizontalVel = dashVelComponent * decayRatio + otherVelComponent;
				m_rigidbody->SetLinearVelocity(engine::Vector3(newHorizontalVel.x, currentVel.y, newHorizontalVel.z));
			}
		}
		
		// 회전 속도 강제 0 (대쉬 중에도 적용)
		ForceStopRotation();
	}

	void PlayerControllerScript::HandleShooting(float deltaTime)
	{
		// ?????????????????????????????????????????????
		// ??? ??? ?? (?? ??)
		// ?????????????????????????????????????????????
		if (m_fireTimer > 0.0f)
		{
			m_fireTimer -= deltaTime;
		}

		// ?????????????????????????????????????????????
		// ?? ??: ??? ?? + ??? ??
		// ?????????????????????????????????????????????
		bool isMouseHeld = engine::Input::IsMouseHeld(engine::Input::Buttons::LEFT);

		if (isMouseHeld && m_fireTimer <= 0.0f)
		{
			// ??!
			if (m_bulletFactory && m_aimPointer)
			{
				engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
				engine::Vector3 direction = m_aimPointer->GetDirectionFrom(playerPos);

				// BulletParams? ??
				BulletParams params;
				params.type = BulletType::Linear;
				params.speed = m_bulletSpeed;
				params.lifetime = m_bulletLifetime;
				params.damage = 10;  // TODO: ?? ??? ??

				m_bulletFactory->Fire(playerPos, direction, params);



				// ?? ????? ?? (??? ???)
				if (m_animFSM)
				{
					m_animFSM->PlayUpperBodyAnimation(m_animName_Fire, false);
				}

				// ??? ??? (?? ???? ????)
				m_fireTimer = m_fireRate;
			}
		}

		// ??: ???? ?? ???? ????? ??
		// ??? ???? ???? ?? ??
	}
		

	// ???????????????????????????????????????????????????????????????
	// ????? ??
	// ???????????????????????????????????????????????????????????????
	

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

		// ?????????????????????????????????????????????
		// LogicFSM ?? + ?? ?? ? AnimFSM ?? ??
		// ?????????????????????????????????????????????
		std::string logicState = m_logicFSM ? m_logicFSM->GetCurrentState() : "Idle";
		bool isMoving = GetMoveInputDirection().LengthSquared() > 0.0001f;
		bool isShooting = (logicState == "IdleShoot" || logicState == "WalkShoot");
		bool isBackward = m_isBackward;

		// AnimFSM ?? ?? (InitializeAnimFSM?? ??? ???? ???? ?)
		std::string animState;

		if (isShooting)
		{
			// ?? ??
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
			// ??? ??
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

		// AnimFSM? ?? ?? (AnimFSM? PlayStateAnimation ??)
		m_animFSM->SetAnimState(animState);
	}


	



	// ???????????????????????????????????????????????????????????????
	// ??? ??
	// ???????????????????????????????????????????????????????????????
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

	// ???????????????????????????????????????????????????????????????
	// GUI / ???
	// ???????????????????????????????????????????????????????????????
	void PlayerControllerScript::OnGui()
	{
		ImGui::Indent();

		ImGui::Text("PlayerControllerScript (Dynamic Rigidbody):");

		// ?????????????????????????????????????????????
		// ???? ?? (??? ????? ??)
		// ?????????????????????????????????????????????
		ImGui::Separator();
		ImGui::Text("=== Component Validation ===");

		// ??? ??? ?? ??? ???? ?? (?? GameObject ???? ??)
		engine::Rigidbody* rigidbody = m_rigidbody ? m_rigidbody : (GetGameObject() ? GetGameObject()->GetComponent<engine::Rigidbody>() : nullptr);
		engine::SkeletalAnimator* skeletalAnimator = m_skeletalAnimator ? m_skeletalAnimator : (GetGameObject() ? GetGameObject()->GetComponent<engine::SkeletalAnimator>() : nullptr);
		engine::AnimFSM* animFSM = m_animFSM ? m_animFSM : (GetGameObject() ? GetGameObject()->GetComponent<engine::AnimFSM>() : nullptr);
		engine::LogicFSM* logicFSM = m_logicFSM ? m_logicFSM : (GetGameObject() ? GetGameObject()->GetComponent<engine::LogicFSM>() : nullptr);
		AimPointer* aimPointer = m_aimPointer ? m_aimPointer : (GetGameObject() ? GetGameObject()->GetComponent<AimPointer>() : nullptr);
		BulletFactory* bulletFactory = m_bulletFactory ? m_bulletFactory : (GetGameObject() ? GetGameObject()->GetComponent<BulletFactory>() : nullptr);

		// ?? ??? ??
		bool allValid = rigidbody && skeletalAnimator && aimPointer && bulletFactory && animFSM && logicFSM;

		if (allValid)
		{
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "[OK] All components are valid!");
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "[ERROR] Some components are missing!");
		}

		// Rigidbody 타입 검증 (Dynamic 필수)
		if (rigidbody)
		{
			if (rigidbody->IsDynamic())
			{
				ImGui::TextColored(ImVec4(0, 1, 0, 1), "[OK] Rigidbody: Dynamic");
			}
			else
			{
				ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "[WARNING] Rigidbody is Kinematic! Change to Dynamic for physics collision.");
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "[ERROR] Rigidbody: MISSING");
		}

		// ?? ???? ?? ??
		ImGui::Indent();
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

		// ═══════════════════════════════════════════════════════════════
		// Movement (Dynamic Rigidbody)
		// - Max Speed: 목표 최대 속도
		// - Acceleration: 최대 속도에 도달하는 속도
		// - Deceleration: 정지하는 속도
		// ═══════════════════════════════════════════════════════════════
		ImGui::Separator();
		ImGui::Text("Movement (Dynamic Physics):");
		
		// 속도 설정 (함께 표시)
		ImGui::DragFloat("Max Speed", &m_moveSpeed, 0.1f, 0.0f, 100.0f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Target maximum movement speed (m/s)");
		}
		ImGui::DragFloat("Acceleration", &m_movementAcceleration, 1.0f, 1.0f, 200.0f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("How fast to reach max speed (higher = snappier)");
		}
		ImGui::DragFloat("Deceleration", &m_movementDeceleration, 1.0f, 1.0f, 200.0f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("How fast to stop (higher = snappier)");
		}
		ImGui::DragFloat("Max Speed Brake", &m_maxSpeedBrakeFactor, 0.5f, 1.0f, 50.0f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Brake force when exceeding max speed");
		}
		
		ImGui::Spacing();
		ImGui::DragFloat("Rotation Speed (rad/s)", &m_rotationSpeed, 0.5f, 1.0f, 30.0f);

		// Rigidbody 상태 표시
		ImGui::Spacing();
		if (m_rigidbody)
		{
			bool isDynamic = m_rigidbody->IsDynamic();
			ImGui::TextColored(isDynamic ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0.5f, 0, 1),
				"Rigidbody: %s", isDynamic ? "Dynamic" : "Kinematic");
			
			if (isDynamic)
			{
				engine::Vector3 linearVel = m_rigidbody->GetLinearVelocity();
				float horizSpeed = sqrtf(linearVel.x * linearVel.x + linearVel.z * linearVel.z);
				ImGui::Text("Current Speed: %.2f / %.2f", horizSpeed, m_moveSpeed);
				
				// 진행바로 속도 표시
				float speedRatio = m_moveSpeed > 0.0f ? horizSpeed / m_moveSpeed : 0.0f;
				ImGui::ProgressBar(speedRatio, ImVec2(-1, 0), "");
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Rigidbody: NOT FOUND");
		}
		
		// ═══════════════════════════════════════════════════════════════
		// 충돌 반응 설정
		// ═══════════════════════════════════════════════════════════════
		ImGui::Separator();
		ImGui::Text("Collision Response:");
		ImGui::DragFloat("Push Back Force", &m_collisionPushBackForce, 0.05f, 0.0f, 5.0f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Impulse force when fully blocked (pushes away from wall)");
		}
		ImGui::DragFloat("Sliding Speed", &m_slidingSpeedMultiplier, 0.05f, 0.0f, 1.0f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Speed multiplier when sliding along walls (0.0~1.0)");
		}
		
		// 충돌 상태 표시
		ImGui::Spacing();
		if (m_isColliding && !m_frameCollisionNormals.empty())
		{
			ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Status: COLLIDING (%d normals)", 
				static_cast<int>(m_frameCollisionNormals.size()));
			for (size_t i = 0; i < m_frameCollisionNormals.size() && i < 3; ++i)
			{
				ImGui::Text("  Normal[%zu]: (%.2f, %.2f, %.2f)", 
					i, m_frameCollisionNormals[i].x, m_frameCollisionNormals[i].y, m_frameCollisionNormals[i].z);
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: Clear");
		}

		// ═══════════════════════════════════════════════════════════════
		// Dash (Impulse + 지수 감쇠)
		// ═══════════════════════════════════════════════════════════════
		ImGui::Separator();
		ImGui::Text("Dash:");
		ImGui::DragFloat("Dash Duration (sec)", &m_dashDuration, 0.1f, 0.1f, 3.0f);
		ImGui::DragFloat("Dash Impulse Multiplier", &m_dashImpulseMultiplier, 0.5f, 5.0f, 50.0f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Dash impulse = moveSpeed * this value. Higher = faster initial speed");
		}
		ImGui::DragFloat("Dash Decay Rate", &m_dashDecayRate, 0.1f, 0.0f, 10.0f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Exponential decay rate. Higher = faster slowdown during dash");
		}
		ImGui::DragFloat("Dash Cooldown (sec)", &m_dashCooldown, 0.1f, 0.0f, 10.0f);
		
		// 대쉬 상태 표시
		ImGui::Text("Dash State: %s", m_isDashing ? "DASHING" : "Ready");
		ImGui::Text("Dash Count: %d / %d", m_CurrentDashCount, m_MaxDashCount);
		if (m_dashCooldownTimer > 0.0f)
		{
			ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Cooldown: %.1f sec", m_dashCooldownTimer);
		}
		if (m_isDashing)
		{
			ImGui::Text("Dash Progress: %.0f%%", (m_dashElapsedTime / m_dashDuration) * 100.0f);
		}

		// ?? ??
		ImGui::Separator();
		ImGui::Text("Shooting:");
		ImGui::DragFloat("Fire Rate (sec)", &m_fireRate, 0.2f, 0.01f, 2.0f);
		ImGui::DragFloat("Bullet Speed", &m_bulletSpeed, 1.0f, 1.0f, 100.0f);
		ImGui::DragFloat("Bullet Lifetime", &m_bulletLifetime, 3.0f, 0.5f, 10.0f);

		// ?? ??
		ImGui::Separator();
		ImGui::Text("Execution:");
		ImGui::DragFloat("Execution Range", &m_executionRange, 0.5f, 1.0f, 50.0f);

		// ????? ??
		ImGui::Separator();
		ImGui::Text("Animation Names:");
		ImGui::InputText("Idle", &m_animName_Idle);
		ImGui::InputText("WalkForward", &m_animName_WalkForward);
		ImGui::InputText("WalkBackward", &m_animName_WalkBackward);
		ImGui::InputText("Fire", &m_animName_Fire);

		// ?? ??
		ImGui::Separator();
		ImGui::Text("References:");
		ImGui::InputText("AimPointer Object", &m_aimPointerObjectName);

		// ??
		ImGui::Separator();
		ImGui::Checkbox("Enable Upper Body Aim", &m_enableUpperBodyAim);

		ImGui::Separator();
		ImGui::Text("Runtime Info:");
		ImGui::Text("Animation State: %s", GetAnimationState().c_str());
		ImGui::Text("Is Moving Backward: %s", m_isBackward ? "Yes" : "No");

		ImGui::Unindent();

		// BaseControllerScript::OnGui()는 호출하지 않음
		// - Movement 관련 파라미터는 위에서 직접 통합 표시
		// - 몬스터는 BaseControllerScript::OnGui()를 그대로 사용

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

		// ????? ?? ??
		j["AnimName_Idle"] = m_animName_Idle;
		j["AnimName_WalkForward"] = m_animName_WalkForward;
		j["AnimName_WalkBackward"] = m_animName_WalkBackward;
		j["AnimName_Fire"] = m_animName_Fire;

		// 처형 설정
		j["ExecutionRange"] = m_executionRange;

		// 충돌 반응 설정
		j["CollisionPushBackForce"] = m_collisionPushBackForce;
		j["SlidingSpeedMultiplier"] = m_slidingSpeedMultiplier;

		// 대쉬 설정 (Dynamic Impulse + 지수 감쇠)
		j["DashDuration"] = m_dashDuration;
		j["DashImpulseMultiplier"] = m_dashImpulseMultiplier;
		j["DashDecayRate"] = m_dashDecayRate;
		j["DashCooldown"] = m_dashCooldown;
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

		// ????? ?? ??
		if (j.contains("AnimName_Idle"))
			m_animName_Idle = j["AnimName_Idle"].get<std::string>();
		if (j.contains("AnimName_WalkForward"))
			m_animName_WalkForward = j["AnimName_WalkForward"].get<std::string>();
		if (j.contains("AnimName_WalkBackward"))
			m_animName_WalkBackward = j["AnimName_WalkBackward"].get<std::string>();
		if (j.contains("AnimName_Fire"))
			m_animName_Fire = j["AnimName_Fire"].get<std::string>();

		// ?? ??
		if (j.contains("ExecutionRange"))
			m_executionRange = j["ExecutionRange"].get<float>();

		// 충돌 반응 설정
		if (j.contains("CollisionPushBackForce"))
			m_collisionPushBackForce = j["CollisionPushBackForce"].get<float>();
		if (j.contains("SlidingSpeedMultiplier"))
			m_slidingSpeedMultiplier = j["SlidingSpeedMultiplier"].get<float>();

		// 대쉬 설정 (Dynamic Impulse + 지수 감쇠)
		if (j.contains("DashDuration"))
			m_dashDuration = j["DashDuration"].get<float>();
		if (j.contains("DashImpulseMultiplier"))
			m_dashImpulseMultiplier = j["DashImpulseMultiplier"].get<float>();
		if (j.contains("DashDecayRate"))
			m_dashDecayRate = j["DashDecayRate"].get<float>();
		if (j.contains("DashCooldown"))
			m_dashCooldown = j["DashCooldown"].get<float>();
	}
}
