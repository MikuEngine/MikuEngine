#pragma once

namespace game
{
    class BossScript;

    // ═══════════════════════════════════════════════════════════════
    // BossPatternBase - 보스 패턴 베이스 클래스
    // 
    // 목적:
    //   - 모든 보스 패턴의 공통 인터페이스 제공
    //   - 패턴 실행, 상태 관리, 정보 조회 기능
    // 
    // 사용법:
    //   1. 이 클래스를 상속받아서 패턴 클래스 생성
    //   2. 순수 가상 함수들을 구현
    //   3. BossPatternManager에 등록하여 사용
    // ═══════════════════════════════════════════════════════════════
    class BossPatternBase
    {
    protected:
        bool m_isActive = false;
        float m_elapsedTime = 0.0f;
        float m_intervalTimer = 0.0f;  // 간격 타이머

    public:
        virtual ~BossPatternBase() = default;

        // ─────────────────────────────────────────────
        // 패턴 실행
        // ─────────────────────────────────────────────
        virtual void Start(BossScript* boss) = 0;  // 패턴 시작
        virtual void Update(BossScript* boss, float deltaTime) = 0;  // 패턴 업데이트
        virtual void End(BossScript* boss) = 0;  // 패턴 종료

        // ─────────────────────────────────────────────
        // 패턴 상태
        // ─────────────────────────────────────────────
        virtual bool IsFinished() const = 0;  // 패턴 완료 여부
        virtual float GetInterval() const = 0;  // 패턴 실행 간격 (N초마다)
        virtual float GetCooldown() const = 0;  // 쿨다운 시간 (선택)

        // ─────────────────────────────────────────────
        // 패턴 정보
        // ─────────────────────────────────────────────
        virtual std::string GetPatternName() const = 0;
        virtual std::string GetPatternDescription() const = 0;
    };
}
