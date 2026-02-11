#include "GamePCH.h"
#include "PlayerControllerScript.h"

#include <algorithm>  // std::remove_if
#include <cmath>      // expf

#include "Script/AimModeController.h"
#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/CharacterScript/Player/BulletPlayer.h"
#include "Script/CharacterScript/Common/ExecutionExitDamageTrigger.h"
#include "Manager/PlayerTemperManager.h"
#include "Manager/StageManager.h"
#include "Manager/BuffManager.h"

#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Object/Component/Camera.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Framework/Asset/Prefab.h>
#include <Framework/System/SystemManager.h>
#include <Engine/Core/System/Input.h>
#include <Engine/Core/System/MyTime.h>
#include <Engine/Framework/Object/Component/Renderer/AfterimageRenderer.h>
#include <Engine/Framework/Object/Component/Renderer/SkeletalMeshRenderer.h>
#include <Engine/Framework/Object/Component/Animator/SkeletalAnimator.h>


namespace game
{
	// ═══════════════════════════════════════════════════════════════
	// 초기화
	// ═══════════════════════════════════════════════════════════════
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

		// LogicFSM 초기화 (한 번만)
		if (!m_fsmInitialized && m_logicFSM)
		{
			InitializeFSM();
			m_fsmInitialized = true;
		}

		game::PlayerTemperManager::ApplyTemper(this);

		// 스테이지 간 유지되는 프레자일 게이지 복원
		float savedFragileGauge = game::StageManager::Get().GetSavedFragileGauge();
		if (savedFragileGauge > 0.0f)
		{
			m_fragileGaugeCurrent = savedFragileGauge;
		}

		// 플레이 씬 시작 시 현재 플레이어의 MaxHP 기준으로 RunHP를 초기화한다.
		StageManager::Get().ResetRunHp(m_PlayerMaxHP);
		m_PlayerCurrentHP = StageManager::Get().GetRunHP();
		m_PlayerCurrentHP = std::clamp(m_PlayerCurrentHP, 0.0f, m_PlayerMaxHP);


		// sound notify 바인딩
		auto animMesh = engine::GameObject::Find("PlayerAnimMesh");
		if (!animMesh) return;
		auto animator = animMesh->GetComponent<engine::SkeletalAnimator>();
		if (!animator) return;

		animator->BindNotify("Event_LeftFootStep", [this]()
			{
				engine::SoundSystem::Get().Play("Player_FootStep_Left_Random", "SFX/Player", true);
			});

		animator->BindNotify("Event_RightFootStep", [this]()
			{
				engine::SoundSystem::Get().Play("Player_FootStep_Right_Random", "SFX/Player", true);
			});
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

		// ─────────────────────────────────────────────
		// AimPointer: 마우스 조준 방향 계산용
		// ─────────────────────────────────────────────
		m_aimPointer = GetGameObject()->GetComponent<AimModeController>();
		if (!m_aimPointer)
		{
			auto* scene = engine::SceneManager::Get().GetScene();
			if (scene)
			{
				// 설정된 이름으로 씬에서 검색
				if (auto* aimGO = scene->FindGameObject(m_aimPointerObjectName))
				{
					m_aimPointer = aimGO->GetComponent<AimModeController>();
				}
				// 이름에 "AimPointer"가 포함된 오브젝트 검색 (폴백)
				if (!m_aimPointer)
				{
					for (const auto& go : scene->GetGameObjects())
					{
						if (go && go->GetName().find("AimPointer") != std::string::npos)
						{
							m_aimPointer = go->GetComponent<AimModeController>();
							if (m_aimPointer) break;
						}
					}
				}
			}
		}

		// ─────────────────────────────────────────────
		// BulletFactory: 총알 생성 팩토리
		// ─────────────────────────────────────────────
		m_bulletFactory = GetGameObject()->GetComponent<BulletFactory>();
	}

	// ═══════════════════════════════════════════════════════════════
	// 행동 제한 (상태 기반 체크)
	// ═══════════════════════════════════════════════════════════════
	bool PlayerControllerScript::CanMove() const
	{
		if (m_isControlLocked)
		{
			return false;
		}

		// 처형 중에는 이동 불가
		if (IsInState("Execution"))
		{
			return false;
		}
		// 대쉬 중에는 일반 이동 불가 (대쉬 전용 처리 사용)
		if (IsInState("Dash"))
		{
			return false;
		}
		return true;
	}

	bool PlayerControllerScript::CanAttack() const
	{
		if (m_isControlLocked)
		{
			return false;
		}

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
	// 입력 처리 (키보드/마우스 → FSM 트리거)
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::ProcessInput()
	{
		if (!m_logicFSM) return;

		if (m_isControlLocked)
		{
			engine::Vector3 zero = engine::Vector3::Zero;
			m_inputMoveDir = zero;
			UpdateLogicalMoveExistence(zero);
			m_logicFSM->SetParameter("IsShooting", false);
			return;
		}

		// Dash 진입 프레임에서 감지된 비정상 케이스는 다음 프레임 입력 단계에서 탈출 트리거를 건다.
		// (같은 프레임 UpdateFSM의 Trigger reset에 먹히는 문제 회피)
		if (m_dashForceExitPending)
		{
			m_logicFSM->SetTrigger("DashComplete");
			m_dashForceExitPending = false;
		}

		// Execution 상태에서는 입력 무시
		if (IsInState("Execution"))
		{
			return;
		}

		// Dash 상태에서는 입력 무시 (대쉬 중 방향 고정)
		if (IsInState("Dash"))
		{
			return;
		}

		// ─────────────────────────────────────────────
		// 1. 이동 입력 → 논리적 이동 벡터 갱신
		// m_isMoving 판정은 UpdateGameLogic()에서 타이머 기반으로 처리
		// ─────────────────────────────────────────────
		m_inputMoveDir = GetMoveInputDirection();
		engine::Vector3 inputDir = m_inputMoveDir;

		// BaseControllerScript의 논리적 이동 벡터 갱신
		UpdateLogicalMoveExistence(inputDir);	

		// ─────────────────────────────────────────────
		// 2. 공격 입력 → FSM 트리거
		// ─────────────────────────────────────────────
		// Dead 상태에서는 입력 무시
		if (IsInState("Dead"))
			return;

		bool isMousePressed = engine::Input::IsMousePressed(engine::Input::Buttons::LEFT);
		bool isMouseHeld = engine::Input::IsMouseHeld(engine::Input::Buttons::LEFT);

		// Pressed 시 트리거 (발사 시작)
		if (isMousePressed)
		{
			m_logicFSM->SetTrigger("Attack");
		}
		// Walk/Idle인데 Fire를 계속 누르고 있으면 Attack 트리거 (대시 종료 후 등에서 발사 재개)
		{
			std::string state = m_logicFSM->GetCurrentState();
			if ((state == "Walk" || state == "Idle") && isMouseHeld)
				m_logicFSM->SetTrigger("Attack");
		}

		// Held 시 파라미터 (연속 발사). 한 번 클릭 시: 손 떼도 1발 나갈 때까지 IdleShoot 유지
		{
			std::string state = m_logicFSM->GetCurrentState();
			bool inShootState = (state == "IdleShoot" || state == "WalkShoot");
			bool keepForFirstShot = inShootState && !m_hasFiredThisSession;
			m_logicFSM->SetParameter("IsShooting", isMouseHeld || keepForFirstShot);
		}
		// Idle/Walk로 돌아왔을 때 다음 클릭을 위해 세션 플래그 리셋
		{
			std::string state = m_logicFSM->GetCurrentState();
			if (state != "IdleShoot" && state != "WalkShoot")
				m_hasFiredThisSession = false;
		}

		// ─────────────────────────────────────────────
		// 3. 대쉬 입력 (Shift/Space)
		// - 이동 중일 때만 대쉬 가능
		// - 쿨다운이 0일 때만 대쉬 가능
		// - 대쉬 카운트가 0보다 클 때만 대쉬 가능
		// ─────────────────────────────────────────────
		bool isShiftPressed = engine::Input::IsKeyPressed(engine::Keys::LeftShift);
		bool isSpaceBarPressed = engine::Input::IsKeyPressed(engine::Keys::Space);
		bool hasDashDirection = m_inputMoveDir.LengthSquared() >= 0.0001f;

		bool isDashAble = m_logicFSM->GetCurrentState() != "Execution" && m_logicFSM->GetCurrentState() != "Dash" && m_logicFSM->GetCurrentState() != "Dead";

		if (isDashAble && (isShiftPressed || isSpaceBarPressed) && hasDashDirection && m_dashCooldownTimer <= 0.0f && m_CurrentDashCount > 0)
		{
			// 대쉬 상태 전이
			m_logicFSM->SetTrigger("StartDash");
			StartDash(m_inputMoveDir);

			// 대쉬 사운드
			engine::SoundSystem::Get().Play("Player_Dash", "SFX/Player", true);
		}
		else if ((isShiftPressed || isSpaceBarPressed) && m_CurrentDashCount <= 0)
		{
			// 대쉬 없을 때 사운드
			engine::SoundSystem::Get().Play("Player_Dash_None", "SFX/Player", true);
		}
	}


	// ═══════════════════════════════════════════════════════════════
	// 게임 로직 - 논리적 처리 (타이머, 회전, 애니메이션)
	// Update()에서 호출됨 (DeltaTime 사용)
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::UpdateGameLogic()
	{
		float deltaTime = engine::Time::DeltaTime();

		// 피격 무적 타이머 갱신
		if (m_invincibleRemainTime > 0.0f)
		{
			m_invincibleRemainTime -= deltaTime;
			if (m_invincibleRemainTime < 0.0f)
			{
				m_invincibleRemainTime = 0.0f;
			}
		}
		
		// ─────────────────────────────────────────────
		// 버프 타이머 업데이트 (BuffManager가 관리, 타임스케일 영향 받음)
		// ─────────────────────────────────────────────
		BuffManager::Update(deltaTime);
		
		// ─────────────────────────────────────────────
		// Idle 전이 대기 타이머 (m_isMoving 판정)
		// - 입력 있음: 즉시 isMoving = true, 타이머 리셋
		// - 입력 없음: 타이머 감소 → 0 도달 시 isMoving = false
		// ─────────────────────────────────────────────
		bool hasInput = m_currentLogicalMoveVector.LengthSquared() > 0.0001f;
		
		if (hasInput)
		{
			// 입력이 있으면 즉시 이동 상태로, 타이머 리셋
			m_isMoving = true;
			m_IdleDransitionWaitTimer = 0.0f;
		}
		else
		{
			// 입력이 없으면 타이머 증가
			if (m_isMoving)
			{
				m_IdleDransitionWaitTimer += deltaTime;
				
				// 타이머가 대기 시간을 초과하면 정지 상태로 전환
				if (m_IdleDransitionWaitTimer >= m_IdleDransitionWaitTime)
				{
					m_isMoving = false;
				}
			}
		}
		
		// ─────────────────────────────────────────────
		// 사망 조건: HP 0 또는 프레자일 게이지 만땅 시 Dead 전이
		// ─────────────────────────────────────────────
		if (m_logicFSM && !IsInState("Dead"))
		{
			if (m_PlayerCurrentHP <= 0.0f || m_fragileGaugeCurrent >= m_fragileGaugeMax)
				m_logicFSM->SetTrigger("Die");
		}

		// FSM 파라미터 갱신
		if (m_logicFSM)
		{
			m_logicFSM->SetParameter("IsMoving", m_isMoving);
			
			if (m_isMoving)
			{
				m_logicFSM->SetParameter("MoveX", m_currentLogicalMoveVector.x);
				m_logicFSM->SetParameter("MoveZ", m_currentLogicalMoveVector.z);
			}
		}
		
		if (m_aimPointer)
		{
			m_aimPointer->SetMoving(m_isMoving);
		}
		
		// 입력과 위치 갱신
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
		// 대쉬 쿨다운 타이머 갱신
		// ─────────────────────────────────────────────
		if (m_dashCooldownTimer > 0.0f)
		{
			m_dashCooldownTimer -= deltaTime;
		}

		// ─────────────────────────────────────────────
		// 대쉬 후 즉시 발사 타이머 갱신
		// ─────────────────────────────────────────────
		if (m_postDashQuickFireTimer > 0.0f)
		{
			m_postDashQuickFireTimer -= deltaTime;
		}

		// ─────────────────────────────────────────────
		// Dash 상태 타임아웃 안전장치
		// - m_dashDuration과 별개로 Dash 상태 고착을 막기 위한 상한
		// ─────────────────────────────────────────────
		if (IsInState("Dash"))
		{
			m_dashStateElapsedTime += deltaTime;
			if (m_dashStateElapsedTime >= m_dashMaxDurationLimit)
			{
				if (m_isDashing)
				{
					EndDash();
				}
				else if (m_logicFSM)
				{
					m_logicFSM->SetTrigger("DashComplete");
				}
				m_dashStateElapsedTime = 0.0f;
				m_dashForceExitPending = false;
			}
		}
		else
		{
			m_dashStateElapsedTime = 0.0f;
		}

		// ─────────────────────────────────────────────
		// 대쉬 충전 시스템
		// - 사용한 대쉬가 있으면 충전 시작
		// - 타이머가 0이 되면 대쉬 1개 회복 후 타이머 리셋
		// - 최대치에 도달하면 충전 대기
		// ─────────────────────────────────────────────
		if (m_CurrentDashCount < m_MaxDashCount)
		{
			m_dashRechargeTimer -= deltaTime;
			if (m_dashRechargeTimer <= 0.0f)
			{
				m_CurrentDashCount++;
				if (m_CurrentDashCount < m_MaxDashCount)
				{
					// 아직 최대가 아니면 타이머 리셋
					m_dashRechargeTimer = m_dashRechargeTime;
				}
				else
				{
					// 최대치에 도달하면 타이머 정지
					m_dashRechargeTimer = m_dashRechargeTime;
				}
			}
		}

		// ─────────────────────────────────────────────
		// 프레자일 게이지 (몬스터가 있을 때만 상승)
		// ─────────────────────────────────────────────
		if (StageManager::Get().ShouldFragileGaugeRise())
		{
			m_fragileGaugeCurrent += m_fragileGaugeRisePerSecond * deltaTime;
			if (m_fragileGaugeCurrent > m_fragileGaugeMax)
				m_fragileGaugeCurrent = m_fragileGaugeMax;
		}

		if (m_isControlLocked)
		{
			m_isMoving = false;
			m_currentLogicalMoveVector = engine::Vector3::Zero;
			return;
		}

		// Execution 상태에서는 이후 로직 스킵
		if (IsInState("Execution"))
		{
			return;
		}
		// Dead 상태에서는 이후 로직 스킵 (이동/공격/대쉬 불가)
		if (IsInState("Dead"))
		{
			return;
		}
		// 잔상: 대쉬 이동 구간에서만 녹화. 감속 구간(이동 종료 직전)에는 녹화하지 않아 종료 지점에 잔상이 몰리지 않게 함
		if (IsInState("Dash") && m_afterimage && m_dashElapsedTime < m_dashDuration * m_dashAfterimageCutoffRatio)
		{
			m_afterimage->RecordSample();
		}
		

		// 발사 로직 처리 (쿨다운, 총알 생성 등)
		if (CanAttack())  HandleShooting(deltaTime);
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
		// Dead 상태에서는 물리 로직 스킵
		if (IsInState("Dead"))
		{
			m_frameCollisionNormals.clear();
			return;
		}

		if (m_isControlLocked)
		{
			if (m_rigidbody && m_rigidbody->IsDynamic())
			{
				m_rigidbody->SetLinearVelocity(engine::Vector3::Zero);
				m_rigidbody->SetAngularVelocity(engine::Vector3::Zero);
			}
			m_frameCollisionNormals.clear();
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
		// 대쉬 감속 처리 (FSM 상태와 무관하게 동작)
		// - Dash 상태가 아니어도 감속 중이면 처리
		// - 최종 이동속도까지 자연스럽게 감속
		// ═══════════════════════════════════════════════════════════════
		if (m_isDashDecaying)
		{
			HandleDashDecay();
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
		// - 버프 배율 적용 (BuffManager에서 가져옴)
		// ═══════════════════════════════════════════════════════════════
		if (CanMove())
		{
			float buffMultiplier = BuffManager::GetMoveSpeedMultiplier();
			float effectiveSpeed = m_temperFinal.moveSpeed * buffMultiplier;
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
		// 상태 전이 시 초기화 처리
		// 각 상태에 필요한 설정 수행

		if (state == "Dash")
		{
			// ═══════════════════════════════════════════════════════════════
			// Dash 상태 진입 시 안전 장치
			// - FSM은 Dash인데 m_isDashing이 false인 경우 (레이스 컨디션)
			// - StartDash()가 early return했지만 FSM 트리거는 발동된 경우
			// ═══════════════════════════════════════════════════════════════
			if (!m_isDashing)
			{
				// 버그 상황 감지: 같은 프레임 즉시 트리거 대신 다음 프레임에서 지연 탈출
				m_dashForceExitPending = true;
			}			

		}
		else if (state == "Execution")
		{
			// 처형 상태 진입 시 속도 초기화
			m_currentVelocity = engine::Vector3::Zero;
			
			// Dynamic Rigidbody 속도도 초기화
			if (m_rigidbody && m_rigidbody->IsDynamic())
			{
				m_rigidbody->SetLinearVelocity(engine::Vector3::Zero);
			}

			// 대쉬 중이었다면 대쉬 완전 종료
			if (m_isDashing)
			{
				EndDash();
			}
			
			// 감속 중이었다면 감속 중단
			if (m_isDashDecaying)
			{
				m_isDashDecaying = false;
			}
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// 상태 이탈 콜백
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::OnStateExited(const std::string& state)
	{
		if (state == "Dash")
		{
			// ═══════════════════════════════════════════════════════════════
			// Dash 상태 이탈 시 강제 정리 (안전 장치)
			// - EndDash()가 호출되지 않은 경우를 대비
			// - FSM 상태는 빠져나왔는데 m_isDashing이 true인 경우 처리
			// ═══════════════════════════════════════════════════════════════
			if (m_isDashing)
			{
				m_isDashing = false;
				m_dashElapsedTime = 0.0f;
				m_dashStateElapsedTime = 0.0f;
				m_dashForceExitPending = false;
				m_dashCooldownTimer = m_temperFinal.dashCooldown;
				
				// 잔상 녹화 종료
				if (m_afterimage)
				{
					m_afterimage->EndRecording();
				}
				
				// 대시 종료 트리거 전달 (BuffManager)
				BuffManager::OnDashEnded();
				
				// 대쉬 후 즉시 발사 시스템 활성화 (EndDash()를 거치지 않은 경우)
				m_postDashQuickFireTimer = m_postDashQuickFireDuration;
				
				// 감속 시작 (EndDash()를 거치지 않은 경우)
				if (!m_isDashDecaying)
				{
					StartDashDecay();
				}
			}
		}
	else if (state == "Execution")
	{
		// ═══════════════════════════════════════════════════════════════
		// 처형 완료 트리거 전달 (BuffManager가 버프 처리)
		// ═══════════════════════════════════════════════════════════════
		BuffManager::OnExecutionCompleted();
		
		// ═══════════════════════════════════════════════════════════════
		// 처형 완료: AimModeController에 처형 후 타이머 시작 (0.5초 유지)
		// ═══════════════════════════════════════════════════════════════
		if (m_aimPointer)
		{
			m_aimPointer->SetExecutionInProgress(false);
			m_aimPointer->m_postExecutionTimer = m_aimPointer->m_postExecutionDuration;
		}
		
		// ═══════════════════════════════════════════════════════════════
		// 처형 종료 데미지 (범위 공격)
		// ═══════════════════════════════════════════════════════════════
		if (m_EED_enable)
		{
			SpawnExecutionExitDamage();
		}
	}
	}

	float PlayerControllerScript::GetExecutionAnimNormalizedTime() const
	{
		if (!IsInState("Execution")) return -1.0f;
		engine::Scene* scene = engine::SceneManager::Get().GetScene();
		if (!scene || m_playerAnimMeshObjectName.empty()) return -1.0f;
		engine::GameObject* meshGO = scene->FindGameObject(m_playerAnimMeshObjectName);
		if (!meshGO) return -1.0f;
		engine::SkeletalAnimator* animator = meshGO->GetComponent<engine::SkeletalAnimator>();
		if (!animator) return -1.0f;
		return animator->GetNormalizedTime(0);
	}

	// ═══════════════════════════════════════════════════════════════
	// 처형 종료 데미지 생성
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::SpawnExecutionExitDamage()
	{
		auto* scene = engine::SceneManager::Get().GetScene();		

		// ExecutionExitDamageTrigger 프리팹 로드 및 인스턴시에이트
		auto triggerGO = engine::Prefab::Instantiate("ExecutionExitDamageTrigger");
		

		// 위치 설정: 플레이어 XZ 좌표 사용, Y는 프리팹 자체 높이 유지
		engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
		engine::Vector3 currentPos = triggerGO->GetTransform()->GetLocalPosition();
		engine::Vector3 spawnPos(playerPos.x, currentPos.y, playerPos.z);  // Y는 프리팹 값 유지
		triggerGO->GetTransform()->SetLocalPosition(spawnPos);
		

		// ExecutionExitDamageTrigger 스크립트 설정
		if (auto* triggerScript = triggerGO->GetComponent<ExecutionExitDamageTrigger>())
		{
			// damage, radiusScale, lifetime 전달
			triggerScript->Setup(m_EED_damage, m_EED_damageScale, m_EED_duration);
			
		}
		else
		{
			LOG_ERROR("[PCS] SpawnExecutionExitDamage: ExecutionExitDamageTrigger script not found on prefab!");
		}
	}

	void PlayerControllerScript::StartExecution(engine::GameObject* targetMonster)
	{
		if (!targetMonster || !m_logicFSM) return;

		// ExecuteMonster 트리거로 Execution 상태 전이 (순간이동·처형은 ExecutionIndicatorManager가 애니 재생 비율 도달 후 수행)
		m_logicFSM->SetTrigger("ExecuteMonster");
	}


	// ═══════════════════════════════════════════════════════════════
	// FSM 초기화
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::InitializeFSM()
	{
		if (!m_logicFSM || m_fsmInitialized) return;

		m_logicFSM->ClearStates();

		// ─────────────────────────────────────────────
		// 상태 등록
		// ─────────────────────────────────────────────
		AddFSMState("Idle", true);   // 기본 상태
		AddFSMState("Walk");
		AddFSMState("IdleShoot");
		AddFSMState("WalkShoot");
		AddFSMState("Dash");         // 대쉬 상태
		AddFSMState("Execution");    // 처형 상태
		AddFSMState("Dead");         // 사망 상태

		m_logicFSM->UpdateStateMap();
		m_logicFSM->SetDefaultState("Idle");
		m_logicFSM->InitializeCurrentState();

		// ─────────────────────────────────────────────
		// 전이 등록
		// ─────────────────────────────────────────────
		// Idle <-> Walk (이동 여부)
		AddFSMTransition("Idle", "Walk", "IsMoving", BoolTrue());
		AddFSMTransition("Walk", "Idle", "IsMoving", BoolFalse());

		// Idle/Walk -> Shoot (공격 트리거)
		AddFSMTransition("Idle", "IdleShoot", "Attack", Trigger());
		AddFSMTransition("Walk", "WalkShoot", "Attack", Trigger());

		// IdleShoot <-> WalkShoot (이동 여부)
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

		// Dash -> Walk (대쉬 완료)
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

		// ─────────────────────────────────────────────
		// 사망 전이 (모든 상태 -> Dead)
		// ─────────────────────────────────────────────
		AddFSMTransition("Idle", "Dead", "Die", Trigger());
		AddFSMTransition("Walk", "Dead", "Die", Trigger());
		AddFSMTransition("IdleShoot", "Dead", "Die", Trigger());
		AddFSMTransition("WalkShoot", "Dead", "Die", Trigger());
		AddFSMTransition("Dash", "Dead", "Die", Trigger());
		AddFSMTransition("Execution", "Dead", "Die", Trigger());
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
	// 대쉬 처리 (Dynamic Rigidbody + Impulse)
	// ═══════════════════════════════════════════════════════════════
void PlayerControllerScript::StartDash(const engine::Vector3& moveDirInput)
	{
		// 입력 방향이 없으면 대쉬 불가
		engine::Vector3 moveDir = moveDirInput;
		if (moveDir.LengthSquared() < 0.0001f)
		{
			// 이동 입력이 없으면 대쉬 취소 (안전 장치)
			return;
		}
		
		m_afterimage->BeginRecording();

		moveDir.y = 0.0f;
		moveDir.Normalize();

		m_dashDirection = moveDir;

		engine::Vector3 dashStartSpeed = m_dashDirection * m_dashInitSpeed;
		m_rigidbody->SetLinearVelocity(dashStartSpeed);

		m_isDashing = true;
		m_dashElapsedTime = 0.0f;
		m_dashStateElapsedTime = 0.0f;
		m_dashForceExitPending = false;

		
		// ═══════════════════════════════════════════════════════════════
		// 감속 시스템 초기화
		// - 대쉬 시작 시 이전 감속 중단
		// ═══════════════════════════════════════════════════════════════
		m_isDashDecaying = false;
		m_dashDecayElapsedTime = 0.0f;
		m_dashDecayStartSpeed = 0.0f;

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
			float dashImpulse = m_dashInitSpeed * m_temperFinal.dashDistance;
			engine::Vector3 impulseForce = m_dashDirection * dashImpulse;
			
			m_rigidbody->AddForce(impulseForce, engine::ForceMode::Impulse);
		}
	}

	void PlayerControllerScript::EndDash()
	{
		m_afterimage->EndRecording();

		m_isDashing = false;
		m_dashElapsedTime = 0.0f;
		m_dashStateElapsedTime = 0.0f;
		m_dashForceExitPending = false;
		m_dashCooldownTimer = m_temperFinal.dashCooldown;

		// FSM 상태 전이 (Dash → Walk)
		if (m_logicFSM)
		{
			m_logicFSM->SetTrigger("DashComplete");
		}

		// ═══════════════════════════════════════════════════════════════
		// 대시 종료 트리거 전달 (BuffManager가 버프 처리)
		// ═══════════════════════════════════════════════════════════════
		BuffManager::OnDashEnded();
		
		// ═══════════════════════════════════════════════════════════════
		// 대쉬 후 즉시 발사 시스템 활성화 (0.3초간 1회만 가능)
		// ═══════════════════════════════════════════════════════════════
		m_postDashQuickFireTimer = m_postDashQuickFireDuration;
		
		// ═══════════════════════════════════════════════════════════════
		// 감속 시작 (FSM 전이와 독립적)
		// ═══════════════════════════════════════════════════════════════
		StartDashDecay();
	}

	// ═══════════════════════════════════════════════════════════════
	// 대쉬 처리 (타이머만 관리, 감속은 별도 처리)
	// - StartDash()에서 Impulse 적용됨
	// - 타이머 종료 시 FSM 전이만 수행
	// - 감속은 StartDashDecay()로 시작하여 별도 진행
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
		
		// 회전 속도 강제 0 (대쉬 중에도 적용)
		ForceStopRotation();
	}
	
	// ═══════════════════════════════════════════════════════════════
	// 대쉬 감속 시작
	// - EndDash()에서 호출
	// - 현재 속도를 저장하고 감속 시작
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::StartDashDecay()
	{
		if (!m_rigidbody || !m_rigidbody->IsDynamic()) return;
		
		// 현재 수평 속도를 감속 시작 속도로 저장
		engine::Vector3 currentVel = m_rigidbody->GetLinearVelocity();
		engine::Vector3 horizontalVel(currentVel.x, 0.0f, currentVel.z);
		m_dashDecayStartSpeed = horizontalVel.Length();
		
		// 감속 시작
		m_isDashDecaying = true;
		m_dashDecayElapsedTime = 0.0f;
	}
	
	// ═══════════════════════════════════════════════════════════════
	// 대쉬 감속 처리
	// - 최종 이동속도(버프 포함)까지만 감속
	// - 지수 감쇠 적용
	// - FSM 상태와 무관하게 동작
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::HandleDashDecay()
	{
		if (!m_isDashDecaying) return;
		if (!m_rigidbody || !m_rigidbody->IsDynamic()) return;
		
		float fixedDelta = engine::Time::FixedDeltaTime();
		m_dashDecayElapsedTime += fixedDelta;
		
		// ═══════════════════════════════════════════════════════════════
		// 목표 속도 계산 (버프 포함)
		// ═══════════════════════════════════════════════════════════════
		float buffMultiplier = BuffManager::GetMoveSpeedMultiplier();
		float targetSpeed = m_temperFinal.moveSpeed * buffMultiplier;
		
		// 현재 속도
		engine::Vector3 currentVel = m_rigidbody->GetLinearVelocity();
		engine::Vector3 horizontalVel(currentVel.x, 0.0f, currentVel.z);
		float currentSpeed = horizontalVel.Length();
		
		// ═══════════════════════════════════════════════════════════════
		// 목표 속도 이하면 감속 종료
		// ═══════════════════════════════════════════════════════════════
		if (currentSpeed <= targetSpeed)
		{
			m_isDashDecaying = false;
			return;
		}
		
		// ═══════════════════════════════════════════════════════════════
		// 지수 감쇠 (시작 속도 → 목표 속도)
		// - t: 0 → 1 (지수 곡선)
		// - newSpeed: startSpeed → targetSpeed
		// ═══════════════════════════════════════════════════════════════
		float progress = m_dashDecayElapsedTime / m_dashDecayDuration;
		float t = 1.0f - expf(-progress * m_dashDecayRate);
		
		// 시작 속도 → 목표 속도로 보간
		float newSpeed = m_dashDecayStartSpeed * (1.0f - t) + targetSpeed * t;
		
		// 목표 속도 이하로 내려가지 않도록 클램프
		if (newSpeed < targetSpeed)
		{
			newSpeed = targetSpeed;
			m_isDashDecaying = false;
		}
		
		// ═══════════════════════════════════════════════════════════════
		// 속도 적용 (방향 유지, 크기만 조정)
		// ═══════════════════════════════════════════════════════════════
		if (horizontalVel.LengthSquared() > 0.0001f)
		{
			engine::Vector3 direction = horizontalVel;
			direction.Normalize();
			engine::Vector3 newVel = direction * newSpeed;
			m_rigidbody->SetLinearVelocity(engine::Vector3(newVel.x, currentVel.y, newVel.z));
		}
	}

	void PlayerControllerScript::HandleShooting(float deltaTime)
	{
		// ─────────────────────────────────────────────
		// 발사 쿨다운 타이머 갱신
		// ─────────────────────────────────────────────
		if (m_fireTimer > 0.0f)
		{
			m_fireTimer -= deltaTime;
		}

		// ─────────────────────────────────────────────
		// 발사 조건: 쿨다운 + 애니 발사 프레임 도달(총 내미는 포즈)
		// - 꾹 누르면: 마우스 홀드 시 발사
		// - 한 번 클릭: 손 떼도 IdleShoot 유지 → 애니 발사 프레임 도달 시 1발만 발사
		// - 버프 적용: m_AtkSpeed * m_buffAtkSpeedMultiplier로 실제 공격속도 계산
		// - 대쉬 후 0.3초: m_canFireNow 체크 우회하여 즉시 발사 가능 (1회만)
		// ─────────────────────────────────────────────
		bool isMouseHeld = engine::Input::IsMouseHeld(engine::Input::Buttons::LEFT);
		std::string shootState = m_logicFSM->GetCurrentState();
		bool inShootState = (shootState == "IdleShoot" || shootState == "WalkShoot");
		bool allowFirstShotWithoutHold = inShootState && !m_hasFiredThisSession && m_canFireNow && m_fireTimer <= 0.0f;
		
		// 대쉬 후 즉시 발사 가능 여부 (1회만, 애니메이션 싱크 무시)
		bool isPostDashQuickFireActive = (m_postDashQuickFireTimer > 0.0f);

		bool doFire = m_fireTimer <= 0.0f && (m_canFireNow || isPostDashQuickFireActive) && (isMouseHeld || allowFirstShotWithoutHold);

		if (doFire)
		{
			// 발사!
			if (m_bulletFactory && m_aimPointer)
			{
				engine::Vector3 playerPos = GetTransform()->GetWorldPosition();
				engine::Vector3 targetPos = m_aimPointer->GetWorldPosition();

				engine::Vector3 bulletStartPos;
				engine::Vector3 direction;

				// 에임 포인터로부터 플레이어 기준의 기본 방향 일단 확보
				engine::Vector3 dirFromPlayer = m_aimPointer->GetDirectionFrom(playerPos);

				// ─────────────────────────────────────────────
				// 총알 발사 위치 계산 (소켓 또는 오프셋)
				// ─────────────────────────────────────────────
				bool useSocket = !m_playerAnimMeshObjectName.empty() && !m_bulletFireSocketName.empty();
				engine::SkeletalMeshRenderer* bulletSocketRenderer = nullptr;

				if (useSocket)
				{
					auto* scene = engine::SceneManager::Get().GetScene();
					engine::GameObject* meshGO = scene ? scene->FindGameObject(m_playerAnimMeshObjectName) : nullptr;
					auto* r = meshGO ? meshGO->GetComponent<engine::SkeletalMeshRenderer>() : nullptr;
					if (r)
					{
						for (const auto& inst : r->GetSocketInstances())
						{
							if (inst.info.name == m_bulletFireSocketName)
							{
								bulletSocketRenderer = r;
								auto effect = engine::Prefab::Instantiate("Effect_Bullet_Fire_V1.00");
								if (effect && effect->GetTransform())
								{
									effect->GetTransform()->SetWorldMatrix(inst.worldMatrix);
									effect->GetTransform()->SetLocalScale(engine::Vector3(1.0f, 1.0f, 1.0f));
								}
								break;
							}
						}
					}
				}

				if (bulletSocketRenderer)
				{
					bulletStartPos = bulletSocketRenderer->GetSocketWorldMatrix(m_bulletFireSocketName).Translation();
				}
				else
				{
					// 소켓을 못 찾았을 경우 Fallback 로직
					bulletStartPos = playerPos + dirFromPlayer * m_bulletStartOffsetForward;
					bulletStartPos.y = playerPos.y + m_bulletStartOffsetY;
				}

				// ─────────────────────────────────────────────
				// 최종 방향 계산 (수평 유지 및 조준점 일치)
				// ─────────────────────────────────────────────
				// 목표 지점의 높이를 발사 지점과 맞춰서 수평 탄도를 만듭니다.
				targetPos.y = bulletStartPos.y;
				direction = targetPos - bulletStartPos;

				if (direction.LengthSquared() > 0.0001f)
				{
					direction.Normalize();
				}
				else
				{
					direction = GetTransform()->GetForward();
				}
				
				// 디버그 표시용: 마지막 실제 발사 XZ 방향 저장
				engine::Vector3 firedXZ = direction;
				firedXZ.y = 0.0f;
				if (firedXZ.LengthSquared() > 0.0001f)
				{
					firedXZ.Normalize();
					m_lastFiredDirectionXZ = firedXZ;
					m_hasLastFiredDirection = true;
				}

				// ─────────────────────────────────────────────
				// BulletParams 설정 및 발사
				// ─────────────────────────────────────────────
				BulletParams params;
				params.type = BulletType::BulletPlayer;
				params.speed = m_temperFinal.bulletSpeed;
				params.range = m_temperFinal.bulletRange;
				params.damage = m_temperFinal.atkDmg;
				params.scale = m_temperFinal.bulletSizeScale;

				m_bulletFactory->Fire(bulletStartPos, direction, params);

				// ─────────────────────────────────────────────
				// 발사 후 상태 관리
				// - 버프 배율 적용: effectiveAtkSpeed = m_AtkSpeed * BuffManager::GetAtkSpeedMultiplier()
				// - 실제 발사 간격: 0.7 / effectiveAtkSpeed
				// ─────────────────────────────────────────────
				for (auto& entry : m_fireCallbacks)
				{
					if (entry.owner && entry.callback) entry.callback();
				}

				// BuffManager에서 버프 배율 가져오기
				float buffMultiplier = BuffManager::GetAtkSpeedMultiplier();
				float effectiveAtkSpeed = m_temperFinal.atkSpeed * buffMultiplier;
				float effectiveFireRate = 0.7f / effectiveAtkSpeed;
				
				m_fireTimer = effectiveFireRate;
				m_hasFiredThisSession = true;
				
				// 대쉬 후 즉시 발사를 사용했다면 타이머 종료 및 다음 발사 준비
				if (m_postDashQuickFireTimer > 0.0f)
				{
					m_postDashQuickFireTimer = 0.0f;
					
					// m_canFireNow = true로 설정하여 0.7초 후 즉시 발사 가능
					// (애니메이션 0.2 지점 대기 없이)
					m_canFireNow = true;
				}
				else
				{
					// 정상 발사: 다음 애니메이션 0.2 지점까지 대기
					m_canFireNow = false;
				}
			}
		}
	}
	
	void PlayerControllerScript::RegisterFireCallback(engine::ScriptBase* owner, const FireCallback& callback)
	{
		FireCallbackEntry entry;
		entry.owner = owner;
		entry.callback = callback;
		m_fireCallbacks.push_back(entry);
	}

	float PlayerControllerScript::GetEffectiveFireRate() const
	{
		const float buffMultiplier = BuffManager::GetAtkSpeedMultiplier();
		const float effectiveAtkSpeed = m_temperFinal.atkSpeed * buffMultiplier;
		if (effectiveAtkSpeed <= 0.0001f)
			return 0.7f;

		return 0.7f / effectiveAtkSpeed;
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
		if (!m_aimPointer)
		{
			isValid = false;
		}
		if (!m_bulletFactory)
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

		ImGui::Text("PlayerControllerScript (Dynamic Rigidbody):");

		// ═══════════════════════════════════════════════════════════════
		// 버프 시스템 상태 표시 (BuffManager 연동) - 최상단 배치
		// ═══════════════════════════════════════════════════════════════
		ImGui::Separator();
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "=== BUFF SYSTEM (BuffManager) ===");
		
		// ─────────────────────────────────────────────
		// 버프 적용 후 최종 스탯 (읽기 전용)
		// ─────────────────────────────────────────────
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Final Stats (with Buffs):");
		ImGui::Indent();
		
		float moveSpeedMultiplier = BuffManager::GetMoveSpeedMultiplier();
		float finalMoveSpeed = m_temperFinal.moveSpeed * moveSpeedMultiplier;
		ImGui::BeginDisabled();
		ImGui::DragFloat("Final Move Speed", &finalMoveSpeed, 0.0f);
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Base: %.2f x Buff: %.2fx = %.2f", m_temperBase.moveSpeed, moveSpeedMultiplier, finalMoveSpeed);
		
		float atkSpeedMultiplier = BuffManager::GetAtkSpeedMultiplier();
		float finalAtkSpeed = m_temperFinal.atkSpeed * atkSpeedMultiplier;
		float finalFireRate = (finalAtkSpeed > 0.001f) ? (0.7f / finalAtkSpeed) : 0.7f;
		ImGui::BeginDisabled();
		ImGui::DragFloat("Final Atk Speed", &finalAtkSpeed, 0.0f);
		ImGui::DragFloat("Final Fire Rate (sec)", &finalFireRate, 0.0f);
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Base: %.2f x Buff: %.2fx = %.2f\nFire Rate: 0.7 / %.2f = %.3f sec", 
				m_temperFinal.atkSpeed, atkSpeedMultiplier, finalAtkSpeed, finalAtkSpeed, finalFireRate);
		
		float atkDamageMultiplier = BuffManager::GetAtkDmgMultiplier();
		float finalAtkDamage = m_temperFinal.atkDmg * atkDamageMultiplier;
		ImGui::BeginDisabled();
		ImGui::DragFloat("Final Atk Damage", &finalAtkDamage, 0.0f);
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Base: %.2f x Buff: %.2fx = %.2f", m_temperBase.atkDmg, atkDamageMultiplier, finalAtkDamage);

		ImGui::Unindent();
		ImGui::Spacing();
		
		// ─────────────────────────────────────────────
		// 대시 버프
		// ─────────────────────────────────────────────
		ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "Dash Buff:");
		ImGui::Indent();
		
		// 설정값 표시 및 수정
		float dashDuration = BuffManager::GetDashBuffDuration();
		if (ImGui::DragFloat("Duration (sec)", &dashDuration, 0.1f, 0.1f, 30.0f))
			BuffManager::SetDashBuffDuration(dashDuration);
			
		float dashBonus = BuffManager::GetDashBuffMoveSpeedBonus();
		if (ImGui::DragFloat("Move Speed Bonus", &dashBonus, 0.01f, 0.0f, 2.0f))
			BuffManager::SetDashBuffMoveSpeedBonus(dashBonus);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("0.1 = 10%% increase, 1.0 = 100%% increase");
		
		// 런타임 상태 표시
		if (BuffManager::IsDashBuffActive())
		{
			float timer = BuffManager::GetDashBuffTimer();
			float multiplier = BuffManager::GetMoveSpeedMultiplier();
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[ACTIVE] Time Left: %.1f sec", timer);
			ImGui::Text("Move Speed Multiplier: %.2fx (%.0f%% increase)", 
				multiplier, (multiplier - 1.0f) * 100.0f);
			float ratio = timer / dashDuration;
			ImGui::ProgressBar(ratio, ImVec2(-1, 0), "");
		}
		else
		{
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[INACTIVE]");
		}
		ImGui::Unindent();
		
		ImGui::Spacing();
		
		// ─────────────────────────────────────────────
		// 처형 버프
		// ─────────────────────────────────────────────
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Execution Buff:");
		ImGui::Indent();
		
		// 설정값 표시 및 수정
		float execDuration = BuffManager::GetExecutionBuffDuration();
		if (ImGui::DragFloat("Buff Duration (sec)", &execDuration, 0.1f, 0.1f, 60.0f))
			BuffManager::SetExecutionBuffDuration(execDuration);
			
		float execBonus = BuffManager::GetExecutionBuffAtkSpeedBonus();
		if (ImGui::DragFloat("Atk Speed Bonus (per stack)", &execBonus, 0.01f, 0.0f, 2.0f))
			BuffManager::SetExecutionBuffAtkSpeedBonus(execBonus);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("0.1 = 10%% increase per stack");
			
		int maxStacks = BuffManager::GetExecutionBuffMaxStacks();
		if (ImGui::DragInt("Max Stacks", &maxStacks, 1, 1, 10))
			BuffManager::SetExecutionBuffMaxStacks(maxStacks);
		
		// 런타임 상태 표시
		if (BuffManager::IsExecutionBuffActive())
		{
			float timer = BuffManager::GetExecutionBuffTimer();
			int stacks = BuffManager::GetExecutionBuffStacks();
			float multiplier = BuffManager::GetAtkSpeedMultiplier();
			ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[ACTIVE] Time Left: %.1f sec", timer);
			ImGui::Text("Stacks: %d / %d", stacks, maxStacks);
			ImGui::Text("Atk Speed Multiplier: %.2fx (%.0f%% increase)", 
				multiplier, (multiplier - 1.0f) * 100.0f);
			float ratio = timer / execDuration;
			ImGui::ProgressBar(ratio, ImVec2(-1, 0), "");
		}
		else
		{
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "[INACTIVE]");
		}
		ImGui::Unindent();

		// ─────────────────────────────────────────────
		// 컴포넌트 검증 (에디터에서 상태 확인용)
		// ─────────────────────────────────────────────
		ImGui::Separator();
		ImGui::Text("=== Component Validation ===");

		// 캐시된 참조 또는 직접 조회 (에디터에서 실시간 확인용)
		engine::Rigidbody* rigidbody = m_rigidbody ? m_rigidbody : (GetGameObject() ? GetGameObject()->GetComponent<engine::Rigidbody>() : nullptr);
		engine::LogicFSM* logicFSM = m_logicFSM ? m_logicFSM : (GetGameObject() ? GetGameObject()->GetComponent<engine::LogicFSM>() : nullptr);
		AimModeController* aimPointer = m_aimPointer ? m_aimPointer : (GetGameObject() ? GetGameObject()->GetComponent<AimModeController>() : nullptr);
		BulletFactory* bulletFactory = m_bulletFactory ? m_bulletFactory : (GetGameObject() ? GetGameObject()->GetComponent<BulletFactory>() : nullptr);

		// 전체 유효성 확인
		bool allValid = rigidbody && aimPointer && bulletFactory && logicFSM;

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

		// 각 컴포넌트 상태 표시
		ImGui::Indent();
		ImGui::Text("AimPointer:        %s", aimPointer ? "[OK]" : "[MISSING]");
		if (!aimPointer) ImGui::SameLine(); if (!aimPointer) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");

		ImGui::Text("BulletFactory:     %s", bulletFactory ? "[OK]" : "[MISSING]");
		if (!bulletFactory) ImGui::SameLine(); if (!bulletFactory) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");

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
		if (ImGui::DragFloat("Base Move Speed", &m_temperBase.moveSpeed, 0.1f, 0.0f, 100.0f))
			PlayerTemperManager::ApplyTemper(this);

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
				ImGui::Text("Current Speed: %.2f / %.2f", horizSpeed, m_temperFinal.moveSpeed);
				
				// 진행바로 속도 표시
				float speedRatio = m_temperFinal.moveSpeed > 0.0f ? horizSpeed / m_temperFinal.moveSpeed : 0.0f;
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
		ImGui::DragFloat("Dash Max Duration Limit (sec)", &m_dashMaxDurationLimit, 0.05f, 0.1f, 3.0f);
		if (ImGui::DragFloat("Base Dash Distance", &m_temperBase.dashDistance, 0.1f, 0.0f, 100.0f))
			PlayerTemperManager::ApplyTemper(this);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Dash impulse = moveSpeed * this value. Higher = faster initial speed");
		}
		ImGui::DragFloat("Dash Decay Rate", &m_dashDecayRate, 0.1f, 0.0f, 10.0f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Exponential decay rate. Higher = faster slowdown");
		}
		ImGui::DragFloat("Dash Decay Duration (sec)", &m_dashDecayDuration, 0.1f, 0.1f, 2.0f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("감속 지속 시간. 이 시간 동안 대쉬 속도 → 최종 이동속도로 감속");
		}
		ImGui::DragFloat("Dash Afterimage Cutoff", &m_dashAfterimageCutoffRatio, 0.05f, 0.2f, 1.0f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("잔상 녹화 구간. 대쉬 시간의 이 비율까지만 녹화 (감속 구간 제외, 0.7=70%%)");
		}
		
		if (ImGui::DragFloat("Dash Cooldown (sec)", &m_temperBase.dashCooldown, 0.1f, 0.0f, 10.0f))
			PlayerTemperManager::ApplyTemper(this);
		ImGui::DragInt("Max Dash Count", &m_MaxDashCount, 1, 1, 10);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Maximum number of dashes available");
		}
		ImGui::DragFloat("Dash Recharge Time (sec)", &m_dashRechargeTime, 0.1f, 0.1f, 10.0f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Time to recharge one dash");
		}
		ImGui::DragFloat("Post-Dash Quick Fire Duration (sec)", &m_postDashQuickFireDuration, 0.05f, 0.0f, 1.0f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("대쉬 후 즉시 발사 가능 시간. 이 시간 안에 1회만 애니메이션 싱크 무시하고 즉시 발사 가능");
		}
		
		// 대쉬 상태 표시
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Runtime Status:");
		ImGui::Text("  FSM State: %s", m_logicFSM ? m_logicFSM->GetCurrentState().c_str() : "N/A");
		ImGui::Text("  Dash Active: %s", m_isDashing ? "YES" : "NO");
		ImGui::Text("  Dash Decaying: %s", m_isDashDecaying ? "YES" : "NO");
		ImGui::Text("  Dash Count: %d / %d", m_CurrentDashCount, m_MaxDashCount);
		
		if (m_dashCooldownTimer > 0.0f)
		{
			ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "  Cooldown: %.2f sec", m_dashCooldownTimer);
		}
		
		if (m_postDashQuickFireTimer > 0.0f)
		{
			ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "  Quick Fire: %.2f sec (Available)", 
				m_postDashQuickFireTimer);
		}
		
		if (m_isDashing)
		{
			float progress = (m_dashDuration > 0.0f) ? (m_dashElapsedTime / m_dashDuration) * 100.0f : 0.0f;
			ImGui::Text("  Dash Progress: %.0f%% (%.2f / %.2f sec)", progress, m_dashElapsedTime, m_dashDuration);
		}
		ImGui::Text("  Dash State Elapsed: %.2f / %.2f sec", m_dashStateElapsedTime, m_dashMaxDurationLimit);
		
		if (m_isDashDecaying)
		{
			float decayProgress = (m_dashDecayDuration > 0.0f) ? (m_dashDecayElapsedTime / m_dashDecayDuration) * 100.0f : 0.0f;
			ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.8f, 1.0f), "  Decay Progress: %.0f%% (%.2f / %.2f sec)", 
				decayProgress, m_dashDecayElapsedTime, m_dashDecayDuration);
			ImGui::Text("  Decay Start Speed: %.2f m/s", m_dashDecayStartSpeed);
			
			// 현재 목표 속도 표시
			float buffMultiplier = BuffManager::GetMoveSpeedMultiplier();
			float targetSpeed = m_temperFinal.moveSpeed * buffMultiplier;
			ImGui::Text("  Target Speed: %.2f m/s (Base: %.2f x Buff: %.2fx)", 
				targetSpeed, m_temperFinal.moveSpeed, buffMultiplier);
		}

		// ═══════════════════════════════════════════════════════════════
		// 공격 변수 - Base값 (편집 가능)
		// ═══════════════════════════════════════════════════════════════
		ImGui::Separator();
		ImGui::Text("=== Attack Stats (Base Values) ===");
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Edit these values. Actual values are calculated by PlayerTemperManager.");
		
		bool baseChanged = false;
		
		if (ImGui::DragFloat("Base Atk Damage", &m_temperBase.atkDmg, 0.5f, 0.0f, 1000.0f))
			baseChanged = true;
		if (ImGui::DragFloat("Base Atk Speed", &m_temperBase.atkSpeed, 0.05f, 0.1f, 10.0f))
			baseChanged = true;
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("1.0 = 1.4 shots/sec (0.7s interval). Higher = faster attack.");
		}
		if (ImGui::DragFloat("Base Bullet Range", &m_temperBase.bulletRange, 1.0f, 1.0f, 200.0f))
			baseChanged = true;
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("BulletPlayer 사거리 (거리 단위). BulletPlayer는 lifetime 대신 range 사용");
		}
		if (ImGui::DragFloat("Base Bullet Size Scale", &m_temperBase.bulletSizeScale, 0.05f, 0.1f, 10.0f))
			baseChanged = true;
		if (ImGui::DragFloat("Base Bullet Speed", &m_temperBase.bulletSpeed, 0.5f, 0.1f, 100.0f))
			baseChanged = true;
		
		// Base값 변경 시 강화 재계산
		if (baseChanged)
		{
			PlayerTemperManager::ApplyTemper(this);
		}
		
		// ═══════════════════════════════════════════════════════════════
		// 공격 변수 - 실제값 (조회만, 강화 적용 후)
		// ═══════════════════════════════════════════════════════════════
		ImGui::Spacing();
		ImGui::Text("=== Attack Stats (Actual Values) ===");
		ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Read-only. Calculated: (Base + Add) x Mul");
		
		ImGui::BeginDisabled();
		ImGui::DragFloat("Actual Atk Damage", &m_temperFinal.atkDmg, 0.0f);
		ImGui::DragFloat("Actual Atk Speed", &m_temperFinal.atkSpeed, 0.0f);
		ImGui::DragFloat("Fire Rate (sec)", &m_temperFinal.fireRate, 0.0f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Calculated: 0.7 / AtkSpeed");
		}
		ImGui::DragFloat("Actual Bullet Range", &m_temperFinal.bulletRange, 0.0f);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("BulletPlayer 사거리 (BulletPlayer는 lifetime 대신 range 사용)");
		}
		ImGui::DragFloat("Actual Bullet Size Scale", &m_temperFinal.bulletSizeScale, 0.0f);
		ImGui::DragFloat("Actual Bullet Speed", &m_temperFinal.bulletSpeed, 0.0f);
		//bool tempDouble = m_isBulletDouble;
		//ImGui::Checkbox("Is Bullet Double", &tempDouble);
		ImGui::EndDisabled();
		
		ImGui::Spacing();
		ImGui::Text("Bullet Start Position:");
		ImGui::InputText("Player Anim Mesh Object", &m_playerAnimMeshObjectName);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("BulletFireSocket이 있는 메쉬 오브젝트 이름 (씬에서 FindGameObject). 기본 PlayerAnimMesh");
		ImGui::InputText("Bullet Fire Socket Name", &m_bulletFireSocketName);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("해당 메쉬의 SkeletalMeshRenderer 소켓 이름. 비우면 아래 오프셋 사용");
		ImGui::TextDisabled("(소켓 미사용 시 아래 오프셋 적용)");
		ImGui::DragFloat("Start Offset Y (height)", &m_bulletStartOffsetY, 0.1f, 0.0f, 10.0f);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Total Y position of bullet spawn (world Y)");
		ImGui::DragFloat("Start Offset Forward", &m_bulletStartOffsetForward, 0.1f, 0.0f, 10.0f);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Offset along firing direction (forward from player)");

	// 처형 설정
	ImGui::Separator();
	ImGui::Text("Execution:");
	ImGui::DragFloat("Execution Range", &m_executionRange, 0.5f, 1.0f, 50.0f);
	
	// 처형 종료 데미지
	ImGui::Separator();
	ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Execution Exit Damage:");
	ImGui::Checkbox("EED_Enable", &m_EED_enable);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("처형 종료 시 주변 적에게 범위 데미지");
	
	if (m_EED_enable)
	{
		ImGui::Indent();
		ImGui::DragFloat("EED_Damage Scale (Radius)", &m_EED_damageScale, 0.5f, 1.0f, 20.0f);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("데미지 범위 (반지름)");
		
		ImGui::DragFloat("EED_Damage", &m_EED_damage, 1.0f, 0.0f, 200.0f);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("적에게 입히는 데미지");
		
		ImGui::DragFloat("EED_Duration (sec)", &m_EED_duration, 0.1f, 0.1f, 5.0f);
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("트리거 지속 시간");
		
		ImGui::Unindent();
	}

	// 참조 설정
		ImGui::Separator();
		ImGui::Text("References:");
		ImGui::InputText("AimPointer Object", &m_aimPointerObjectName);

	// 프레자일 게이지 (몬스터 있을 때 상승, 직렬화·GUI 조정)
	ImGui::Separator();
	ImGui::Text("Fragile Gauge:");
	ImGui::DragFloat("Fragile Gauge Max", &m_fragileGaugeMax, 1.0f, 1.0f, 1000.0f);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("최대치. 몬스터가 있을 때 이 값까지 상승.");
	ImGui::DragFloat("Fragile Gauge Rise/sec", &m_fragileGaugeRisePerSecond, 0.5f, 0.0f, 200.0f);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("몬스터가 있을 때 초당 상승량.");
	float gaugeRatio = m_fragileGaugeMax > 0.0f ? (m_fragileGaugeCurrent / m_fragileGaugeMax) : 0.0f;
	ImGui::Text("Current: %.1f / %.1f", m_fragileGaugeCurrent, m_fragileGaugeMax);
	ImGui::ProgressBar(gaugeRatio, ImVec2(-1, 0), "");

	// HP (Max는 편집 가능, Current는 읽기 전용)
	ImGui::Separator();
	ImGui::Text("HP:");
	ImGui::DragFloat("Player Max HP", &m_PlayerMaxHP, 1.0f, 1.0f, 10000.0f);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("플레이어 최대 체력 (직렬화 저장)");
	ImGui::Text("Player Current HP (ReadOnly): %.1f", m_PlayerCurrentHP);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("런타임 현재 체력 (읽기 전용)");

	// 무적 상태
	ImGui::Separator();
	ImGui::Text("Invincibility:");
	ImGui::Checkbox("Is Invincible", &m_isInvincible);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("무적 상태 (데미지 무시)");
	if (ImGui::DragFloat("Invincible Time On Hit (sec)", &m_temperBase.invincibleTime, 0.05f, 0.0f, 10.0f))
		PlayerTemperManager::ApplyTemper(this);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("피격 후 데미지를 무시하는 무적 시간");
	ImGui::Text("Invincible Time (Applied): %.2f sec", m_invincibleTime);
	ImGui::Text("Invincible Timer Left: %.2f sec", m_invincibleRemainTime);

	ImGui::Separator();
	ImGui::Text("Runtime Info:");
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
		j["MoveSpeed"] = m_temperBase.moveSpeed;
		j["RotationSpeed"] = m_rotationSpeed;
		
		// 공격 변수 - Base값만 저장 (실제값은 강화 적용 후 계산됨)
		j["BaseAtkDmg"] = m_temperBase.atkDmg;
		j["BaseAtkSpeed"] = m_temperBase.atkSpeed;
		j["BaseBulletRange"] = m_temperBase.bulletRange;
		j["BaseBulletSizeScale"] = m_temperBase.bulletSizeScale;
		j["BaseBulletSpeed"] = m_temperBase.bulletSpeed;
		
		j["BulletStartOffsetY"] = m_bulletStartOffsetY;
		j["BulletStartOffsetForward"] = m_bulletStartOffsetForward;
		j["PlayerAnimMeshObjectName"] = m_playerAnimMeshObjectName;
		j["BulletFireSocketName"] = m_bulletFireSocketName;
		j["AimPointerObjectName"] = m_aimPointerObjectName;
		j["FSMInitialized"] = m_fsmInitialized;

		// 처형 설정
		j["ExecutionRange"] = m_executionRange;

		// 충돌 반응 설정
		j["CollisionPushBackForce"] = m_collisionPushBackForce;
		j["SlidingSpeedMultiplier"] = m_slidingSpeedMultiplier;

		// 대쉬 설정 (Dynamic Impulse + 독립 감속)
		j["DashDuration"] = m_dashDuration;
		j["DashMaxDurationLimit"] = m_dashMaxDurationLimit;
		j["BaseDashDistance"] = m_temperBase.dashDistance;
		j["DashDecayRate"] = m_dashDecayRate;
		j["DashDecayDuration"] = m_dashDecayDuration;
		j["DashAfterimageCutoffRatio"] = m_dashAfterimageCutoffRatio;
		j["DashCooldown"] = m_temperBase.dashCooldown;
		j["MaxDashCount"] = m_MaxDashCount;
		j["DashRechargeTime"] = m_dashRechargeTime;
		j["PostDashQuickFireDuration"] = m_postDashQuickFireDuration;

	// 프레자일 게이지
	j["FragileGaugeCurrent"] = m_fragileGaugeCurrent;
	j["FragileGaugeMax"] = m_fragileGaugeMax;
	j["FragileGaugeRisePerSecond"] = m_fragileGaugeRisePerSecond;
	j["PlayerMaxHP"] = m_PlayerMaxHP;
	j["InvincibleTime"] = m_temperBase.invincibleTime;
	
	// 무적 상태
	j["IsInvincible"] = m_isInvincible;
	
	// 처형 종료 데미지
	j["EED_Enable"] = m_EED_enable;
	j["EED_DamageScale"] = m_EED_damageScale;
	j["EED_Damage"] = m_EED_damage;
	j["EED_Duration"] = m_EED_duration;
}

	void PlayerControllerScript::Load(const engine::json& j)
	{
		BaseControllerScript::Load(j);

		if (j.contains("MoveSpeed"))
			m_temperBase.moveSpeed = j["MoveSpeed"].get<float>();
		if (j.contains("RotationSpeed"))
			m_rotationSpeed = j["RotationSpeed"].get<float>();
		
		// 공격 변수 - Base값 로드
		if (j.contains("BaseAtkDmg"))
			m_temperBase.atkDmg = j["BaseAtkDmg"].get<float>();
		if (j.contains("BaseAtkSpeed"))
			m_temperBase.atkSpeed = j["BaseAtkSpeed"].get<float>();
		if (j.contains("BaseBulletRange"))
			m_temperBase.bulletRange = j["BaseBulletRange"].get<float>();
		if (j.contains("BaseBulletSizeScale"))
			m_temperBase.bulletSizeScale = j["BaseBulletSizeScale"].get<float>();
		if (j.contains("BaseBulletSpeed"))
			m_temperBase.bulletSpeed = j["BaseBulletSpeed"].get<float>();
		
		// 하위 호환성: 기존 씬 파일에서 실제값으로 저장된 경우 Base값으로 사용
		if (!j.contains("BaseAtkDmg") && j.contains("FireRate"))
		{
			// 기존 FireRate로부터 AtkSpeed 역산 (0.7 / fireRate)
			float oldFireRate = j["FireRate"].get<float>();
			if (oldFireRate > 0.001f)
				m_temperBase.atkSpeed = 0.7f / oldFireRate;
		}
		if (!j.contains("BaseBulletSpeed") && j.contains("BulletSpeed"))
			m_temperBase.bulletSpeed = j["BulletSpeed"].get<float>();
		if (j.contains("BulletStartOffsetY"))
			m_bulletStartOffsetY = j["BulletStartOffsetY"].get<float>();
		if (j.contains("BulletStartOffsetForward"))
			m_bulletStartOffsetForward = j["BulletStartOffsetForward"].get<float>();
		if (j.contains("PlayerAnimMeshObjectName"))
			m_playerAnimMeshObjectName = j["PlayerAnimMeshObjectName"].get<std::string>();
		if (j.contains("BulletFireSocketName"))
			m_bulletFireSocketName = j["BulletFireSocketName"].get<std::string>();
		if (j.contains("AimPointerObjectName"))
			m_aimPointerObjectName = j["AimPointerObjectName"].get<std::string>();
		if (j.contains("FSMInitialized"))
			m_fsmInitialized = j["FSMInitialized"].get<bool>();

		// 처형 설정
		if (j.contains("ExecutionRange"))
			m_executionRange = j["ExecutionRange"].get<float>();

		// 충돌 반응 설정
		if (j.contains("CollisionPushBackForce"))
			m_collisionPushBackForce = j["CollisionPushBackForce"].get<float>();
		if (j.contains("SlidingSpeedMultiplier"))
			m_slidingSpeedMultiplier = j["SlidingSpeedMultiplier"].get<float>();

		// 대쉬 설정 (Dynamic Impulse + 독립 감속)
		if (j.contains("DashDuration"))
			m_dashDuration = j["DashDuration"].get<float>();
		if (j.contains("DashMaxDurationLimit"))
			m_dashMaxDurationLimit = j["DashMaxDurationLimit"].get<float>();
		if (j.contains("BaseDashDistance"))
			m_temperBase.dashDistance = j["BaseDashDistance"].get<float>();
		// (하위호환 원하면)
		else if (j.contains("DashImpulseMultiplier"))
			m_temperBase.dashDistance = j["DashImpulseMultiplier"].get<float>();
		if (j.contains("DashDecayRate"))
			m_dashDecayRate = j["DashDecayRate"].get<float>();
		if (j.contains("DashDecayDuration"))
			m_dashDecayDuration = j["DashDecayDuration"].get<float>();
		if (j.contains("DashAfterimageCutoffRatio"))
			m_dashAfterimageCutoffRatio = j["DashAfterimageCutoffRatio"].get<float>();
		if (j.contains("DashCooldown"))
			m_temperBase.dashCooldown = j["DashCooldown"].get<float>();
		if (j.contains("MaxDashCount"))
			m_MaxDashCount = j["MaxDashCount"].get<int>();
		if (j.contains("DashRechargeTime"))
			m_dashRechargeTime = j["DashRechargeTime"].get<float>();
		if (j.contains("PostDashQuickFireDuration"))
			m_postDashQuickFireDuration = j["PostDashQuickFireDuration"].get<float>();

	// 프레자일 게이지
	if (j.contains("FragileGaugeCurrent"))
		m_fragileGaugeCurrent = j["FragileGaugeCurrent"].get<float>();
	if (j.contains("FragileGaugeMax"))
		m_fragileGaugeMax = j["FragileGaugeMax"].get<float>();
	if (j.contains("FragileGaugeRisePerSecond"))
		m_fragileGaugeRisePerSecond = j["FragileGaugeRisePerSecond"].get<float>();
	if (j.contains("PlayerMaxHP"))
		m_PlayerMaxHP = j["PlayerMaxHP"].get<float>();
	if (j.contains("InvincibleTime"))
		m_temperBase.invincibleTime = j["InvincibleTime"].get<float>();
	
	// 무적 상태
	if (j.contains("IsInvincible"))
		m_isInvincible = j["IsInvincible"].get<bool>();
	
	// 처형 종료 데미지
	if (j.contains("EED_Enable"))
		m_EED_enable = j["EED_Enable"].get<bool>();
	if (j.contains("EED_DamageScale"))
		m_EED_damageScale = j["EED_DamageScale"].get<float>();
	if (j.contains("EED_Damage"))
		m_EED_damage = j["EED_Damage"].get<float>();
	if (j.contains("EED_Duration"))
		m_EED_duration = j["EED_Duration"].get<float>();
	
	// ═══════════════════════════════════════════════════════════════
	// Base값 로드 완료 후 강화 적용하여 실제값 계산
	// ═══════════════════════════════════════════════════════════════
	PlayerTemperManager::ApplyTemper(this);
}

	// ═══════════════════════════════════════════════════════════════
	// 데미지 처리
	// ═══════════════════════════════════════════════════════════════
	void PlayerControllerScript::TakeDamage(float damage)
	{
		if (m_isInvincible)
		{
			return;
		}
		if (m_invincibleRemainTime > 0.0f)
		{
			return;
		}
		if (damage <= 0.0f) return;

		m_PlayerCurrentHP -= damage;
		if (m_PlayerCurrentHP < 0.0f) m_PlayerCurrentHP = 0.0f;
		m_invincibleRemainTime = std::max(0.0f, m_invincibleTime);

		StageManager::Get().SetRunHP(m_PlayerCurrentHP);

		if (m_onDamaged) m_onDamaged();

		// 사망 전이는 UpdateGameLogic에서 HP 조건으로 Dead 상태 전이 후, SceneController_Play에서 Fail() 호출
	}

	void PlayerControllerScript::SetControlLocked(bool locked)
	{
		m_isControlLocked = locked;

		if (!locked)
		{
			return;
		}

		m_inputMoveDir = engine::Vector3::Zero;
		m_currentLogicalMoveVector = engine::Vector3::Zero;
		m_isMoving = false;
		m_fireTimer = 0.0f;
		m_hasFiredThisSession = false;
		m_postDashQuickFireTimer = 0.0f;

		if (m_logicFSM)
		{
			m_logicFSM->SetParameter("IsShooting", false);
		}

		if (m_isDashing)
		{
			EndDash();
		}

		if (m_rigidbody && m_rigidbody->IsDynamic())
		{
			m_rigidbody->SetLinearVelocity(engine::Vector3::Zero);
			m_rigidbody->SetAngularVelocity(engine::Vector3::Zero);
		}
	}
}
