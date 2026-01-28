#pragma once

namespace game
{
    class BossScript;

    class BossPatternBase
    {
    protected:
        bool m_isActive = false;
        float m_elapsedTime = 0.0f;
        float m_intervalTimer = 0.0f;  // 간격 타이머

    public:
        virtual ~BossPatternBase() = default;

        virtual void Start(BossScript* boss) = 0;  // 패턴 시작
        virtual void Update(BossScript* boss, float deltaTime) = 0;  // 패턴 업데이트
        virtual void End(BossScript* boss) = 0;  // 패턴 종료

        bool IsActive() const { return m_isActive; }  // 패턴 활성화 여부
        virtual bool IsFinished() const = 0;  // 패턴 완료 여부
        virtual float GetInterval() const = 0;  // 패턴 실행 간격 (N초마다)
        virtual float GetCooldown() const = 0;  // 쿨다운 시간 (선택)

        virtual std::string GetPatternName() const = 0;
        virtual std::string GetPatternDescription() const = 0;
    };
}
