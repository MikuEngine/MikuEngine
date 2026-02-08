#include "GamePCH.h"
#include "BuffManager.h"

namespace game
{
	namespace
	{
		// ═══════════════════════════════════════════════════════════════
		// 버프 설정값 (익명 네임스페이스)
		// ═══════════════════════════════════════════════════════════════
		
		// 대시 버프 설정
		float g_dashBuffDuration = 5.0f;            // 지속 시간 (초)
		float g_dashBuffMoveSpeedBonus = 0.1f;      // 이동속도 보너스 (10% = 0.1)
		
		// 처형 버프 설정
		float g_executionBuffDuration = 10.0f;      // 지속 시간 (초)
		float g_executionBuffAtkSpeedBonus = 0.1f;  // 공격속도 보너스 (스택당 10% = 0.1)
		int g_executionBuffMaxStacks = 3;           // 최대 스택

		// ═══════════════════════════════════════════════════════════════
		// 대시 버프 런타임 상태
		// ═══════════════════════════════════════════════════════════════
		bool g_isDashBuffActive = false;
		float g_dashBuffTimer = 0.0f;
		float g_buffMoveSpeedMultiplier = 1.0f;     // 이동속도 배율

		// ═══════════════════════════════════════════════════════════════
		// 처형 버프 런타임 상태
		// ═══════════════════════════════════════════════════════════════
		bool g_isExecutionBuffActive = false;
		float g_executionBuffTimer = 0.0f;
		int g_executionBuffStacks = 0;
		float g_buffAtkSpeedMultiplier = 1.0f;      // 공격속도 배율

		// ═══════════════════════════════════════════════════════════════
		// 콜백 함수 포인터 (확장용)
		// ═══════════════════════════════════════════════════════════════
		BuffManager::BuffCallback g_onDashBuffApplied = nullptr;
		BuffManager::BuffCallback g_onDashBuffRemoved = nullptr;
		BuffManager::BuffCallback g_onExecutionBuffApplied = nullptr;
		BuffManager::BuffCallback g_onExecutionBuffRemoved = nullptr;
	}

	// ═══════════════════════════════════════════════════════════════
	// 초기화 및 업데이트
	// ═══════════════════════════════════════════════════════════════
	void BuffManager::Initialize()
	{
		Reset();
	}

	void BuffManager::Update(float deltaTime)
	{
		// ─────────────────────────────────────────────
		// 대시 버프 타이머
		// ─────────────────────────────────────────────
		if (g_isDashBuffActive)
		{
			g_dashBuffTimer -= deltaTime;
			if (g_dashBuffTimer <= 0.0f)
			{
				// 버프 해제
				g_isDashBuffActive = false;
				g_dashBuffTimer = 0.0f;
				g_buffMoveSpeedMultiplier = 1.0f;

				// 콜백 호출
				if (g_onDashBuffRemoved)
					g_onDashBuffRemoved(0);
			}
		}

		// ─────────────────────────────────────────────
		// 처형 버프 타이머
		// ─────────────────────────────────────────────
		if (g_isExecutionBuffActive)
		{
			g_executionBuffTimer -= deltaTime;
			if (g_executionBuffTimer <= 0.0f)
			{
				// 버프 해제
				g_isExecutionBuffActive = false;
				g_executionBuffTimer = 0.0f;
				g_executionBuffStacks = 0;
				g_buffAtkSpeedMultiplier = 1.0f;

				// 콜백 호출
				if (g_onExecutionBuffRemoved)
					g_onExecutionBuffRemoved(0);
			}
		}
	}

	void BuffManager::Reset()
	{
		// 런타임 상태 초기화
		g_isDashBuffActive = false;
		g_dashBuffTimer = 0.0f;
		g_buffMoveSpeedMultiplier = 1.0f;

		g_isExecutionBuffActive = false;
		g_executionBuffTimer = 0.0f;
		g_executionBuffStacks = 0;
		g_buffAtkSpeedMultiplier = 1.0f;

		// 콜백 초기화
		g_onDashBuffApplied = nullptr;
		g_onDashBuffRemoved = nullptr;
		g_onExecutionBuffApplied = nullptr;
		g_onExecutionBuffRemoved = nullptr;
	}

	// ═══════════════════════════════════════════════════════════════
	// 버프 트리거
	// ═══════════════════════════════════════════════════════════════
	void BuffManager::OnDashEnded()
	{
		// 이미 활성화 중이면 타이머만 리셋
		if (g_isDashBuffActive)
		{
			g_dashBuffTimer = g_dashBuffDuration;
			return;
		}

		// 버프 활성화
		g_isDashBuffActive = true;
		g_dashBuffTimer = g_dashBuffDuration;
		g_buffMoveSpeedMultiplier = 1.0f + g_dashBuffMoveSpeedBonus;  // 1.1 = 10% 증가

		// 콜백 호출
		if (g_onDashBuffApplied)
			g_onDashBuffApplied(0);
	}

	void BuffManager::OnExecutionCompleted()
	{
		// 이미 활성화 중이면 타이머 리셋 + 스택 증가
		if (g_isExecutionBuffActive)
		{
			g_executionBuffTimer = g_executionBuffDuration;

			// 스택 증가 (최대치 체크)
			if (g_executionBuffStacks < g_executionBuffMaxStacks)
			{
				g_executionBuffStacks++;
				// 공격속도 배율 재계산: 1.0 + (스택 * 보너스)
				g_buffAtkSpeedMultiplier = 1.0f + (g_executionBuffStacks * g_executionBuffAtkSpeedBonus);

				// 콜백 호출 (스택 증가 시에도 호출)
				if (g_onExecutionBuffApplied)
					g_onExecutionBuffApplied(g_executionBuffStacks);
			}
			return;
		}

		// 버프 활성화 (첫 스택)
		g_isExecutionBuffActive = true;
		g_executionBuffTimer = g_executionBuffDuration;
		g_executionBuffStacks = 1;
		g_buffAtkSpeedMultiplier = 1.0f + g_executionBuffAtkSpeedBonus;  // 1.1 = 10% 증가

		// 콜백 호출
		if (g_onExecutionBuffApplied)
			g_onExecutionBuffApplied(g_executionBuffStacks);
	}

	// ═══════════════════════════════════════════════════════════════
	// 배율 조회
	// ═══════════════════════════════════════════════════════════════
	float BuffManager::GetMoveSpeedMultiplier()
	{
		return g_buffMoveSpeedMultiplier;
	}

	float BuffManager::GetAtkSpeedMultiplier()
	{
		return g_buffAtkSpeedMultiplier;
	}

	// ═══════════════════════════════════════════════════════════════
	// 버프 상태 조회
	// ═══════════════════════════════════════════════════════════════
	bool BuffManager::IsDashBuffActive()
	{
		return g_isDashBuffActive;
	}

	float BuffManager::GetDashBuffTimer()
	{
		return g_dashBuffTimer;
	}

	bool BuffManager::IsExecutionBuffActive()
	{
		return g_isExecutionBuffActive;
	}

	float BuffManager::GetExecutionBuffTimer()
	{
		return g_executionBuffTimer;
	}

	int BuffManager::GetExecutionBuffStacks()
	{
		return g_executionBuffStacks;
	}

	// ═══════════════════════════════════════════════════════════════
	// 대시 버프 설정
	// ═══════════════════════════════════════════════════════════════
	void BuffManager::SetDashBuffDuration(float value)
	{
		g_dashBuffDuration = value;
	}

	float BuffManager::GetDashBuffDuration()
	{
		return g_dashBuffDuration;
	}

	void BuffManager::SetDashBuffMoveSpeedBonus(float value)
	{
		g_dashBuffMoveSpeedBonus = value;
	}

	float BuffManager::GetDashBuffMoveSpeedBonus()
	{
		return g_dashBuffMoveSpeedBonus;
	}

	// ═══════════════════════════════════════════════════════════════
	// 처형 버프 설정
	// ═══════════════════════════════════════════════════════════════
	void BuffManager::SetExecutionBuffDuration(float value)
	{
		g_executionBuffDuration = value;
	}

	float BuffManager::GetExecutionBuffDuration()
	{
		return g_executionBuffDuration;
	}

	void BuffManager::SetExecutionBuffAtkSpeedBonus(float value)
	{
		g_executionBuffAtkSpeedBonus = value;
	}

	float BuffManager::GetExecutionBuffAtkSpeedBonus()
	{
		return g_executionBuffAtkSpeedBonus;
	}

	void BuffManager::SetExecutionBuffMaxStacks(int value)
	{
		g_executionBuffMaxStacks = value;
	}

	int BuffManager::GetExecutionBuffMaxStacks()
	{
		return g_executionBuffMaxStacks;
	}

	// ═══════════════════════════════════════════════════════════════
	// 콜백 등록
	// ═══════════════════════════════════════════════════════════════
	void BuffManager::RegisterDashBuffAppliedCallback(BuffCallback callback)
	{
		g_onDashBuffApplied = callback;
	}

	void BuffManager::RegisterDashBuffRemovedCallback(BuffCallback callback)
	{
		g_onDashBuffRemoved = callback;
	}

	void BuffManager::RegisterExecutionBuffAppliedCallback(BuffCallback callback)
	{
		g_onExecutionBuffApplied = callback;
	}

	void BuffManager::RegisterExecutionBuffRemovedCallback(BuffCallback callback)
	{
		g_onExecutionBuffRemoved = callback;
	}
}
