#pragma once
#include "Script/CharacterScript/Player/StatId.h"

namespace game
{
	class PlayerControllerScript;

	// ═══════════════════════════════════════════════════════════════
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
	// 
	// 수명:
	//   - 정적 클래스, 에디터/게임 프로세스와 동일한 수명
	//   - 앱 종료 시 강화수치 초기화됨
	// ═══════════════════════════════════════════════════════════════
	class PlayerTemperManager
	{
	public:
		// ─────────────────────────────────────────────
		// 초기화
		// ─────────────────────────────────────────────
		//static void Initialize();

		// ─────────────────────────────────────────────
		// 핵심 기능: 플레이어에게 강화 적용
		// - player에서 Base값 읽음
		// - (Base + 합연산) × 곱연산 계산
		// - player의 실제값 변수에 설정
		// ─────────────────────────────────────────────
		//static void ApplyTemper(PlayerControllerScript* player);

		// ─────────────────────────────────────────────
		// 전체 강화수치 초기화 (합=0, 곱=1, 불린=false)
		// ─────────────────────────────────────────────
		//static void ResetAllTemper();

		// ─────────────────────────────────────────────
		// 합연산 강화수치 Setter/Getter
		// ─────────────────────────────────────────────
		static void SetAddAtkDmg(float value);
		static float GetAddAtkDmg();

		static void SetAddAtkSpeed(float value);
		static float GetAddAtkSpeed();

		static void SetAddBulletLifetime(float value);
		static float GetAddBulletLifetime();

		static void SetAddBulletRange(float value);
		static float GetAddBulletRange();

		static void SetAddBulletSizeScale(float value);
		static float GetAddBulletSizeScale();

		static void SetAddBulletSpeed(float value);
		static float GetAddBulletSpeed();

		// ─────────────────────────────────────────────
		// 곱연산 강화수치 Setter/Getter
		// ─────────────────────────────────────────────
		static void SetMulAtkDmg(float value);
		static float GetMulAtkDmg();

		static void SetMulAtkSpeed(float value);
		static float GetMulAtkSpeed();

		static void SetMulBulletLifetime(float value);
		static float GetMulBulletLifetime();

		static void SetMulBulletRange(float value);
		static float GetMulBulletRange();

		static void SetMulBulletSizeScale(float value);
		static float GetMulBulletSizeScale();

		static void SetMulBulletSpeed(float value);
		static float GetMulBulletSpeed();

		// ─────────────────────────────────────────────
		// 불린 강화 Setter/Getter
		// ─────────────────────────────────────────────
		//static void SetIsBulletDouble(bool value);
		//static bool GetIsBulletDouble();

		// Refactor
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