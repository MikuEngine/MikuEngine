#include "GamePCH.h"
#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/CharacterScript/Player/BulletPlayer.h"
#include "Script/CharacterScript/Monster/BulletMonster.h"
#include "Script/CharacterScript/Monster/RoundType/MonsterRoundType.h"
#include "Script/CharacterScript/Monster/RoundType/MonsterRoundGreen.h"

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
		// ─────────────────────────────────────────────
		auto* bullet = go->GetComponent<BulletPlayer>();
		bullet->Setup(std::move(movement), params.lifetime, params.damage, params.range);

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
			// launchAngle, ownGravity 사용
			return std::make_unique<ParabolicMovement>(params.ownGravity, params.launchAngle);

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
		// 같은 GameObject의 MonsterRoundType에서 BulletParams 정보 표시
		// ─────────────────────────────────────────────
		auto* roundMonster = GetGameObject()->GetComponent<MonsterRoundType>();
		if (roundMonster)
		{
			const BulletParams& params = roundMonster->GetBulletParams();
			
			// Green 몬스터인지 확인 (에디터 모드에서도 올바른 타입 표시)
			auto* greenMonster = GetGameObject()->GetComponent<MonsterRoundGreen>();
			bool isGreenMonster = (greenMonster != nullptr);
			
			// 실제 사용될 타입 결정 (Green = 항상 Parabolic)
			BulletType displayType = isGreenMonster ? BulletType::Parabolic : params.type;
			
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
				ImGui::Text("Launch Angle: %.1f deg (auto)", params.launchAngle);
				ImGui::Text("Own Gravity: %.2f m/s^2", params.ownGravity);
				{
					float maxRange = (params.speed * params.speed) / params.ownGravity;
					ImGui::Text("Max Range: %.1f m", maxRange);
				}
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
