#include "GamePCH.h"
#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/CharacterScript/Player/BulletPlayer.h"
#include "Script/CharacterScript/Monster/BulletMonster.h"
#include "Script/CharacterScript/Monster/RoundType/MonsterRoundType.h"
#include "Script/CharacterScript/Monster/RoundType/MonsterRoundGreen.h"

#include <Framework/Asset/Prefab.h>

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
		bullet->Setup(std::move(movement), params.lifetime, static_cast<float>(params.damage));
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

	void BulletFactory::ParabolicFireMonster(const engine::Vector3& position, const engine::Vector3& direction, const BulletParams& params)
	{
		auto go = engine::Prefab::Instantiate("BulletParabolicMonster");
		
		go->GetTransform()->SetLocalPosition(position);

		auto movement = CreateMovement(params);
		movement->Initialize(go, direction, params.speed);

		auto* bullet = go->GetComponent<BulletMonster>();
		
		bullet->Setup(std::move(movement), params, this);
	}

	void BulletFactory::FieldFireMonster(const engine::Vector3& position, const BulletParams& params)
	{
		auto go = engine::Prefab::Instantiate("BulletLinearMonster");
		if (!go)
		{
			LOG_PRINT("[BulletFactory] ERROR: Failed to instantiate 'FieldFireMonster' prefab!");
			return;
		}

		go->GetTransform()->SetLocalPosition(position);

		auto* bullet = go->GetComponent<BulletMonster>();
		if (!bullet)
		{
			LOG_PRINT("[BulletFactory] ERROR: 'FieldFireMonster' prefab missing BulletMonster component!");
			return;
		}
		bullet->SetupField(params.radius, params);
	}

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
			// launchAngle, ownGravity 무시 (곡선 이동)
			return std::make_unique<CurvedMovement>(params.curveSpeed);

		case BulletType::Field:
			// Field 타입도 Parabolic 궤적 사용 (착탄 후 장판 생성)
			return std::make_unique<ParabolicMovement>(params.ownGravity, params.launchAngle);

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
				ImGui::Text("=== Curve Settings ===");
				ImGui::Text("Curve Speed: %.2f", params.curveSpeed);
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
