#pragma once

#include <Engine/Framework/Object/Component/Script.h>

namespace game
{
	class PlayerControllerScript;

	// ═══════════════════════════════════════════════════════════════
	// TemperTestScript - 강화 시스템 테스트용 스크립트
	// 
	// 기능:
	//   - 각 변수별 강화 단계 설정 (0~3)
	//   - 각 단계별 합연산(Add), 곱연산(Mul) 값 설정
	//   - Apply 버튼으로 플레이어에게 강화 적용
	//   - Reset 버튼으로 모든 강화 초기화
	//   - 현재 적용된 강화수치 표시
	// 
	// 사용법:
	//   - 씬의 아무 오브젝트에 이 스크립트 추가
	//   - OnGui에서 강화 설정 및 테스트
	// ═══════════════════════════════════════════════════════════════
	class TemperTestScript : public engine::Script<TemperTestScript>
	{
		REGISTER_SCRIPT(TemperTestScript, Script)

	private:
		// ─────────────────────────────────────────────
		// 플레이어 오브젝트 이름 (씬에서 검색용)
		// ─────────────────────────────────────────────
		std::string m_playerObjectName = "Player";

		// ─────────────────────────────────────────────
		// 현재 강화 단계 (0~3, 저장 안함)
		// 0 = 강화 없음, 1~3 = 해당 단계 강화
		// ─────────────────────────────────────────────
		int m_atkDmgLevel = 0;
		int m_atkSpeedLevel = 0;
		int m_bulletLifetimeLevel = 0;
		int m_bulletSizeScaleLevel = 0;
		int m_bulletSpeedLevel = 0;
		bool m_bulletDoubleEnabled = false;

		// ─────────────────────────────────────────────
		// 강화 단계별 합연산 값 (저장됨)
		// [0] = 1강, [1] = 2강, [2] = 3강
		// ─────────────────────────────────────────────
		float m_atkDmgAdd[3] = { 0.0f, 0.0f, 0.0f };
		float m_atkSpeedAdd[3] = { 0.0f, 0.0f, 0.0f };
		float m_bulletLifetimeAdd[3] = { 0.0f, 0.0f, 0.0f };
		float m_bulletSizeScaleAdd[3] = { 0.0f, 0.0f, 0.0f };
		float m_bulletSpeedAdd[3] = { 0.0f, 0.0f, 0.0f };

		// ─────────────────────────────────────────────
		// 강화 단계별 곱연산 값 (저장됨)
		// [0] = 1강, [1] = 2강, [2] = 3강
		// 기본값 1.0 (곱해도 변화 없음)
		// ─────────────────────────────────────────────
		float m_atkDmgMul[3] = { 1.0f, 1.0f, 1.0f };
		float m_atkSpeedMul[3] = { 1.0f, 1.0f, 1.0f };
		float m_bulletLifetimeMul[3] = { 1.0f, 1.0f, 1.0f };
		float m_bulletSizeScaleMul[3] = { 1.0f, 1.0f, 1.0f };
		float m_bulletSpeedMul[3] = { 1.0f, 1.0f, 1.0f };

		// ─────────────────────────────────────────────
		// 캐시된 플레이어 참조
		// ─────────────────────────────────────────────
		PlayerControllerScript* m_cachedPlayer = nullptr;

	public:
		void Start() override;

	private:
		// ─────────────────────────────────────────────
		// 플레이어 찾기 (씬에서 이름으로 검색)
		// ─────────────────────────────────────────────
		PlayerControllerScript* FindPlayer();

		// ─────────────────────────────────────────────
		// 강화 적용
		// ─────────────────────────────────────────────
		void ApplyTemperToPlayer();

		// ─────────────────────────────────────────────
		// 강화 초기화
		// ─────────────────────────────────────────────
		void ResetAllLevels();

	public:
		void OnGui() override;
		void Save(engine::json& j) const override;
		void Load(const engine::json& j) override;
	};
}
