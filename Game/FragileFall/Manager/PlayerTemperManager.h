#pragma once
#include <unordered_map>
#include "Script/CharacterScript/Player/StatId.h"

// PlayerTemperManager - 플레이어 강화 시스템 관리자
// 
// 역할:
//   - 강화수치(합연산, 곱연산) 저장 및 관리
//   - PlayerControllerScript의 Base값을 읽어 강화 적용 후 실제값 계산
//   - 공식: 실제값 = (Base + 합연산) × 곱연산
// 
// 사용법:
//   - PlayerControllerScript::Load() 끝에서 ApplyTemper(this) 호출
//   - OnGui에서 Base값 변경 시 ApplyTemper(this) 호출
//   - 강화 획득 시 Set~() 함수로 강화수치 설정 후 ApplyTemper() 호출

namespace game
{
	class PlayerControllerScript;

	class PlayerTemperManager
	{
	public:
		static void Initialize();
		static void ResetAllTemper();
		static void ApplyTemper(PlayerControllerScript* player);

		// Setter / Getter 함수는 하나씩
		static void SetStat(StatType type, CalcType calc, float value);
		static float GetStat(StatType type, CalcType calc);

		static void SetIsBulletDouble(bool value) { m_isBulletDouble = value; }
		static bool GetIsBulletDouble() { return m_isBulletDouble; }

	private:
		static std::unordered_map<StatType, StatValue> m_stats;
		static bool m_isBulletDouble;
	};
}