#pragma once

#include <Framework/Object/Component/Transform.h>
#include <memory>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // IBulletMovement - 총알 이동 전략 인터페이스
    // 
    // Strategy 패턴: 총알의 궤적/이동 방식을 캡슐화
    // 새로운 이동 방식 추가 시 이 인터페이스를 구현
    // ═══════════════════════════════════════════════════════════════
    class IBulletMovement
    {
    public:
        virtual ~IBulletMovement() = default;

        // 초기화 (발사 방향, 속도 설정)
        virtual void Initialize(const engine::Vector3& direction, float speed) = 0;

        // 매 프레임 이동 업데이트
        virtual void Update(engine::Transform* transform, float deltaTime) = 0;

        // 현재 속도 벡터 반환 (Rigidbody 연동용)
        virtual engine::Vector3 GetVelocity() const = 0;
    };

    // ═══════════════════════════════════════════════════════════════
    // LinearMovement - 직선 이동
    // 
    // 가장 기본적인 총알 이동: 일정 방향으로 일정 속도로 직진
    // ═══════════════════════════════════════════════════════════════
    class LinearMovement : public IBulletMovement
    {
    private:
        engine::Vector3 m_velocity = engine::Vector3::Zero;

    public:
        LinearMovement() = default;

        void Initialize(const engine::Vector3& direction, float speed) override
        {
            m_velocity = direction;
            m_velocity.Normalize();
            m_velocity *= speed;
        }

        void Update(engine::Transform* transform, float deltaTime) override
        {
            if (transform)
            {
                engine::Vector3 pos = transform->GetLocalPosition();
                pos += m_velocity * deltaTime;
                transform->SetLocalPosition(pos);
            }
        }

        engine::Vector3 GetVelocity() const override
        {
            return m_velocity;
        }
    };

    // ═══════════════════════════════════════════════════════════════
    // 추후 구현 예정:
    // - ParabolicMovement: 포물선 (곡사)
    // - SpiralMovement: 나선 궤도
    // - HomingMovement: 유도 미사일
    // ═══════════════════════════════════════════════════════════════
}
