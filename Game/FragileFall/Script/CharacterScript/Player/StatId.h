#pragma once

namespace game
{
	enum class StatType
	{
		// --- 공격 (Attack) ---
		AtkDmg, AtkSpeed, BulletRange, BulletSize, BulletSpeed,

		// --- 기술/처형 (Execution) ---
		Exe_FragileRegen,      // 처형 시 프레자일 게이지 회복값
		Exe_Range,             // 처형 사거리
		Exe_SplashDmg,         // 처형 시 주변 대미지
		Exe_SplashRange,       // 처형 대미지 범위
		Exe_DashChargeRegen,   // 처형 시 대시 횟수 회복
		Exe_HpRegen,           // 처형 시 체력 회복

		// --- 체력/생존 (Vitality) ---
		Hp_Max,                // 체력 증가 (HealthBoost)
		Hp_RegenOnClear,       // 스테이지 클리어 시 체력 회복
		Fragile_Max,           // 프레자일 최대값
		Fragile_RegenOnClear,  // 스테이지 클리어 시 프레자일 회복
		Fragile_GainRate,      // 프레자일 게이지 상승량 (피격 시 등)
		InvincibleTime,        // 피격 시 무적시간
		
		// --- 이동 (Movement & Dash) ---
		MoveSpeed,
		Dash_Distance,         // 대시 이동거리
		Dash_Cooldown,         // 대시 충전시간
		Dash_InvincibleTime,   // 대시 무적시간 증가

		// --- 특수 (Special) ---
		BulletDouble,

		Count
	};

	// 버프 종류
	enum class BuffId
	{
		Dash_MoveSpeed,     // 대시 후 n초간 이속 증가: (Duration + Bonus)
		Dash_AtkDmg,        // 대시 후 n초간 공격력 증가: (Duration + Bonus)
		Execution_AtkSpeed, // 처형 후 공속 증가(스택형 가능): (Duration + Bonus + MaxStacks) ← 확장

		Count,
	};

	enum class CalcType
	{
		Add,
		Mul,
	};

	struct StatValue
	{
		float add = 0.0f;
		float mul = 1.0f;
	};
}