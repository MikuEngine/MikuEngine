#include "GamePCH.h"
#include "PlayerTemperManager.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

namespace game
{
	namespace
	{
		// ═══════════════════════════════════════════════════════════════
		// 강화수치 저장 (익명 네임스페이스)
		// - 합연산: 기본값 0 (더해도 변화 없음)
		// - 곱연산: 기본값 1.0 (곱해도 변화 없음)
		// ═══════════════════════════════════════════════════════════════

		// 합연산 강화수치
		float g_addAtkDmg = 0.0f;
		float g_addAtkSpeed = 0.0f;
		float g_addBulletLifetime = 0.0f;
		float g_addBulletSizeScale = 0.0f;
		float g_addBulletSpeed = 0.0f;

		// 곱연산 강화수치
		float g_mulAtkDmg = 1.0f;
		float g_mulAtkSpeed = 1.0f;
		float g_mulBulletLifetime = 1.0f;
		float g_mulBulletSizeScale = 1.0f;
		float g_mulBulletSpeed = 1.0f;

		// 불린 강화
		bool g_isBulletDouble = false;

		// m_fireRate 계산용 상수: 기본 초당 발사 횟수 (공격속도 1.0배일 때)
		// 1.0 / 0.7 ≈ 1.428571
		constexpr float BASE_FIRE_PER_SEC = 1.0f / 0.7f;
	}

	void PlayerTemperManager::Initialize()
	{
		ResetAllTemper();
	}

	void PlayerTemperManager::ApplyTemper(PlayerControllerScript* player)
	{
		if (!player) return;

		// ═══════════════════════════════════════════════════════════════
		// Base값 읽기 → 강화 계산 → 실제값 설정
		// 공식: 실제값 = (Base + 합연산) × 곱연산
		// ═══════════════════════════════════════════════════════════════

		// 공격력
		float baseAtkDmg = player->GetBaseAtkDmg();
		float finalAtkDmg = (baseAtkDmg + g_addAtkDmg) * g_mulAtkDmg;
		player->SetPlayerAtkDmg(finalAtkDmg);

		// 공격속도
		float baseAtkSpeed = player->GetBaseAtkSpeed();
		float finalAtkSpeed = (baseAtkSpeed + g_addAtkSpeed) * g_mulAtkSpeed;
		player->SetAtkSpeed(finalAtkSpeed);

		// 발사 간격 계산: m_fireRate = 1 / (BASE_FIRE_PER_SEC * m_AtkSpeed)
		// 간략화: m_fireRate = 0.7 / m_AtkSpeed
		float fireRate = (finalAtkSpeed > 0.001f) ? (0.7f / finalAtkSpeed) : 0.7f;
		player->SetFireRate(fireRate);

		// 총알 수명
		float baseBulletLifetime = player->GetBaseBulletLifetime();
		float finalBulletLifetime = (baseBulletLifetime + g_addBulletLifetime) * g_mulBulletLifetime;
		player->SetBulletLifetime(finalBulletLifetime);

		// 총알 크기
		float baseBulletSizeScale = player->GetBaseBulletSizeScale();
		float finalBulletSizeScale = (baseBulletSizeScale + g_addBulletSizeScale) * g_mulBulletSizeScale;
		player->SetBulletSizeScale(finalBulletSizeScale);

		// 총알 속도
		float baseBulletSpeed = player->GetBaseBulletSpeed();
		float finalBulletSpeed = (baseBulletSpeed + g_addBulletSpeed) * g_mulBulletSpeed;
		player->SetBulletSpeed(finalBulletSpeed);

		// 더블샷
		player->SetIsBulletDouble(g_isBulletDouble);
	}

	void PlayerTemperManager::ResetAllTemper()
	{
		// 합연산 초기화 (0)
		g_addAtkDmg = 0.0f;
		g_addAtkSpeed = 0.0f;
		g_addBulletLifetime = 0.0f;
		g_addBulletSizeScale = 0.0f;
		g_addBulletSpeed = 0.0f;

		// 곱연산 초기화 (1.0)
		g_mulAtkDmg = 1.0f;
		g_mulAtkSpeed = 1.0f;
		g_mulBulletLifetime = 1.0f;
		g_mulBulletSizeScale = 1.0f;
		g_mulBulletSpeed = 1.0f;

		// 불린 초기화
		g_isBulletDouble = false;
	}

	// ═══════════════════════════════════════════════════════════════
	// 합연산 Setter/Getter
	// ═══════════════════════════════════════════════════════════════
	void PlayerTemperManager::SetAddAtkDmg(float value) { LOG_PRINT("[Temper] SetAddAtkDmg addr={} value={}", (void*)&g_addAtkDmg, value);  g_addAtkDmg = value; }
	float PlayerTemperManager::GetAddAtkDmg() { return g_addAtkDmg; }

	void PlayerTemperManager::SetAddAtkSpeed(float value) { g_addAtkSpeed = value; }
	float PlayerTemperManager::GetAddAtkSpeed() { return g_addAtkSpeed; }

	void PlayerTemperManager::SetAddBulletLifetime(float value) { g_addBulletLifetime = value; }
	float PlayerTemperManager::GetAddBulletLifetime() { return g_addBulletLifetime; }

	void PlayerTemperManager::SetAddBulletSizeScale(float value) { g_addBulletSizeScale = value; }
	float PlayerTemperManager::GetAddBulletSizeScale() { return g_addBulletSizeScale; }

	void PlayerTemperManager::SetAddBulletSpeed(float value) { g_addBulletSpeed = value; }
	float PlayerTemperManager::GetAddBulletSpeed() { return g_addBulletSpeed; }

	// ═══════════════════════════════════════════════════════════════
	// 곱연산 Setter/Getter
	// ═══════════════════════════════════════════════════════════════
	void PlayerTemperManager::SetMulAtkDmg(float value) { g_mulAtkDmg = value; }
	float PlayerTemperManager::GetMulAtkDmg() { return g_mulAtkDmg; }

	void PlayerTemperManager::SetMulAtkSpeed(float value) { g_mulAtkSpeed = value; }
	float PlayerTemperManager::GetMulAtkSpeed() { return g_mulAtkSpeed; }

	void PlayerTemperManager::SetMulBulletLifetime(float value) { g_mulBulletLifetime = value; }
	float PlayerTemperManager::GetMulBulletLifetime() { return g_mulBulletLifetime; }

	void PlayerTemperManager::SetMulBulletSizeScale(float value) { g_mulBulletSizeScale = value; }
	float PlayerTemperManager::GetMulBulletSizeScale() { return g_mulBulletSizeScale; }

	void PlayerTemperManager::SetMulBulletSpeed(float value) { g_mulBulletSpeed = value; }
	float PlayerTemperManager::GetMulBulletSpeed() { return g_mulBulletSpeed; }

	// ═══════════════════════════════════════════════════════════════
	// 불린 Setter/Getter
	// ═══════════════════════════════════════════════════════════════
	void PlayerTemperManager::SetIsBulletDouble(bool value) { g_isBulletDouble = value; }
	bool PlayerTemperManager::GetIsBulletDouble() { return g_isBulletDouble; }
}
