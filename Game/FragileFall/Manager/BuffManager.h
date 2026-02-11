#pragma once

namespace game
{
	class PlayerControllerScript;

	// ═══════════════════════════════════════════════════════════════
	// BuffManager - 플레이어 버프 시스템 관리자
	// 
	// 역할:
	//   - 모든 버프 상태 관리 (타이머, 활성 여부, 스택, 배율)
	//   - 버프 트리거 처리 (대시 종료, 처형 완료)
	//   - 타이머 업데이트 (매 프레임)
	//   - 배율 제공 (이동속도, 공격속도)
	// 
	// 사용법:
	//   - 게임 시작 시 Initialize() 호출
	//   - 매 프레임 Update(deltaTime) 호출 (PlayerControllerScript::UpdateGameLogic)
	//   - 트리거 전달: OnDashEnded(), OnExecutionCompleted()
	//   - 배율 가져오기: GetMoveSpeedMultiplier(), GetAtkSpeedMultiplier()
	// 
	// 수명:
	//   - 정적 클래스, 에디터/게임 프로세스와 동일한 수명
	//   - 플레이어는 1개이므로 정적 변수로 상태 관리
	// ═══════════════════════════════════════════════════════════════
	class BuffManager
	{
	public:
		// ─────────────────────────────────────────────
		// 초기화 및 업데이트
		// ─────────────────────────────────────────────
		static void Initialize();
		static void Update(float deltaTime);  // 매 프레임 호출 (타이머 갱신)
		static void Reset();                  // 모든 버프 상태 초기화

		// ─────────────────────────────────────────────
		// 버프 트리거 (PlayerControllerScript에서 호출)
		// ─────────────────────────────────────────────
		static void OnDashEnded();           // 대시 종료 시 호출
		static void OnExecutionCompleted();  // 처형 완료 시 호출

		// ─────────────────────────────────────────────
		// 배율 조회 (PlayerControllerScript에서 사용)
		// ─────────────────────────────────────────────
		static float GetMoveSpeedMultiplier();   // 이동속도 배율 (1.0 = 기본, 1.1 = 10% 증가)
		static float GetAtkSpeedMultiplier();    // 공격속도 배율 (1.0 = 기본, 1.1 = 10% 증가)
		static float GetAtkDmgMultiplier();		 // 공격력 배율

		// ─────────────────────────────────────────────
		// 버프 상태 조회 (UI, 디버그용)
		// ─────────────────────────────────────────────
		static bool IsDashBuffActive();
		static float GetDashBuffTimer();
		
		static bool IsExecutionBuffActive();
		static float GetExecutionBuffTimer();
		static int GetExecutionBuffStacks();

		// ─────────────────────────────────────────────
		// 대시 버프 설정 Setter/Getter
		// ─────────────────────────────────────────────
		static void SetDashBuffDuration(float value);
		static float GetDashBuffDuration();

		static void SetDashBuffMoveSpeedBonus(float value);
		static float GetDashBuffMoveSpeedBonus();

		// 공격력 버프
		static void  SetDashAtkDmgBuffDuration(float value);
		static float GetDashAtkDmgBuffDuration();

		static void  SetDashAtkDmgBuffBonus(float value);
		static float GetDashAtkDmgBuffBonus();

		// ─────────────────────────────────────────────
		// 처형 버프 설정 Setter/Getter
		// ─────────────────────────────────────────────
		static void SetExecutionBuffDuration(float value);
		static float GetExecutionBuffDuration();

		static void SetExecutionBuffAtkSpeedBonus(float value);
		static float GetExecutionBuffAtkSpeedBonus();

		static void SetExecutionBuffMaxStacks(int value);
		static int GetExecutionBuffMaxStacks();

		// ─────────────────────────────────────────────
		// 가상 함수 콜백 (확장용)
		// - PlayerControllerScript 파생 클래스에서 오버라이딩 가능하도록
		// - BuffManager는 콜백 포인터를 저장하고 호출
		// ─────────────────────────────────────────────
		using BuffCallback = void(*)(int stacksOrZero);  // 스택 수 전달 (처형 버프용)
		
		static void RegisterDashBuffAppliedCallback(BuffCallback callback);
		static void RegisterDashBuffRemovedCallback(BuffCallback callback);
		static void RegisterExecutionBuffAppliedCallback(BuffCallback callback);
		static void RegisterExecutionBuffRemovedCallback(BuffCallback callback);
	};
}
