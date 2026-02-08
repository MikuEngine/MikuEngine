#include "GamePCH.h"
#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/CharacterScript/Player/BulletPlayer.h"
#include "Script/CharacterScript/Monster/BulletMonster.h"
#include "Script/CharacterScript/Monster/RoundType/MonsterRoundType.h"
#include "Script/CharacterScript/Monster/RoundType/MonsterRoundGreen.h"
#include "Script/Boss/BossPattern/Components/BossBulletThreeway.h"
#include "Script/Boss/BossPattern/Components/BossBulletEightway.h"
#include "Script/ParticleAttachment.h"

#include <Framework/Asset/Prefab.h>
#include <Framework/System/SoundSystem.h>

namespace game
{
	// ═══════════════════════════════════════════════════════════════
	// 플레이어 총알 발사
	// ═══════════════════════════════════════════════════════════════
	void BulletFactory::Fire(const engine::Vector3& position,
		const engine::Vector3& direction,
		const BulletParams& params)
	{
		auto go = engine::Prefab::Instantiate("BulletPlayer");

		// 발사 위치는 호출자(PCS)에서 이미 오프셋 적용됨
		go->GetTransform()->SetLocalPosition(position);


		// ─────────────────────────────────────────────
		// 5. Movement 생성 및 초기화
		// ─────────────────────────────────────────────
		auto movement = CreateMovement(params);
		movement->Initialize(go, direction, params.speed);

		// ─────────────────────────────────────────────
		// 6. BulletPlayer 컴포넌트 추가 및 설정
		// - scale 전달 (Start()에서 Transform + Collider 스케일 적용)
		// ─────────────────────────────────────────────
		auto* bullet = go->GetComponent<BulletPlayer>();
		bullet->Setup(std::move(movement), params.lifetime, params.damage, params.range, params.scale);

		// effect
		auto effect = engine::Prefab::Instantiate("Effect_Bullet_Trail");
		if (effect)
		{
			effect->GetComponent<ParticleAttachment>()->SetTarget(go);
		}

		engine::SoundSystem::Get().Play("Player_Shot_Random", "SFX/Player");
	}

	// ═══════════════════════════════════════════════════════════════
	// 몬스터 직선 총알 발사
	// ═══════════════════════════════════════════════════════════════
	void BulletFactory::LinearFireMonster(const engine::Vector3& position,
		const engine::Vector3& direction,
		const BulletParams& params)
	{		
		// 총알 발사 시, 프리팹 이름으로 찾아서 인스턴시에이트
		auto go = engine::Prefab::Instantiate("BulletLinearMonster");

		go->GetTransform()->SetLocalPosition(position);

		// ─────────────────────────────────────────────
		// 5. Movement 생성 및 초기화
		// ─────────────────────────────────────────────
		auto movement = CreateMovement(params);
		movement->Initialize(go, direction, params.speed);

		// ─────────────────────────────────────────────
		// 6. BulletMonster 컴포넌트 추가 및 설정
		// ─────────────────────────────────────────────
		auto* bullet = go->GetComponent<BulletMonster>();

		bullet->Setup(std::move(movement), params, this);

		// effect
		auto effect = engine::Prefab::Instantiate("Effect_Bullet_Trail");
		if (effect)
		{
			effect->GetComponent<ParticleAttachment>()->SetTarget(go);
		}
	}

	// 포물선 탄환
	void BulletFactory::ParabolicFireMonster(const engine::Vector3& position, const engine::Vector3& direction, const BulletParams& params)
	{
		auto go = engine::Prefab::Instantiate("BulletParabolicMonster");
		
		go->GetTransform()->SetLocalPosition(position);

		auto movement = CreateMovement(params);
		movement->Initialize(go, direction, params.speed);

		auto* bullet = go->GetComponent<BulletMonster>();
		
		bullet->Setup(std::move(movement), params, this);

		// effect
		auto effect = engine::Prefab::Instantiate("Effect_Bullet_Trail");
		if (effect)
		{
			effect->GetComponent<ParticleAttachment>()->SetTarget(go);
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// 나선형 총알 발사 (4발, +X/-X/+Z/-Z 방향)
	// angularSpeed, radiusGrowthRate: 실시간 반영을 위해 직접 전달
	// ═══════════════════════════════════════════════════════════════
	void BulletFactory::CurvedFireMonster(const engine::Vector3& position,
	                                      float angularSpeed,
	                                      float radiusGrowthRate,
	                                      const BulletParams& params)
	{
		// 실시간 반영을 위해 params 복사 후 값 설정
		BulletParams curvedParams = params;
		curvedParams.angularSpeed = angularSpeed;
		curvedParams.radiusGrowthRate = radiusGrowthRate;

		// 4방향 발사 (+X, -X, +Z, -Z)
		const engine::Vector3 directions[4] = {
			engine::Vector3( 1.0f, 0.0f,  0.0f),  // +X
			engine::Vector3(-1.0f, 0.0f,  0.0f),  // -X
			engine::Vector3( 0.0f, 0.0f,  1.0f),  // +Z
			engine::Vector3( 0.0f, 0.0f, -1.0f),  // -Z
		};

		for (int i = 0; i < 4; ++i)
		{
			auto go = engine::Prefab::Instantiate("BulletCurvedMonster");
			if (!go)
			{
				LOG_PRINT("[BulletFactory] ERROR: Failed to instantiate 'BulletCurvedMonster' prefab!");
				continue;
			}

			go->GetTransform()->SetLocalPosition(position);

			// Movement 생성 및 초기화 (curvedParams 사용)
			auto movement = CreateMovement(curvedParams);
			movement->Initialize(go, directions[i], curvedParams.speed);

			// BulletMonster 컴포넌트 설정
			auto* bullet = go->GetComponent<BulletMonster>();
			if (bullet)
			{
				bullet->Setup(std::move(movement), curvedParams, this);
			}

			// effect
			auto effect = engine::Prefab::Instantiate("Effect_Bullet_Trail");
			if (effect)
			{
				effect->GetComponent<ParticleAttachment>()->SetTarget(go);
			}
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// 3방향 총알 발사 (중앙 + 좌우 퍼짐)
	// ═══════════════════════════════════════════════════════════════
	void BulletFactory::ThreewayFireMonster(const engine::Vector3& position,
	                                        const engine::Vector3& direction,
	                                        float spreadAngle,
	                                        const BulletParams& params)
	{
		// 3방향: 좌(-spreadAngle), 중앙(0), 우(+spreadAngle)
		const float angles[3] = { -spreadAngle, 0.0f, spreadAngle };

		for (int i = 0; i < 3; ++i)
		{
			auto go = engine::Prefab::Instantiate("BulletLinearMonster");
			if (!go)
			{
				LOG_PRINT("[BulletFactory] ERROR: Failed to instantiate 'BulletLinearMonster' prefab!");
				continue;
			}

			go->GetTransform()->SetLocalPosition(position);

			// 방향 회전 (Y축 기준)
			DirectX::SimpleMath::Matrix rot = DirectX::SimpleMath::Matrix::CreateRotationY(angles[i]);
			engine::Vector3 fireDir = engine::Vector3::TransformNormal(direction, rot);
			fireDir.Normalize();

			// Movement 생성 및 초기화
			auto movement = CreateMovement(params);
			movement->Initialize(go, fireDir, params.speed);

			// BulletMonster 컴포넌트 설정
			auto* bullet = go->GetComponent<BulletMonster>();
			if (bullet)
			{
				bullet->Setup(std::move(movement), params, this);
			}

			// effect
			auto effect = engine::Prefab::Instantiate("Effect_Bullet_Trail");
			if (effect)
			{
				effect->GetComponent<ParticleAttachment>()->SetTarget(go);
			}
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// 보스 3방향 총알 발사 (중앙 + 좌우 퍼짐)
	// ═══════════════════════════════════════════════════════════════
	void BulletFactory::ThreewayFireBoss(const engine::Vector3& position,
	                                     const engine::Vector3& direction,
	                                     float spreadAngle,
	                                     const BulletParams& params)
	{
		// 생성 위치: 보스 xz 좌표 + y=1.5 (오프셋)
		engine::Vector3 spawnPosition = position;
		spawnPosition.y = 1.5f;

		// direction의 Y 성분 제거 (XZ 평면 방향만 사용)
		engine::Vector3 directionXZ = direction;
		directionXZ.y = 0.0f;
		directionXZ.Normalize();

		// 3방향: 좌(-spreadAngle), 중앙(0), 우(+spreadAngle)
		const float angles[3] = { -spreadAngle, 0.0f, spreadAngle };

		for (int i = 0; i < 3; ++i)
		{
			auto go = engine::Prefab::Instantiate("BossBulletProjectile");
			if (!go) continue;

			go->GetTransform()->SetLocalPosition(spawnPosition);

			// 방향 회전 (Y축 기준)
			DirectX::SimpleMath::Matrix rot = DirectX::SimpleMath::Matrix::CreateRotationY(angles[i]);
			engine::Vector3 fireDir = engine::Vector3::TransformNormal(directionXZ, rot);
			fireDir.Normalize();

			// Movement 생성 및 초기화 (Linear만 사용)
			auto movement = CreateMovement(params);
			movement->Initialize(go, fireDir, params.speed);

			// BossBulletThreeway 컴포넌트 설정
			auto* bullet = go->GetComponent<BossBulletThreeway>();
			if (bullet)
			{
				bullet->Setup(std::move(movement), params, this);
			}

			// effect
			auto effect = engine::Prefab::Instantiate("Effect_Bullet_Trail");
			if (effect)
			{
				effect->GetComponent<ParticleAttachment>()->SetTarget(go);
			}
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// 보스 메테오 8방향 총알 발사
	// ═══════════════════════════════════════════════════════════════
	void BulletFactory::EightwayFireBossMeteor(const engine::Vector3& position,
	                                           const BulletParams& params)
	{
		// 생성 위치: 메테오 XZ 좌표 + y=1.5
		engine::Vector3 spawnPosition = position;
		spawnPosition.y = 1.5f;

		// 8방향: 0도부터 45도씩 증가
		const int bulletCount = 8;
		const float angleStep = DirectX::XM_2PI / bulletCount;

		for (int i = 0; i < bulletCount; ++i)
		{
			auto go = engine::Prefab::Instantiate("BossMeteorBulletProjectile");
			if (!go) continue;

			go->GetTransform()->SetLocalPosition(spawnPosition);

			// 방향 계산 (Y축 기준 회전)
			float angle = angleStep * i;
			engine::Vector3 fireDir(std::sin(angle), 0.0f, std::cos(angle));
			fireDir.Normalize();

			// Movement 생성 및 초기화 (Linear만 사용)
			auto movement = CreateMovement(params);
			movement->Initialize(go, fireDir, params.speed);

			// BossBulletEightway 컴포넌트 설정
			auto* bullet = go->GetComponent<BossBulletEightway>();
			if (bullet)
			{
				bullet->Setup(std::move(movement), params, this);
			}

			// effect
			auto effect = engine::Prefab::Instantiate("Effect_Bullet_Trail");
			if (effect)
			{
				effect->GetComponent<ParticleAttachment>()->SetTarget(go);
			}
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// 연사 총알 발사 (랜덤 각도 + 속도/수명 변조)
	// ═══════════════════════════════════════════════════════════════
	void BulletFactory::BurstFireMonster(const engine::Vector3& position,
	                                     const engine::Vector3& direction,
	                                     int count,
	                                     float spreadAngle,
	                                     float lifetimeModMin,
	                                     float lifetimeModMax,
	                                     float speedModMin,
	                                     float speedModMax,
	                                     const BulletParams& params)
	{
		for (int i = 0; i < count; ++i)
		{
			auto go = engine::Prefab::Instantiate("BulletLinearMonster");
			if (!go)
			{
				LOG_PRINT("[BulletFactory] ERROR: Failed to instantiate 'BulletLinearMonster' prefab!");
				continue;
			}

			go->GetTransform()->SetLocalPosition(position);

			// 방향 랜덤 오프셋 (±spreadAngle)
			float randomOffset = ((static_cast<float>(rand()) / RAND_MAX) * spreadAngle * 2.0f) - spreadAngle;
			DirectX::SimpleMath::Matrix rot = DirectX::SimpleMath::Matrix::CreateRotationY(randomOffset);
			engine::Vector3 fireDir = engine::Vector3::TransformNormal(direction, rot);
			fireDir.Normalize();

			// 개별 파라미터 (lifetime, speed 랜덤 변조)
			BulletParams individualParams = params;

			float lifetimeMod = lifetimeModMin + (static_cast<float>(rand()) / RAND_MAX) * (lifetimeModMax - lifetimeModMin);
			individualParams.lifetime *= lifetimeMod;

			float speedMod = speedModMin + (static_cast<float>(rand()) / RAND_MAX) * (speedModMax - speedModMin);
			individualParams.speed *= speedMod;

			// Movement 생성 및 초기화
			auto movement = CreateMovement(individualParams);
			movement->Initialize(go, fireDir, individualParams.speed);

			// BulletMonster 컴포넌트 설정
			auto* bullet = go->GetComponent<BulletMonster>();
			if (bullet)
			{
				bullet->Setup(std::move(movement), individualParams, this);
			}

			// effect
			auto effect = engine::Prefab::Instantiate("Effect_Bullet_Trail");
			if (effect)
			{
				effect->GetComponent<ParticleAttachment>()->SetTarget(go);
			}
		}
	}

	// ExplosinTriggerScript로 기능 이전
	//void BulletFactory::FieldFireMonster(const engine::Vector3& position, const BulletParams& params)
	//{
	//	auto go = engine::Prefab::Instantiate("BulletLinearMonster");
	//	
	//	go->GetTransform()->SetLocalPosition(position);

	//	auto* bullet = go->GetComponent<BulletMonster>();
	//
	//	bullet->SetupField(params.radius, params);
	//}

	// ═══════════════════════════════════════════════════════════════
	// Movement 생성 (Strategy 패턴)
	// 
	// 참고:
	//   - launchAngle, ownGravity는 Parabolic 타입에서만 사용
	//   - 다른 타입에서는 이 파라미터들을 명시적으로 무시
	// ═══════════════════════════════════════════════════════════════
	std::unique_ptr<IBulletMovement> BulletFactory::CreateMovement(const BulletParams& params)
	{
		switch (params.type)
		{
		case BulletType::BulletPlayer:
			// launchAngle, ownGravity 무시 (직선 이동)
			return std::make_unique<BulletPlayerMovement>();

		case BulletType::Linear:
			// launchAngle, ownGravity 무시 (직선 이동)
			return std::make_unique<LinearMovement>();

		case BulletType::Parabolic:
			// launchAngle, ownGravity, maxLaunchAngle 사용
			return std::make_unique<ParabolicMovement>(params.ownGravity, params.launchAngle, params.maxLaunchAngle);

		case BulletType::Curve:
			// 나선형 이동 (angularSpeed, radiusGrowthRate 사용)
			return std::make_unique<CurvedMovement>(params.angularSpeed, params.radiusGrowthRate);

		//case BulletType::Field:
		//	// Field 타입도 Parabolic 궤적 사용 (착탄 후 장판 생성)
		//	return std::make_unique<ParabolicMovement>(params.ownGravity, params.launchAngle);

		default:
			return std::make_unique<LinearMovement>();

		// 추후 구현:
		// case BulletType::Homing:
		//     return std::make_unique<HomingMovement>(params.target, params.turnSpeed);
		}
	}

	// ═══════════════════════════════════════════════════════════════
	// GUI / 직렬화
	// ═══════════════════════════════════════════════════════════════
	void BulletFactory::OnGui()
	{
		ImGui::Text("=== BulletFactory ===");
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
			"Factory for creating bullets. Settings managed by MonsterScript.");
		
		// ─────────────────────────────────────────────
		// 같은 GameObject의 MonsterScript에서 BulletParams 정보 표시
		// ─────────────────────────────────────────────
		auto* monsterScript = GetGameObject()->GetComponent<MonsterScript>();
		if (monsterScript)
		{
			const BulletParams& params = monsterScript->GetBulletParams();
			
			// Parabolic 몬스터인지 확인 (에디터 모드에서도 올바른 타입 표시)
			bool isParabolicMonster = monsterScript->IsParabolicBullet();
			
			// 실제 사용될 타입 결정 (Parabolic 몬스터 = 항상 Parabolic)
			BulletType displayType = isParabolicMonster ? BulletType::Parabolic : params.type;
			
			ImGui::Separator();
			ImGui::Text("=== Current Bullet Settings ===");
			
			// 타입 이름 표시
			const char* typeNames[] = { "BulletPlayer", "Linear", "Parabolic", "Curve", "Field" };
			int typeIndex = static_cast<int>(displayType);
			if (typeIndex >= 0 && typeIndex < 5)
			{
				ImGui::Text("Type: %s", typeNames[typeIndex]);
			}
			else
			{
				ImGui::Text("Type: Unknown (%d)", typeIndex);
			}
			
			// 공통 속성
			ImGui::Text("Speed: %.2f m/s", params.speed);
			ImGui::Text("Lifetime: %.2f sec", params.lifetime);
			ImGui::Text("Damage: %d", params.damage);
			
			// 타입별 전용 속성
			ImGui::Separator();
			switch (displayType)
			{
			case BulletType::Parabolic:
			case BulletType::Field:
				ImGui::Text("=== Parabolic Settings ===");
				ImGui::Text("Launch Angle: %.1f deg (auto, range: %.1f~%.1f)", 
					params.launchAngle, params.minLaunchAngle, params.maxLaunchAngle);
				ImGui::Text("Speed: %.1f m/s (adjusted)", params.speed);
				ImGui::Text("Own Gravity: %.2f m/s^2", params.ownGravity);
				ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
					"(Speed varies by distance in 45~%.1f deg range)", params.maxLaunchAngle);
				break;
				
			case BulletType::Curve:
				ImGui::Text("=== Curve (Spiral) Settings ===");
				ImGui::Text("Angular Speed: %.2f rad/s", params.angularSpeed);
				ImGui::Text("Radius Growth: %.2f m/s", params.radiusGrowthRate);
				break;
				
			case BulletType::Linear:
			case BulletType::BulletPlayer:
			default:
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
					"(No additional settings for this type)");
				break;
			}
			
			ImGui::Separator();
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), 
				"Edit these values in MonsterScript inspector");
		}
		else
		{
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
				"No MonsterRoundType found on this GameObject.");
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), 
				"BulletParams will be provided at Fire() call.");
		}
	}

	void BulletFactory::Save(engine::json& j) const
	{
		Object::Save(j);
	}

	void BulletFactory::Load(const engine::json& j)
	{
		Object::Load(j);
	}
}
