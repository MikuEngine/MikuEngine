#include "GamePCH.h"
#include "Script/CharacterScript/Monster/BulletMonster.h"
#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/CharacterScript/Common/BulletMovement.h"
#include "Script/CharacterScript/Common/ExplosionDamageTrigger.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Collider.h>
#include <Framework/Object/Component/CapsuleCollider.h>
#include <Framework/Physics/PhysicsLayer.h>

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>

#include <Framework/Asset/Prefab.h>

#include <Framework/Object/Component/Renderer/DebugRenderer.h>

namespace game
{
	namespace
	{
		// BulletMonster 메쉬는 +Y 축이 정면이므로 X축 +90도 보정이 필요하다.
		// 메쉬의 앞(+Y)이 실제 진행 방향 벡터를 바라보도록 회전값을 만든다.
		engine::Quaternion BuildBulletRotationFromDirection(const engine::Vector3& position, const engine::Vector3& direction)
		{
			(void)position;

			engine::Vector3 normalizedDir = direction;
			normalizedDir.Normalize();

			// yaw는 XZ 평면 기준으로 계산.
			const float yawRad = std::atan2(normalizedDir.x, normalizedDir.z);
			const float yawDeg = engine::ToDegree(yawRad);

			// +Y 정면 메쉬 보정 + 고도각 반영:
			// - 기본 pitch 90도에서 시작해, 진행 방향의 고도각(elevation)을 반영한다.
			// - 이 값으로 포물선 탄환이 궤적 접선 방향을 바라보게 된다.
			const float elevationRad = std::asin(std::clamp(normalizedDir.y, -1.0f, 1.0f));
			const float elevationDeg = engine::ToDegree(elevationRad);
			const engine::Vector3 euler(90.0f - elevationDeg, yawDeg, 0.0f);
			return engine::Quaternion::CreateFromYawPitchRoll(
				engine::ToRadian(euler.y),
				engine::ToRadian(euler.x),
				engine::ToRadian(euler.z));
		}

		void UpdateParabolicBulletRotation(engine::GameObject* bulletOwner)
		{
			if (!bulletOwner)
				return;

			auto* transform = bulletOwner->GetTransform();
			auto* rb = bulletOwner->GetComponent<engine::Rigidbody>();
			if (!transform || !rb)
				return;

			const engine::Vector3 velocity = rb->GetLinearVelocity();
			if (velocity.LengthSquared() < 0.0001f)
			{
				// 속도가 거의 0인 프레임에서는 마지막 회전값을 유지한다.
				return;
			}

			engine::Vector3 direction = velocity;
			direction.Normalize();

			engine::Quaternion bulletRot = BuildBulletRotationFromDirection(transform->GetWorldPosition(), direction);
			transform->SetLocalRotation(bulletRot);
			rb->ForceSetRotation(bulletRot, true);
		}

		void UpdateCurveBulletRotationFromPathDelta(engine::GameObject* bulletOwner, const engine::Vector3& previousPos, const engine::Vector3& currentPos)
		{
			if (!bulletOwner)
				return;

			auto* transform = bulletOwner->GetTransform();
			auto* rb = bulletOwner->GetComponent<engine::Rigidbody>();
			if (!transform || !rb)
				return;

			engine::Vector3 delta = currentPos - previousPos;
			if (delta.LengthSquared() < 0.0001f)
			{
				// 이동량이 너무 작으면 기존 회전을 유지한다.
				return;
			}

			delta.Normalize();
			engine::Quaternion bulletRot = BuildBulletRotationFromDirection(currentPos, delta);
			transform->SetLocalRotation(bulletRot);
			rb->ForceSetRotation(bulletRot, true);
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// 초기화 (Factory에서 호출)
	// ═══════════════════════════════════════════════════════════════
	void BulletMonster::Setup(std::unique_ptr<IBulletMovement> movement, const BulletParams& params, BulletFactory* factory)
	{
		m_movement = std::move(movement);
		m_params = params;
		m_cachedFactory = factory;

		// 포물선 타입은 lifetime 10초 (충돌로 사라지지 않는 경우 대비)
		if (m_params.type == BulletType::Parabolic)
		{
			m_lifetime = 10.0f;
		}
		else
		{
			m_lifetime = params.lifetime;
		}
		
		// ─────────────────────────────────────────────
		// 스케일 즉시 적용 (Start() 호출 전에 Prefab Scale 덮어쓰기)
		// ─────────────────────────────────────────────
		GetTransform()->SetLocalScale(engine::Vector3(m_params.scale, m_params.scale, m_params.scale));
	}

	//void BulletMonster::SetupField(float radius, const BulletParams& params)
	//{
	//	m_isFieldType = true;
	//	m_radius = radius;
	//	m_lifetime = params.lifetime;
	//	m_params = params;

	//	auto* collider = GetGameObject()->GetComponent<engine::Collider>();
	//	if (collider)
	//	{
	//		collider->SetLayer(engine::PhysicsLayer::Field);
	//		collider->SetIsTrigger(true);
	//	}

	//	auto* scene = engine::SceneManager::Get().GetScene();
	//	if (scene)
	//	{
	//		m_targetPlayer = scene->FindGameObject("Player");
	//	}
	//}

	// ═══════════════════════════════════════════════════════════════
	// 생명주기
	// ═══════════════════════════════════════════════════════════════
	void BulletMonster::Start()
	{
		m_elapsedTime = 0.0f;
		m_lastWorldPos = GetTransform()->GetWorldPosition();
		m_hasLastWorldPos = true;
		if (auto* collider = GetGameObject()->GetComponent<engine::Collider>())
		{
			m_initialColliderCenter = collider->GetCenter();
			m_hasInitialColliderCenter = true;
		}

		// ─────────────────────────────────────────────
		// 총알 스케일 적용 (균등 스케일)
		// - BulletMonster Prefab들도 syncWithTransform=true이므로 Collider 자동 스케일
		// ─────────────────────────────────────────────
		GetTransform()->SetLocalScale(engine::Vector3(m_params.scale, m_params.scale, m_params.scale));

		auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>();
		if (!rb) return;

		// Rigidbody 깨우기 (Sleep 상태 해제)
		rb->WakeUp();

		// Parabolic/Field 타입: 자체 중력 사용 (PhysX 글로벌 중력 OFF)
		if (m_params.type == BulletType::Parabolic || m_params.type == BulletType::Field)
		{
			rb->SetUseGravity(false);
			rb->SetLinearDamping(0.0f);
			rb->SetAngularDamping(0.0f);
		}

		// 방향 및 회전 계산
		engine::Vector3 velocity = m_movement->GetVelocity();
		if (velocity.LengthSquared() > 0.0001f)
		{
			engine::Vector3 direction = velocity;
			direction.Normalize();

			engine::Quaternion bulletRot = BuildBulletRotationFromDirection(GetTransform()->GetWorldPosition(), direction);

			GetTransform()->SetLocalRotation(bulletRot);

			rb->ForceSetRotation(bulletRot, true);
			ApplyColliderPivotAlignedRotation(bulletRot);
		}

		// Rigidbody에 초기 속도 설정
		if (!m_movement) return;
		rb->SetLinearVelocity(m_movement->GetVelocity());
		rb->WakeUp();
	}

	void BulletMonster::Update()
	{
		// 죽는 중이면 타이머만 체크
		if (m_isDying)
		{
			m_deathTimer += engine::Time::DeltaTime();
			if (m_deathTimer >= m_deathDelay)
			{
				// 폭발 트리거 생성 (착탄 시에만)
				if (m_shouldSpawnExplosion)
				{
					SpawnExplosionTrigger(m_impactPoint);
				}
				GetGameObject()->Destroy();
			}
			return;
		}

		// 생존 시간 누적
		float dt = engine::Time::DeltaTime();
		m_elapsedTime += dt;

		// Movement 업데이트
		if (m_movement && !m_isFieldType)
		{
			m_movement->Update(GetTransform(), dt);
		}

		// 포물선 탄환은 비행 중 계속해서 현재 진행 방향을 바라보게 한다.
		if (m_params.type == BulletType::Parabolic)
		{
			UpdateParabolicBulletRotation(GetGameObject());
			ApplyColliderPivotAlignedRotation(GetTransform()->GetLocalRotation());
		}
		else if (m_params.type == BulletType::Curve)
		{
			const engine::Vector3 currentPos = GetTransform()->GetWorldPosition();
			if (m_hasLastWorldPos)
			{
				UpdateCurveBulletRotationFromPathDelta(GetGameObject(), m_lastWorldPos, currentPos);
				ApplyColliderPivotAlignedRotation(GetTransform()->GetLocalRotation());
			}
			m_lastWorldPos = currentPos;
			m_hasLastWorldPos = true;
		}

		// 수명 체크 (lifetime 만료 시 폭발 트리거 생성 안 함)
		if (m_elapsedTime >= m_lifetime)
		{
			GetGameObject()->Destroy();
			return;
		}

		engine::Vector3 pos = GetTransform()->GetWorldPosition();

		// ═══════════════════════════════════════════════════════════════
		//  포물선 타입 전용: Y좌표 기반 착탄
		// Y <= -0.1 도달 시 착탄점에서 폭발 트리거 생성		
		// ═══════════════════════════════════════════════════════════════
		
		if (m_params.type == BulletType::Parabolic && !m_isDying)
		{
			constexpr float kGroundY = -0.1f;
			if (pos.y <= kGroundY)
			{
				DieWithExplosion(pos);
				return;
			}
		}
		

		// ─────────────────────────────────────────────
		// Field 타입: BulletLinearMonster + ParabolicMovement 착탄 처리
		// - 현재 Field 타입은 별도 방식으로 처리됨	
		// ─────────────────────────────────────────────
		
		//if (!m_isDying && !m_isFieldType && m_params.type == BulletType::Field)
		//{
		//	if (pos.y <= 0.1f)
		//	{
		//		if (m_cachedFactory)
		//		{
		//			engine::Vector3 spawnPos = pos;
		//			spawnPos.y = 0.0f;
		//			m_cachedFactory->FieldFireMonster(spawnPos, m_params);
		//		}

		//		m_isDying = true;
		//		m_deathTimer = 0.0f;
		//		m_shouldSpawnExplosion = false;

		//		if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
		//		{
		//			rb->SetLinearVelocity(engine::Vector3::Zero);
		//		}
		//		return;
		//	}
		//}
		

		// 장판형 총알일 경우 주기적으로 데미지 적용
//		if (m_isFieldType)
//		{
//#ifdef _DEBUG
//			engine::DebugRenderer::Get().AddDebugCircle(
//				pos + engine::Vector3(0, 0.05f, 0),
//				m_radius,
//				engine::Vector3::UnitY,
//				DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.2f),
//				32
//			);
//#endif
//			m_tickTimer += engine::Time::DeltaTime();
//			if (m_tickTimer >= m_tickInterval)
//			{
//				m_tickTimer = 0.0f;
//				if (m_targetPlayer)
//				{
//					float distance = engine::Vector3::Distance(pos, m_targetPlayer->GetTransform()->GetWorldPosition());
//					if (distance <= m_params.radius)
//					{
//						if (auto* playerScript = m_targetPlayer->GetComponent<PlayerControllerScript>())
//						{
//							playerScript->TakeDamage(m_params.damage);
//						}
//					}
//				}
//			}
//			return;
//		}
//
//		// 화면 밖 체크 (XZ 범위) - 폭발 트리거 생성 안 함
//		float boundary = 50.0f;
//		if (std::abs(pos.x) > boundary || std::abs(pos.z) > boundary)
//		{
//			GetGameObject()->Destroy();
//			return;
//		}
	}

	void BulletMonster::FixedUpdate()
	{
		// ─────────────────────────────────────────────
		// Movement의 FixedUpdate 호출 (물리 연산)
		// PhysX simulate()와 동기화됨
		// ─────────────────────────────────────────────
		if (m_movement)
		{
			m_movement->FixedUpdate();
		}
	}

	void BulletMonster::ApplyColliderPivotAlignedRotation(const engine::Quaternion& bulletRot)
	{
		auto* collider = GetGameObject()->GetComponent<engine::Collider>();
		if (!collider || !collider->IgnoresWorldRotation())
		{
			return;
		}

		if (!m_hasInitialColliderCenter)
		{
			m_initialColliderCenter = collider->GetCenter();
			m_hasInitialColliderCenter = true;
		}

		const engine::Vector3 eulerRad = bulletRot.ToEuler();
		const engine::Vector3 eulerDeg(
			engine::ToDegree(eulerRad.x),
			engine::ToDegree(eulerRad.y),
			engine::ToDegree(eulerRad.z));
		collider->SetRotation(eulerDeg);

		auto* capsule = GetGameObject()->GetComponent<engine::CapsuleCollider>();
		if (!capsule)
		{
			// 캡슐이 아니면 기존 center를 유지한다.
			collider->SetCenter(m_initialColliderCenter);
			return;
		}

		// 피봇을 캡슐의 바닥 끝점으로 고정한다.
		const float halfHeight = std::max(0.001f, capsule->GetHeight() * 0.5f);
		const engine::Vector3 localBottomPivot = m_initialColliderCenter + engine::Vector3(0.0f, -halfHeight, 0.0f);
		const engine::Vector3 centerFromPivot(0.0f, halfHeight, 0.0f);

		const auto rotMtx = DirectX::SimpleMath::Matrix::CreateFromQuaternion(
			DirectX::SimpleMath::Quaternion(bulletRot.x, bulletRot.y, bulletRot.z, bulletRot.w));
		const engine::Vector3 rotatedCenterFromPivot = engine::Vector3::TransformNormal(centerFromPivot, rotMtx);
		collider->SetCenter(localBottomPivot + rotatedCenterFromPivot);
	}

	// ═══════════════════════════════════════════════════════════════
	// 충돌 콜백
	// ═══════════════════════════════════════════════════════════════
	void BulletMonster::OnTriggerEnter(const engine::CollisionInfo& info)
	{
		if (m_isDying || m_isFieldType) return;
		if (!info.gameObject) return;

		// ─────────────────────────────────────────────
		// 포물선 타입: 플레이어 충돌 시 착탄점에서 폭발		
		// ─────────────────────────────────────────────
		
		if (m_params.type == BulletType::Parabolic)
		{
			if (info.gameObject->GetComponent<PlayerControllerScript>())
			{
				engine::Vector3 impactPoint = GetTransform()->GetWorldPosition();
				if (!info.contacts.empty())
				{
					impactPoint = info.contacts[0].point;
				}
				
				// Player 충돌 시 y좌표를 0으로 설정 (바닥 높이에서 폭발)
				impactPoint.y = 0.0f;
				
				DieWithExplosion(impactPoint);
			}
			return;
		}		

		auto collider = info.gameObject->GetComponent<engine::Collider>();
		if (!collider) return;
		auto layer = collider->GetLayer();

		bool isPlayer = (layer == engine::PhysicsLayer::Player);
		bool isEnvironment = (layer == engine::PhysicsLayer::Environment);
		bool isWall = (layer == engine::PhysicsLayer::Wall);

		// 일반 타입: 플레이어, 환경, 벽과 충돌 시
		if (isPlayer || isEnvironment || isWall)
		{
			if (isPlayer)
			{
				auto* playerScript = info.gameObject->GetComponent<PlayerControllerScript>();
				if (playerScript)
				{
					playerScript->TakeDamage(m_params.damage);
				}
			}

			m_isDying = true;
			m_deathTimer = 0.0f;
			m_shouldSpawnExplosion = false;

			if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
			{
				rb->SetLinearVelocity(engine::Vector3::Zero);
			}

			auto effect = engine::Prefab::Instantiate("Effect_Bullet_Destory_V1.00");
			if (effect && effect->GetTransform())
			{
				effect->GetTransform()->SetWorldMatrix(GetTransform()->GetWorld());
				effect->GetTransform()->SetLocalScale(engine::Vector3(1.0f, 1.0f, 1.0f));
			}
		}
	}

	void BulletMonster::OnCollisionEnter(const engine::CollisionInfo& info)
	{
		if (m_isDying || m_isFieldType) return;
		if (!info.gameObject) return;
		
		if (m_params.type == BulletType::Parabolic)
		{
			auto* collider = info.gameObject->GetComponent<engine::Collider>();
			if (!collider) return;

			uint32_t layer = collider->GetLayer();

			if (layer == engine::PhysicsLayer::Wall || layer == engine::PhysicsLayer::Player)
			{
				engine::Vector3 impactPoint = GetTransform()->GetWorldPosition();
				if (!info.contacts.empty())
				{
					impactPoint = info.contacts[0].point;
				}
				
				// Player 충돌 시 y좌표를 0으로 설정 (바닥 높이)
				if (layer == engine::PhysicsLayer::Player)
				{
					impactPoint.y = 0.0f;
				}
				
				DieWithExplosion(impactPoint);
			}
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// 포물선 탄환 전용 헬퍼
	// ═══════════════════════════════════════════════════════════════
	void BulletMonster::SpawnExplosionTrigger(const engine::Vector3& position)
	{
		auto* scene = engine::SceneManager::Get().GetScene();
		if (!scene) return;

		// ExplosionDamageTrigger 프리팹 로드 및 인스턴시에이트
		auto explosionGO = engine::Prefab::Instantiate("ExplosionDamageTrigger");

		if (!explosionGO) return;

		// 위치 설정
		explosionGO->GetTransform()->SetLocalPosition(position);

		// ExplosionDamageTrigger 스크립트 설정
		if (auto* explosionScript = explosionGO->GetComponent<ExplosionDamageTrigger>())
		{
			// damage, explosionRadius, lifetime 전달
			explosionScript->Setup(m_params.damage, m_params.explosionRadius, 1.5f);
		}
	}

	void BulletMonster::DieWithExplosion(const engine::Vector3& impactPoint)
	{
		m_isDying = true;
		m_deathTimer = 0.0f;
		m_shouldSpawnExplosion = true;
		m_impactPoint = impactPoint;

		// 속도 정지
		if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
		{
			rb->SetLinearVelocity(engine::Vector3::Zero);
		}
	}
}
