#include "GamePCH.h"
#include "MonsterScript.h"

#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Transform.h>
#include <Framework/Object/Component/Animator/SkeletalAnimator.h>
#include <Framework/Object/Component/Pathfinding/PathfindingAgent.h>
#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>
#include <Engine/Core/System/MyTime.h>

namespace game
{
	// ═══════════════════════════════════════════════════════════════
	// 생명주기
	// ═══════════════════════════════════════════════════════════════
	void MonsterScript::Awake()
	{
		BaseControllerScript::Awake();
	}

	void MonsterScript::Start()
	{
		BaseControllerScript::Start();

		// 애니메이션 초기화
		InitializeAnimations();

		// 총알 설정 초기화
		InitializeBullet();

		// LogicFSM 초기화
		if (!m_fsmInitialized && m_logicFSM)
		{
			InitializeFSM();
			m_fsmInitialized = true;
		}

		// AnimFSM 초기화
		if (m_animFSM)
		{
			InitializeAnimFSM();
		}

		// Rigidbody 필수 확인
		if (!m_rigidbody)
		{
			LOG_PRINT("[MonsterScript] ERROR: Monster requires Dynamic Rigidbody!");
			return;
		}

		// ─────────────────────────────────────────────
		// Mass 강제 설정 (플레이어와의 충돌 시 밀림 방지)
		// Scene 파일의 값보다 우선 적용됨
		// - 이동하지 않는 몬스터 (moveSpeed == 0): 1500
		// - 이동하는 몬스터: 100
		// ─────────────────────────────────────────────
		float mass = (m_moveSpeed <= 0.0f) ? 15000.0f : 500.0f;
		m_rigidbody->SetMass(mass);
		m_rigidbody->SetAngularDamping(55.0f);
		m_rigidbody->SetLinearDamping(10.0f);

		// Rigidbody 제약 설정 (m_moveSpeed 기준 자동 판단)
		// Dynamic Rigidbody의 예기치 않은 동작(높이 변화, 넘어짐) 방지
		using namespace engine;
		RigidbodyConstraints constraints;

		if (m_moveSpeed <= 0.0f)
		{
			// 이동 불가 몬스터: 위치 완전 고정 (X, Y, Z), 회전은 Y축만 허용
			constraints = RigidbodyConstraints::FreezePosition |
				RigidbodyConstraints::FreezeRotationX |
				RigidbodyConstraints::FreezeRotationZ;
		}
		else
		{
			// 이동 가능 몬스터: 높이(Y) 고정, XZ 이동 가능, 회전은 Y축만 허용
			constraints = RigidbodyConstraints::FreezePositionY |
				RigidbodyConstraints::FreezeRotationX |
				RigidbodyConstraints::FreezeRotationZ;
		}

		m_rigidbody->SetConstraints(constraints);

		// Dynamic Rigidbody 설정 확인
		if (!m_rigidbody->IsDynamic())
		{
			LOG_PRINT("[MonsterScript] WARNING: Monster Rigidbody should be Dynamic!");
		}

		// 플레이어 찾기
		FindPlayer();

		// 첫 발사 쿨타임 설정 (게임 시작 즉시 발사 방지)
		m_fireTimer = m_fireRate;
	}

	// ═══════════════════════════════════════════════════════════════
	// 컴포넌트 캐싱
	// ═══════════════════════════════════════════════════════════════
	void MonsterScript::CacheComponents()
	{
		BaseControllerScript::CacheComponents();

		if (!GetGameObject()) return;

		m_rigidbody = GetGameObject()->GetComponent<engine::Rigidbody>();
		m_skeletalAnimator = GetGameObject()->GetComponent<engine::SkeletalAnimator>();

		// BulletFactory는 자신의 GameObject에서만 검색
		m_bulletFactory = GetGameObject()->GetComponent<BulletFactory>();

		// PathfindingAgent 검색 (같은 GameObject에 있어야 함)
		m_pathfindingAgent = GetGameObject()->GetComponent<engine::PathfindingAgent>();
	}

	// ═══════════════════════════════════════════════════════════════
	// 플레이어 추적
	// ═══════════════════════════════════════════════════════════════
	void MonsterScript::FindPlayer()
	{
		auto* scene = engine::SceneManager::Get().GetScene();
		if (!scene) return;

		// 1. "Player" 이름으로 검색
		if (auto* playerGO = scene->FindGameObject(m_targetPlayerObjectName))
		{
			m_targetPlayer = playerGO->GetComponent<PlayerControllerScript>();
			if (m_targetPlayer) return;
		}

		// 2. PlayerControllerScript 컴포넌트로 검색
		if (!m_targetPlayer)
		{
			for (const auto& go : scene->GetGameObjects())
			{
				if (auto* player = go->GetComponent<PlayerControllerScript>())
				{
					m_targetPlayer = player;
					return;
				}
			}
		}
	}

	float MonsterScript::GetDistanceToPlayer() const
	{
		return GetDistanceToTarget(m_targetPlayer ? m_targetPlayer->GetGameObject() : nullptr);
	}

	engine::Vector3 MonsterScript::CalculateDirectionToPlayer() const
	{
		return CalculateDirectionToTarget(m_targetPlayer ? m_targetPlayer->GetGameObject() : nullptr);
	}

	bool MonsterScript::IsPlayerInRange() const
	{
		return IsTargetInRange(m_targetPlayer ? m_targetPlayer->GetGameObject() : nullptr, m_AttackRange);
	}

	void MonsterScript::RotateTowardsPlayer()
	{
		// FixedUpdate에서 호출됨 - deltaTime 파라미터 불필요
		// (BaseControllerScript::RotateTowardsTarget 내부에서 물리 엔진이 처리)
		RotateTowardsTarget(m_targetPlayer ? m_targetPlayer->GetGameObject() : nullptr, 0.0f);
	}

	bool MonsterScript::IsLookingAtPlayer() const
	{
		return IsLookingAtTarget(m_targetPlayer ? m_targetPlayer->GetGameObject() : nullptr);
	}

	float MonsterScript::GetPathDistanceToPlayer() const
	{
		if (!m_pathfindingAgent || !m_targetPlayer || !m_targetPlayer->GetGameObject())
		{
			return FLT_MAX;
		}

		// PathfindingAgent가 유효한 경로를 가지고 있으면 경로 길이 계산
		if (m_pathfindingAgent->HasPath())
		{
			const auto& path = m_pathfindingAgent->GetPath();
			if (!path.empty())
			{
				float totalDistance = 0.0f;
				engine::Vector3 currentPos = GetTransform()->GetWorldPosition();

				// 현재 위치에서 첫 waypoint까지
				totalDistance += engine::Vector3::Distance(currentPos, path[0]);

				// waypoint 간 거리 합산
				for (size_t i = 1; i < path.size(); ++i)
				{
					totalDistance += engine::Vector3::Distance(path[i - 1], path[i]);
				}

				return totalDistance;
			}
		}

		// 경로가 없으면 직선 거리 반환
		return GetDistanceToPlayer();
	}


	void MonsterScript::Attack(float deltaTime)
	{
		// 기본 구현은 비어있음
		// 자손 클래스에서 오버라이드하여 구현
	}

	// ═══════════════════════════════════════════════════════════════
	// 이동 (PathfindingAgent 활용) - FixedUpdate에서 호출
	// ═══════════════════════════════════════════════════════════════
	void MonsterScript::MoveTowardsPlayer()
	{
		if (!m_pathfindingAgent || !m_rigidbody || !m_targetPlayer || m_moveSpeed <= 0.0f)
		{
			return;
		}

		engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
		engine::Vector3 playerPos = m_targetPlayer->GetTransform()->GetWorldPosition();

		// PathfindingAgent 자동 경로 업데이트 (FixedUpdate 기반 - 즉시 계산)
		float fixedDeltaTime = engine::Time::FixedDeltaTime();
		m_pathfindingAgent->UpdatePathfindingFixed(fixedDeltaTime, playerPos);

		engine::Vector3 nextWaypoint;
		engine::Vector3 direction;

		// waypoint 가져오기 시도
		bool hasValidWaypoint = m_pathfindingAgent->HasPath() && 
		                        m_pathfindingAgent->GetCurrentWaypoint(nextWaypoint);

		if (!hasValidWaypoint)
		{
			// 경로가 없거나 유효한 waypoint가 없으면 직접 이동
			direction = CalculateDirectionToPlayer();
		}
		else
		{
			engine::Vector3 toCurrentWaypoint = nextWaypoint - currentPos;
			toCurrentWaypoint.y = 0.0f;

			if (toCurrentWaypoint.LengthSquared() > 0.0001f)
			{
				toCurrentWaypoint.Normalize();
				direction = toCurrentWaypoint;
			}
			else
			{
				// waypoint에 거의 도달했으면 직접 이동으로 폴백
				direction = CalculateDirectionToPlayer();
			}
		}

		// ─────────────────────────────────────────────
		// 방향 정규화 및 이동
		// ─────────────────────────────────────────────
		direction.y = 0.0f;

		if (direction.LengthSquared() < 0.0001f)
		{
			return;
		}

		direction.Normalize();

		RotateToDirection(direction, 0.0f);  // deltaTime 불필요 (물리 엔진이 처리)
		
		// 속도 블렌딩 적용 (충돌 시 penetration 방지)
		engine::Vector3 currentVel = m_rigidbody->GetLinearVelocity();
		engine::Vector3 desiredVel = direction * m_moveSpeed;
		engine::Vector3 finalVel = engine::Vector3::Lerp(currentVel, desiredVel, m_velocityBlendFactor);
		m_rigidbody->SetLinearVelocity(finalVel);
	}

	// ═══════════════════════════════════════════════════════════════
	// 입력 처리 (FSM 파라미터 설정)
	// ═══════════════════════════════════════════════════════════════
	void MonsterScript::ProcessInput()
	{
		if (!m_logicFSM) return;

		// 플레이어를 찾지 못했으면 재탐색
		if (!m_targetPlayer)
		{
			FindPlayer();
		}

		// 플레이어와의 거리 체크
		m_isPlayerInRange = IsPlayerInRange();

		// FSM 파라미터 업데이트
		m_logicFSM->SetParameter("PlayerInRange", m_isPlayerInRange);
	}

	// ═══════════════════════════════════════════════════════════════
	// 게임 로직 업데이트 - 비물리 (Update에서 호출)
	// ═══════════════════════════════════════════════════════════════
	void MonsterScript::UpdateGameLogic()
	{
		float deltaTime = engine::Time::DeltaTime();
		std::string currentState = GetCurrentState();

		CheckHealth();
		UpdateStateBasedBehavior(currentState, deltaTime);
	}

	// ═══════════════════════════════════════════════════════════════
	// 물리 로직 업데이트 - FixedUpdate에서 호출
	// ═══════════════════════════════════════════════════════════════
	void MonsterScript::UpdatePhysicsLogic()
	{
		std::string currentState = GetCurrentState();
		UpdatePhysicsStateBasedBehavior(currentState);
	}

	// ═══════════════════════════════════════════════════════════════
	// 상태별 행동 - 비물리 (타이머, 공격 쿨다운 등)
	// ═══════════════════════════════════════════════════════════════
	void MonsterScript::UpdateStateBasedBehavior(const std::string& state, float deltaTime)
	{
		if (state == "Engage")
		{
			ExecuteEngageBehaviorNonPhysics(deltaTime);
		}
		else if (state == "Idle")
		{
			ExecuteIdleBehaviorNonPhysics();
		}
		else if (state == "Fragile")
		{
			ExecuteFragileBehaviorNonPhysics();
		}
		else if (state == "Dead")
		{
			ExecuteDeadBehaviorNonPhysics();
		}
	}

	void MonsterScript::ExecuteEngageBehaviorNonPhysics(float deltaTime)
	{
		// 공격 타이머 및 발사 처리 (비물리)
		Attack(deltaTime);
	}

	void MonsterScript::ExecuteIdleBehaviorNonPhysics()
	{
		// 비물리 Idle 처리
	}

	void MonsterScript::ExecuteFragileBehaviorNonPhysics()
	{
		// Fragile 상태에서는 아무 행동도 하지 않음
		// Execution을 기다리는 상태
	}

	void MonsterScript::ExecuteDeadBehaviorNonPhysics()
	{
		// 비물리 Dead 처리
	}

	// ═══════════════════════════════════════════════════════════════
	// 상태별 행동 - 물리 (이동, 회전)
	// ═══════════════════════════════════════════════════════════════
	void MonsterScript::UpdatePhysicsStateBasedBehavior(const std::string& state)
	{
		if (state == "Engage")
		{
			ExecuteEngageBehaviorPhysics();
		}
		else if (state == "Idle")
		{
			ExecuteIdleBehaviorPhysics();
		}
		else if (state == "Fragile")
		{
			ExecuteFragileBehaviorPhysics();
		}
		else if (state == "Dead")
		{
			ExecuteDeadBehaviorPhysics();
		}
	}

	void MonsterScript::ExecuteEngageBehaviorPhysics()
	{
		// 회전 (물리)
		RotateTowardsPlayer();
	}

	void MonsterScript::ExecuteIdleBehaviorPhysics()
	{
		StopRotation();
	}

	void MonsterScript::ExecuteFragileBehaviorPhysics()
	{
		// Fragile 상태: 물리적으로 정지
	}

	void MonsterScript::ExecuteDeadBehaviorPhysics()
	{
		StopAllMovement();
	}

	void MonsterScript::StopAllMovement()
	{
		if (m_rigidbody)
		{
			StopRotation();
			m_rigidbody->SetLinearVelocity(engine::Vector3::Zero);
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// 상태 진입 콜백
	// ═══════════════════════════════════════════════════════════════
	void MonsterScript::OnStateEntered(const std::string& state)
	{
		if (state == "Fragile")
		{
			StopAllMovement();
			OnFragile();
		}
		else if (state == "Dead")
		{
			StopAllMovement();
			OnDeath();
		}
		else if (state == "Idle")
		{
			StopRotation();
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// 행동 제한
	// ═══════════════════════════════════════════════════════════════
	bool MonsterScript::CanMove() const
	{
		std::string state = GetCurrentState();
		return state != "Fragile" && state != "Dead";
	}

	bool MonsterScript::CanAttack() const
	{
		std::string state = GetCurrentState();
		return state == "Engage" && state != "Fragile" && state != "Dead";
	}

	// ═══════════════════════════════════════════════════════════════
	// 체력 관리
	// ═══════════════════════════════════════════════════════════════
	void MonsterScript::CheckHealth()
	{
		std::string currentState = GetCurrentState();

		if (m_Hp <= 0 && currentState != "Fragile" && currentState != "Dead")
		{
			TriggerFragile();
		}
	}

	void MonsterScript::TriggerFragile()
	{
		if (m_logicFSM)
		{
			m_logicFSM->SetTrigger("Fragile");
			m_isFragile = true;
		}
	}

	void MonsterScript::TriggerDeath()
	{
		// Execution에서 호출됨
		if (m_logicFSM)
		{
			m_logicFSM->SetTrigger("Die");
			m_isDead = true;
		}
	}

	void MonsterScript::OnFragile()
	{
		// Fragile 상태 진입 시 처리
		// 추후 이펙트나 사운드 추가 가능
	}

	void MonsterScript::OnDeath()
	{
		// TODO: Dead 애니메이션 재생 후 일정 시간 후 파괴
		// float deathAnimDuration = 2.0f;
		// GetGameObject()->Destroy(deathAnimDuration);
	}

	void MonsterScript::TakeDamage(int damage)
	{
		// 이미 Fragile 또는 Dead 상태이면 무시
		std::string currentState = GetCurrentState();
		if (currentState == "Fragile" || currentState == "Dead")
		{
			return;
		}

		m_Hp -= damage;
		if (m_Hp < 0)
		{
			m_Hp = 0;
		}

		// 체력 체크는 CheckHealth()에서 자동으로 처리됨
	}

	// ═══════════════════════════════════════════════════════════════
	// 컴포넌트 검증
	// ═══════════════════════════════════════════════════════════════
	bool MonsterScript::ValidateComponents() const
	{
		bool isValid = true;

		if (!m_rigidbody) isValid = false;
		if (!m_skeletalAnimator) isValid = false;
		if (!m_bulletFactory) isValid = false;
		if (!m_animFSM) isValid = false;
		if (!m_logicFSM) isValid = false;

		return isValid;
	}

	// ═══════════════════════════════════════════════════════════════
	// GUI / 직렬화
	// ═══════════════════════════════════════════════════════════════
	void MonsterScript::OnGui()
	{
		ImGui::Indent();

		ImGui::Text("MonsterScript:");

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
		BulletFactory* bulletFactory = m_bulletFactory ? m_bulletFactory : (GetGameObject() ? GetGameObject()->GetComponent<BulletFactory>() : nullptr);
		engine::PathfindingAgent* pathfindingAgent = m_pathfindingAgent ? m_pathfindingAgent : (GetGameObject() ? GetGameObject()->GetComponent<engine::PathfindingAgent>() : nullptr);

		// 전체 유효성 검사 (PathfindingAgent는 이동 몬스터만 필요하므로 선택)
		bool allValid = rigidbody && skeletalAnimator && bulletFactory && animFSM && logicFSM;

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

		ImGui::Text("BulletFactory:     %s", bulletFactory ? "[OK]" : "[MISSING]");
		if (!bulletFactory) ImGui::SameLine(); if (!bulletFactory) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");

		ImGui::Text("AnimFSM:           %s", animFSM ? "[OK]" : "[MISSING]");
		if (!animFSM) ImGui::SameLine(); if (!animFSM) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");

		ImGui::Text("LogicFSM:          %s", logicFSM ? "[OK]" : "[MISSING]");
		if (!logicFSM) ImGui::SameLine(); if (!logicFSM) ImGui::TextColored(ImVec4(1, 0, 0, 1), "<-- Required!");

		ImGui::Text("PathfindingAgent:  %s", pathfindingAgent ? "[OK]" : "[MISSING]");
		if (m_moveSpeed > 0.0f && !pathfindingAgent) ImGui::SameLine(); if (m_moveSpeed > 0.0f && !pathfindingAgent) ImGui::TextColored(ImVec4(1, 1, 0, 1), "<-- Required for moving monsters!");
		ImGui::Unindent();

		// 스탯
		ImGui::Separator();
		ImGui::Text("Stats:");
		ImGui::DragInt("HP", &m_Hp, 1, 1, 10);
		ImGui::DragFloat("Attack Range", &m_AttackRange, 0.1f, 0.1f, 15.0f);

		// 설정
		ImGui::Separator();
		ImGui::Text("Settings:");
		ImGui::DragFloat("Move Speed", &m_moveSpeed, 0.1f, 0.0f, 100.0f);
		ImGui::DragFloat("Rotation Speed", &m_rotationSpeed, 0.1f, 0.0f, 10.0f);
		ImGui::DragFloat("Fire Rate (sec)", &m_fireRate, 0.1f, 0.1f, 10.0f);
		ImGui::DragFloat("Bullet Speed", &m_bulletSpeed, 0.1f, 0.1f, 100.0f);
		ImGui::DragFloat("Bullet Lifetime", &m_bulletLifetime, 0.1f, 0.5f, 10.0f);

		// 런타임 정보
		ImGui::Separator();
		ImGui::Text("Runtime Info:");
		ImGui::Text("Current State: %s", GetCurrentState().c_str());
		ImGui::Text("Player Found: %s", m_targetPlayer ? "Yes" : "No");
		if (m_targetPlayer)
		{
			ImGui::Text("Distance to Player: %.2f", GetDistanceToPlayer());
			ImGui::Text("Player In Range: %s", IsPlayerInRange() ? "Yes" : "No");
			ImGui::Text("Looking at Player: %s", IsLookingAtPlayer() ? "Yes" : "No");
		}
		ImGui::Text("Fire Timer: %.2f", m_fireTimer);

		ImGui::Unindent();

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
		ImGui::Separator();
		ImGui::PopStyleColor();
		ImGui::Spacing();
		ImGui::Spacing();
	}

	void MonsterScript::Save(engine::json& j) const
	{
		BaseControllerScript::Save(j);

		j["Hp"] = m_Hp;
		j["AttackRange"] = m_AttackRange;
		j["MoveSpeed"] = m_moveSpeed;
		j["RotationSpeed"] = m_rotationSpeed;
		j["FireRate"] = m_fireRate;
		j["BulletSpeed"] = m_bulletSpeed;
		j["BulletLifetime"] = m_bulletLifetime;
	}

	void MonsterScript::Load(const engine::json& j)
	{
		BaseControllerScript::Load(j);

		m_Hp = j.value("Hp", 100);
		m_AttackRange = j.value("AttackRange", 10.0f);
		m_moveSpeed = j.value("MoveSpeed", 0.0f);
		m_rotationSpeed = j.value("RotationSpeed", 2.0f);
		m_fireRate = j.value("FireRate", 3.0f);
		m_bulletSpeed = j.value("BulletSpeed", 1.0f);
		m_bulletLifetime = j.value("BulletLifetime", 3.0f);
	}
}
