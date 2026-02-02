#include "GamePCH.h"
#include "TemperTestScript.h"

#include "Manager/PlayerTemperManager.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>

namespace game
{
	void TemperTestScript::Start()
	{
		// 시작 시 플레이어 캐싱
		m_cachedPlayer = FindPlayer();
	}

	PlayerControllerScript* TemperTestScript::FindPlayer()
	{
		auto* scene = engine::SceneManager::Get().GetScene();
		if (!scene) return nullptr;

		auto* playerGO = scene->FindGameObject(m_playerObjectName);
		if (!playerGO) return nullptr;

		return playerGO->GetComponent<PlayerControllerScript>();
	}

	void TemperTestScript::ApplyTemperToPlayer()
	{
		// 플레이어 찾기 (캐시가 없으면 다시 검색)
		if (!m_cachedPlayer)
		{
			m_cachedPlayer = FindPlayer();
		}

		if (!m_cachedPlayer)
		{
			return;
		}

		// ═══════════════════════════════════════════════════════════════
		// 현재 강화 단계에 따른 강화수치 계산 및 설정
		// 단계 0 = 강화 없음 (add=0, mul=1.0)
		// 단계 1~3 = 해당 인덱스의 값 사용
		// ═══════════════════════════════════════════════════════════════

		// 공격력
		float addAtkDmg = (m_atkDmgLevel > 0) ? m_atkDmgAdd[m_atkDmgLevel - 1] : 0.0f;
		float mulAtkDmg = (m_atkDmgLevel > 0) ? m_atkDmgMul[m_atkDmgLevel - 1] : 1.0f;
		PlayerTemperManager::SetAddAtkDmg(addAtkDmg);
		PlayerTemperManager::SetMulAtkDmg(mulAtkDmg);

		// 공격속도
		float addAtkSpeed = (m_atkSpeedLevel > 0) ? m_atkSpeedAdd[m_atkSpeedLevel - 1] : 0.0f;
		float mulAtkSpeed = (m_atkSpeedLevel > 0) ? m_atkSpeedMul[m_atkSpeedLevel - 1] : 1.0f;
		PlayerTemperManager::SetAddAtkSpeed(addAtkSpeed);
		PlayerTemperManager::SetMulAtkSpeed(mulAtkSpeed);

		// 총알 수명
		float addBulletLifetime = (m_bulletLifetimeLevel > 0) ? m_bulletLifetimeAdd[m_bulletLifetimeLevel - 1] : 0.0f;
		float mulBulletLifetime = (m_bulletLifetimeLevel > 0) ? m_bulletLifetimeMul[m_bulletLifetimeLevel - 1] : 1.0f;
		PlayerTemperManager::SetAddBulletLifetime(addBulletLifetime);
		PlayerTemperManager::SetMulBulletLifetime(mulBulletLifetime);

		// 총알 크기
		float addBulletSizeScale = (m_bulletSizeScaleLevel > 0) ? m_bulletSizeScaleAdd[m_bulletSizeScaleLevel - 1] : 0.0f;
		float mulBulletSizeScale = (m_bulletSizeScaleLevel > 0) ? m_bulletSizeScaleMul[m_bulletSizeScaleLevel - 1] : 1.0f;
		PlayerTemperManager::SetAddBulletSizeScale(addBulletSizeScale);
		PlayerTemperManager::SetMulBulletSizeScale(mulBulletSizeScale);

		// 총알 속도
		float addBulletSpeed = (m_bulletSpeedLevel > 0) ? m_bulletSpeedAdd[m_bulletSpeedLevel - 1] : 0.0f;
		float mulBulletSpeed = (m_bulletSpeedLevel > 0) ? m_bulletSpeedMul[m_bulletSpeedLevel - 1] : 1.0f;
		PlayerTemperManager::SetAddBulletSpeed(addBulletSpeed);
		PlayerTemperManager::SetMulBulletSpeed(mulBulletSpeed);

		// 더블샷
		PlayerTemperManager::SetIsBulletDouble(m_bulletDoubleEnabled);

		// 플레이어에게 적용
		PlayerTemperManager::ApplyTemper(m_cachedPlayer);
	}

	void TemperTestScript::ResetAllLevels()
	{
		m_atkDmgLevel = 0;
		m_atkSpeedLevel = 0;
		m_bulletLifetimeLevel = 0;
		m_bulletSizeScaleLevel = 0;
		m_bulletSpeedLevel = 0;
		m_bulletDoubleEnabled = false;

		// PlayerTemperManager도 초기화
		PlayerTemperManager::ResetAllTemper();

		// 플레이어에게 적용 (강화 없는 상태로)
		if (!m_cachedPlayer)
		{
			m_cachedPlayer = FindPlayer();
		}
		if (m_cachedPlayer)
		{
			PlayerTemperManager::ApplyTemper(m_cachedPlayer);
		}
	}

	void TemperTestScript::OnGui()
	{
		ImGui::Indent();
		ImGui::Text("=== Temper Test Script ===");
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Test tool for PlayerTemperManager");

		// ═══════════════════════════════════════════════════════════════
		// 플레이어 검색 설정
		// ═══════════════════════════════════════════════════════════════
		ImGui::Separator();
		ImGui::Text("Player Object Name:");
		ImGui::InputText("##PlayerName", &m_playerObjectName);

		// 플레이어 상태 표시
		if (!m_cachedPlayer)
		{
			m_cachedPlayer = FindPlayer();
		}
		if (m_cachedPlayer)
		{
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "[OK] Player Found");
		}
		else
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "[ERROR] Player Not Found");
		}

		// ═══════════════════════════════════════════════════════════════
		// 강화 단계 설정 (0~3)
		// ═══════════════════════════════════════════════════════════════
		ImGui::Separator();
		ImGui::Text("=== Enhancement Levels (0~3) ===");
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "0 = No enhancement, 1~3 = Enhancement level");

		ImGui::SliderInt("Atk Damage Level", &m_atkDmgLevel, 0, 3);
		ImGui::SliderInt("Atk Speed Level", &m_atkSpeedLevel, 0, 3);
		ImGui::SliderInt("Bullet Lifetime Level", &m_bulletLifetimeLevel, 0, 3);
		ImGui::SliderInt("Bullet Size Scale Level", &m_bulletSizeScaleLevel, 0, 3);
		ImGui::SliderInt("Bullet Speed Level", &m_bulletSpeedLevel, 0, 3);
		ImGui::Checkbox("Bullet Double (true/false)", &m_bulletDoubleEnabled);

		// ═══════════════════════════════════════════════════════════════
		// 단계별 Add/Mul 값 설정
		// ═══════════════════════════════════════════════════════════════
		ImGui::Separator();
		ImGui::Text("=== Enhancement Values per Level ===");
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Formula: (Base + Add) x Mul");

		// 공격력
		if (ImGui::CollapsingHeader("Atk Damage Values"))
		{
			ImGui::Indent();
			ImGui::Text("Level 1:"); ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("Add##AtkDmg1", &m_atkDmgAdd[0], 0.5f); ImGui::SameLine();
			ImGui::DragFloat("Mul##AtkDmg1", &m_atkDmgMul[0], 0.05f, 0.1f, 10.0f);
			ImGui::PopItemWidth();

			ImGui::Text("Level 2:"); ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("Add##AtkDmg2", &m_atkDmgAdd[1], 0.5f); ImGui::SameLine();
			ImGui::DragFloat("Mul##AtkDmg2", &m_atkDmgMul[1], 0.05f, 0.1f, 10.0f);
			ImGui::PopItemWidth();

			ImGui::Text("Level 3:"); ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("Add##AtkDmg3", &m_atkDmgAdd[2], 0.5f); ImGui::SameLine();
			ImGui::DragFloat("Mul##AtkDmg3", &m_atkDmgMul[2], 0.05f, 0.1f, 10.0f);
			ImGui::PopItemWidth();
			ImGui::Unindent();
		}

		// 공격속도
		if (ImGui::CollapsingHeader("Atk Speed Values"))
		{
			ImGui::Indent();
			ImGui::Text("Level 1:"); ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("Add##AtkSpd1", &m_atkSpeedAdd[0], 0.05f); ImGui::SameLine();
			ImGui::DragFloat("Mul##AtkSpd1", &m_atkSpeedMul[0], 0.05f, 0.1f, 10.0f);
			ImGui::PopItemWidth();

			ImGui::Text("Level 2:"); ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("Add##AtkSpd2", &m_atkSpeedAdd[1], 0.05f); ImGui::SameLine();
			ImGui::DragFloat("Mul##AtkSpd2", &m_atkSpeedMul[1], 0.05f, 0.1f, 10.0f);
			ImGui::PopItemWidth();

			ImGui::Text("Level 3:"); ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("Add##AtkSpd3", &m_atkSpeedAdd[2], 0.05f); ImGui::SameLine();
			ImGui::DragFloat("Mul##AtkSpd3", &m_atkSpeedMul[2], 0.05f, 0.1f, 10.0f);
			ImGui::PopItemWidth();
			ImGui::Unindent();
		}

		// 총알 수명
		if (ImGui::CollapsingHeader("Bullet Lifetime Values"))
		{
			ImGui::Indent();
			ImGui::Text("Level 1:"); ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("Add##BltLife1", &m_bulletLifetimeAdd[0], 0.1f); ImGui::SameLine();
			ImGui::DragFloat("Mul##BltLife1", &m_bulletLifetimeMul[0], 0.05f, 0.1f, 10.0f);
			ImGui::PopItemWidth();

			ImGui::Text("Level 2:"); ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("Add##BltLife2", &m_bulletLifetimeAdd[1], 0.1f); ImGui::SameLine();
			ImGui::DragFloat("Mul##BltLife2", &m_bulletLifetimeMul[1], 0.05f, 0.1f, 10.0f);
			ImGui::PopItemWidth();

			ImGui::Text("Level 3:"); ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("Add##BltLife3", &m_bulletLifetimeAdd[2], 0.1f); ImGui::SameLine();
			ImGui::DragFloat("Mul##BltLife3", &m_bulletLifetimeMul[2], 0.05f, 0.1f, 10.0f);
			ImGui::PopItemWidth();
			ImGui::Unindent();
		}

		// 총알 크기
		if (ImGui::CollapsingHeader("Bullet Size Scale Values"))
		{
			ImGui::Indent();
			ImGui::Text("Level 1:"); ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("Add##BltSize1", &m_bulletSizeScaleAdd[0], 0.05f); ImGui::SameLine();
			ImGui::DragFloat("Mul##BltSize1", &m_bulletSizeScaleMul[0], 0.05f, 0.1f, 10.0f);
			ImGui::PopItemWidth();

			ImGui::Text("Level 2:"); ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("Add##BltSize2", &m_bulletSizeScaleAdd[1], 0.05f); ImGui::SameLine();
			ImGui::DragFloat("Mul##BltSize2", &m_bulletSizeScaleMul[1], 0.05f, 0.1f, 10.0f);
			ImGui::PopItemWidth();

			ImGui::Text("Level 3:"); ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("Add##BltSize3", &m_bulletSizeScaleAdd[2], 0.05f); ImGui::SameLine();
			ImGui::DragFloat("Mul##BltSize3", &m_bulletSizeScaleMul[2], 0.05f, 0.1f, 10.0f);
			ImGui::PopItemWidth();
			ImGui::Unindent();
		}

		// 총알 속도
		if (ImGui::CollapsingHeader("Bullet Speed Values"))
		{
			ImGui::Indent();
			ImGui::Text("Level 1:"); ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("Add##BltSpd1", &m_bulletSpeedAdd[0], 0.5f); ImGui::SameLine();
			ImGui::DragFloat("Mul##BltSpd1", &m_bulletSpeedMul[0], 0.05f, 0.1f, 10.0f);
			ImGui::PopItemWidth();

			ImGui::Text("Level 2:"); ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("Add##BltSpd2", &m_bulletSpeedAdd[1], 0.5f); ImGui::SameLine();
			ImGui::DragFloat("Mul##BltSpd2", &m_bulletSpeedMul[1], 0.05f, 0.1f, 10.0f);
			ImGui::PopItemWidth();

			ImGui::Text("Level 3:"); ImGui::SameLine();
			ImGui::PushItemWidth(80);
			ImGui::DragFloat("Add##BltSpd3", &m_bulletSpeedAdd[2], 0.5f); ImGui::SameLine();
			ImGui::DragFloat("Mul##BltSpd3", &m_bulletSpeedMul[2], 0.05f, 0.1f, 10.0f);
			ImGui::PopItemWidth();
			ImGui::Unindent();
		}

		// ═══════════════════════════════════════════════════════════════
		// Apply / Reset 버튼
		// ═══════════════════════════════════════════════════════════════
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::Button("Apply Enhancement", ImVec2(150, 30)))
		{
			ApplyTemperToPlayer();
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset All", ImVec2(100, 30)))
		{
			ResetAllLevels();
		}

		// ═══════════════════════════════════════════════════════════════
		// 현재 적용된 강화수치 표시
		// ═══════════════════════════════════════════════════════════════
		ImGui::Separator();
		ImGui::Text("=== Current Applied Values (Read-only) ===");
		ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Values currently set in PlayerTemperManager");

		ImGui::BeginDisabled();
		
		// 합연산
		ImGui::Text("Add Values:");
		float addAtkDmg = PlayerTemperManager::GetAddAtkDmg();
		float addAtkSpeed = PlayerTemperManager::GetAddAtkSpeed();
		float addBulletLifetime = PlayerTemperManager::GetAddBulletLifetime();
		float addBulletSizeScale = PlayerTemperManager::GetAddBulletSizeScale();
		float addBulletSpeed = PlayerTemperManager::GetAddBulletSpeed();
		ImGui::DragFloat("Add AtkDmg##Current", &addAtkDmg);
		ImGui::DragFloat("Add AtkSpeed##Current", &addAtkSpeed);
		ImGui::DragFloat("Add BulletLifetime##Current", &addBulletLifetime);
		ImGui::DragFloat("Add BulletSizeScale##Current", &addBulletSizeScale);
		ImGui::DragFloat("Add BulletSpeed##Current", &addBulletSpeed);

		// 곱연산
		ImGui::Spacing();
		ImGui::Text("Mul Values:");
		float mulAtkDmg = PlayerTemperManager::GetMulAtkDmg();
		float mulAtkSpeed = PlayerTemperManager::GetMulAtkSpeed();
		float mulBulletLifetime = PlayerTemperManager::GetMulBulletLifetime();
		float mulBulletSizeScale = PlayerTemperManager::GetMulBulletSizeScale();
		float mulBulletSpeed = PlayerTemperManager::GetMulBulletSpeed();
		ImGui::DragFloat("Mul AtkDmg##Current", &mulAtkDmg);
		ImGui::DragFloat("Mul AtkSpeed##Current", &mulAtkSpeed);
		ImGui::DragFloat("Mul BulletLifetime##Current", &mulBulletLifetime);
		ImGui::DragFloat("Mul BulletSizeScale##Current", &mulBulletSizeScale);
		ImGui::DragFloat("Mul BulletSpeed##Current", &mulBulletSpeed);

		// 불린
		ImGui::Spacing();
		bool isBulletDouble = PlayerTemperManager::GetIsBulletDouble();
		ImGui::Checkbox("Is Bullet Double##Current", &isBulletDouble);

		ImGui::EndDisabled();

		ImGui::Unindent();
	}

	void TemperTestScript::Save(engine::json& j) const
	{
		j["PlayerObjectName"] = m_playerObjectName;

		// 공격력 단계별 값
		j["AtkDmgAdd"] = { m_atkDmgAdd[0], m_atkDmgAdd[1], m_atkDmgAdd[2] };
		j["AtkDmgMul"] = { m_atkDmgMul[0], m_atkDmgMul[1], m_atkDmgMul[2] };

		// 공격속도 단계별 값
		j["AtkSpeedAdd"] = { m_atkSpeedAdd[0], m_atkSpeedAdd[1], m_atkSpeedAdd[2] };
		j["AtkSpeedMul"] = { m_atkSpeedMul[0], m_atkSpeedMul[1], m_atkSpeedMul[2] };

		// 총알 수명 단계별 값
		j["BulletLifetimeAdd"] = { m_bulletLifetimeAdd[0], m_bulletLifetimeAdd[1], m_bulletLifetimeAdd[2] };
		j["BulletLifetimeMul"] = { m_bulletLifetimeMul[0], m_bulletLifetimeMul[1], m_bulletLifetimeMul[2] };

		// 총알 크기 단계별 값
		j["BulletSizeScaleAdd"] = { m_bulletSizeScaleAdd[0], m_bulletSizeScaleAdd[1], m_bulletSizeScaleAdd[2] };
		j["BulletSizeScaleMul"] = { m_bulletSizeScaleMul[0], m_bulletSizeScaleMul[1], m_bulletSizeScaleMul[2] };

		// 총알 속도 단계별 값
		j["BulletSpeedAdd"] = { m_bulletSpeedAdd[0], m_bulletSpeedAdd[1], m_bulletSpeedAdd[2] };
		j["BulletSpeedMul"] = { m_bulletSpeedMul[0], m_bulletSpeedMul[1], m_bulletSpeedMul[2] };
	}

	void TemperTestScript::Load(const engine::json& j)
	{
		if (j.contains("PlayerObjectName"))
			m_playerObjectName = j["PlayerObjectName"].get<std::string>();

		// 공격력 단계별 값
		if (j.contains("AtkDmgAdd") && j["AtkDmgAdd"].is_array() && j["AtkDmgAdd"].size() >= 3)
		{
			m_atkDmgAdd[0] = j["AtkDmgAdd"][0].get<float>();
			m_atkDmgAdd[1] = j["AtkDmgAdd"][1].get<float>();
			m_atkDmgAdd[2] = j["AtkDmgAdd"][2].get<float>();
		}
		if (j.contains("AtkDmgMul") && j["AtkDmgMul"].is_array() && j["AtkDmgMul"].size() >= 3)
		{
			m_atkDmgMul[0] = j["AtkDmgMul"][0].get<float>();
			m_atkDmgMul[1] = j["AtkDmgMul"][1].get<float>();
			m_atkDmgMul[2] = j["AtkDmgMul"][2].get<float>();
		}

		// 공격속도 단계별 값
		if (j.contains("AtkSpeedAdd") && j["AtkSpeedAdd"].is_array() && j["AtkSpeedAdd"].size() >= 3)
		{
			m_atkSpeedAdd[0] = j["AtkSpeedAdd"][0].get<float>();
			m_atkSpeedAdd[1] = j["AtkSpeedAdd"][1].get<float>();
			m_atkSpeedAdd[2] = j["AtkSpeedAdd"][2].get<float>();
		}
		if (j.contains("AtkSpeedMul") && j["AtkSpeedMul"].is_array() && j["AtkSpeedMul"].size() >= 3)
		{
			m_atkSpeedMul[0] = j["AtkSpeedMul"][0].get<float>();
			m_atkSpeedMul[1] = j["AtkSpeedMul"][1].get<float>();
			m_atkSpeedMul[2] = j["AtkSpeedMul"][2].get<float>();
		}

		// 총알 수명 단계별 값
		if (j.contains("BulletLifetimeAdd") && j["BulletLifetimeAdd"].is_array() && j["BulletLifetimeAdd"].size() >= 3)
		{
			m_bulletLifetimeAdd[0] = j["BulletLifetimeAdd"][0].get<float>();
			m_bulletLifetimeAdd[1] = j["BulletLifetimeAdd"][1].get<float>();
			m_bulletLifetimeAdd[2] = j["BulletLifetimeAdd"][2].get<float>();
		}
		if (j.contains("BulletLifetimeMul") && j["BulletLifetimeMul"].is_array() && j["BulletLifetimeMul"].size() >= 3)
		{
			m_bulletLifetimeMul[0] = j["BulletLifetimeMul"][0].get<float>();
			m_bulletLifetimeMul[1] = j["BulletLifetimeMul"][1].get<float>();
			m_bulletLifetimeMul[2] = j["BulletLifetimeMul"][2].get<float>();
		}

		// 총알 크기 단계별 값
		if (j.contains("BulletSizeScaleAdd") && j["BulletSizeScaleAdd"].is_array() && j["BulletSizeScaleAdd"].size() >= 3)
		{
			m_bulletSizeScaleAdd[0] = j["BulletSizeScaleAdd"][0].get<float>();
			m_bulletSizeScaleAdd[1] = j["BulletSizeScaleAdd"][1].get<float>();
			m_bulletSizeScaleAdd[2] = j["BulletSizeScaleAdd"][2].get<float>();
		}
		if (j.contains("BulletSizeScaleMul") && j["BulletSizeScaleMul"].is_array() && j["BulletSizeScaleMul"].size() >= 3)
		{
			m_bulletSizeScaleMul[0] = j["BulletSizeScaleMul"][0].get<float>();
			m_bulletSizeScaleMul[1] = j["BulletSizeScaleMul"][1].get<float>();
			m_bulletSizeScaleMul[2] = j["BulletSizeScaleMul"][2].get<float>();
		}

		// 총알 속도 단계별 값
		if (j.contains("BulletSpeedAdd") && j["BulletSpeedAdd"].is_array() && j["BulletSpeedAdd"].size() >= 3)
		{
			m_bulletSpeedAdd[0] = j["BulletSpeedAdd"][0].get<float>();
			m_bulletSpeedAdd[1] = j["BulletSpeedAdd"][1].get<float>();
			m_bulletSpeedAdd[2] = j["BulletSpeedAdd"][2].get<float>();
		}
		if (j.contains("BulletSpeedMul") && j["BulletSpeedMul"].is_array() && j["BulletSpeedMul"].size() >= 3)
		{
			m_bulletSpeedMul[0] = j["BulletSpeedMul"][0].get<float>();
			m_bulletSpeedMul[1] = j["BulletSpeedMul"][1].get<float>();
			m_bulletSpeedMul[2] = j["BulletSpeedMul"][2].get<float>();
		}
	}
}
