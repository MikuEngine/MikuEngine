#include "GamePCH.h"
#include "PlayerTemperManager.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

namespace game
{
	std::unordered_map<StatType, StatValue> PlayerTemperManager::m_stats;
	bool PlayerTemperManager::m_isBulletDouble = false;

	void PlayerTemperManager::Initialize()
	{
		ResetAllTemper();
	}

	void PlayerTemperManager::ResetAllTemper()
	{
		m_stats.clear();
		m_isBulletDouble = false;
	}
	
	void PlayerTemperManager::SetStat(StatType type, CalcType calc, float value)
	{
		if (calc == CalcType::Add)
			m_stats[type].add = value;
		else
			m_stats[type].mul = value;
	}

	float PlayerTemperManager::GetStat(StatType type, CalcType calc)
	{
		auto it = m_stats.find(type);
		if (it == m_stats.end())
		{
			// 데이터가 없다면 기본값 반환 (합연산은 0, 곱연산은 1)
			return (calc == CalcType::Add) ? 0.0f : 1.0f;
		}

		// 2. 요청한 연산 타입에 따라 값 반환
		if (calc == CalcType::Add)
		{
			return it->second.add;
		}
		else // CalcType::Mul
		{
			return it->second.mul;
		}
	}

	void PlayerTemperManager::ApplyTemper(PlayerControllerScript* player)
	{
		if (!player) return;

		// 공식 : 실제값 = (Base + 합연산) * 곱연산
		// 헬퍼 람다함수
		auto Apply = [&](StatType type, float baseValue, auto setterFunc)
			{
				const float add = GetStat(type, CalcType::Add);
				const float mul = GetStat(type, CalcType::Mul);
				const float finalValue = (baseValue + add) * mul;
				(player->*setterFunc)(finalValue);
				return finalValue;
			};

		// --- 공격 (Attack) ---
		Apply(StatType::AtkDmg, player->GetBaseAtkDmg(), &PlayerControllerScript::SetPlayerAtkDmg);
		Apply(StatType::BulletRange, player->GetBaseBulletRange(), &PlayerControllerScript::SetBulletRange);
		Apply(StatType::BulletSize, player->GetBaseBulletSizeScale(), &PlayerControllerScript::SetBulletSizeScale);
		Apply(StatType::BulletSpeed, player->GetBaseBulletSpeed(), &PlayerControllerScript::SetBulletSpeed);
		// --- 특별 처리 로직 ---
		float finalAtkSpeed = Apply(StatType::AtkSpeed, player->GetBaseAtkSpeed(), &PlayerControllerScript::SetAtkSpeed);
		float fireRate = (finalAtkSpeed > 0.001f) ? (0.7f / finalAtkSpeed) : 0.7f;
		player->SetFireRate(fireRate);

		// --- 기술/처형 (Execution) ---
		Apply(StatType::Exe_FragileRegen,    player->GetBaseExeFragileRegen(),    &PlayerControllerScript::SetExeFragileRegen);
		Apply(StatType::Exe_Range,           player->GetBaseExeRange(),           &PlayerControllerScript::SetExeRange);
		Apply(StatType::Exe_SplashDmg,       player->GetBaseExeSplashDmg(),       &PlayerControllerScript::SetExeSplashDmg);
		Apply(StatType::Exe_SplashRange,     player->GetBaseExeSplashRange(),     &PlayerControllerScript::SetExeSplashRange);
		Apply(StatType::Exe_DashChargeRegen, player->GetBaseExeDashChargeRegen(), &PlayerControllerScript::SetExeDashChargeRegen);
		Apply(StatType::Exe_HpRegen,         player->GetBaseExeHpRegen(),         &PlayerControllerScript::SetExeHpRegen);

		// --- 체력/생존 (Vitality) ---
		Apply(StatType::Hp_Max, player->GetBaseMaxHp(), &PlayerControllerScript::SetMaxHp);
		Apply(StatType::Hp_RegenOnClear, player->GetBaseHpRegenOnClear(), &PlayerControllerScript::SetHpRegenOnClear);
		Apply(StatType::Fragile_Max, player->GetBaseFragileMax(), &PlayerControllerScript::SetFragileMax);
		Apply(StatType::Fragile_RegenOnClear, player->GetBaseFragileRegenOnClear(), &PlayerControllerScript::SetFragileRegenOnClear);
		Apply(StatType::Fragile_GainRate, player->GetBaseFragileGainRate(), &PlayerControllerScript::SetFragileGainRate);
		Apply(StatType::InvincibleTime, player->GetBaseInvincibleTime(), &PlayerControllerScript::SetInvincibleTime);

		// --- 이동 (Movement) ---
		Apply(StatType::MoveSpeed, player->GetBaseMoveSpeed(), &PlayerControllerScript::SetMoveSpeed);
		Apply(StatType::Dash_Distance, player->GetBaseDashDistance(), &PlayerControllerScript::SetDashDistance);
		Apply(StatType::Dash_Cooldown, player->GetBaseDashCooldown(), &PlayerControllerScript::SetDashCooldown);
		Apply(StatType::Dash_InvincibleTime, player->GetBaseDashInvincibleTime(), &PlayerControllerScript::SetDashInvincibleTime);
	}
}
