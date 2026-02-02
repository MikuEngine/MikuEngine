#pragma once

#include <Framework/Object/Component/Transform.h>
#include <Framework/Object/Component/Rigidbody.h>
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
        // virtual void Initialize(const engine::Vector3& direction, float speed) = 0;
        virtual void Initialize(engine::GameObject* owner, const engine::Vector3& direction, float speed) = 0;

        // 매 프레임 이동 업데이트
        virtual void Update(engine::Transform* transform, float deltaTime) = 0;

        // 현재 속도 벡터 반환 (Rigidbody 연동용)
        virtual engine::Vector3 GetVelocity() const = 0;
    };


    // ═══════════════════════════════════════════════════════════════
    // 플레이어용 BulletMovement 별도 설정
    // 
    // 가장 기본적인 총알 이동: 일정 방향으로 일정 속도로 직진
    // ═══════════════════════════════════════════════════════════════
    class BulletPlayerMovement : public IBulletMovement
    {
    private:
        engine::Vector3 m_velocity = engine::Vector3::Zero;

    public:
        BulletPlayerMovement() = default;

        void Initialize(engine::GameObject* owner, const engine::Vector3& direction, float speed) override
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

        void Initialize(engine::GameObject* owner, const engine::Vector3& direction, float speed) override
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
	// ParabolicMovement - 포물선 이동
    // 
    //  포물선 궤적을 따라 이동하는 총알
    // ═══════════════════════════════════════════════════════════════

    class ParabolicMovement : public IBulletMovement
    {
    private:
        engine::Vector3 m_velocity = engine::Vector3::Zero;
		float m_gravity = 9.81f;

    public:
        ParabolicMovement(float gravity) : m_gravity(gravity) {}

        void Initialize(engine::GameObject* owner, const engine::Vector3& direction, float speed) override
        {
            m_velocity = direction * speed;
        }

        void Update(engine::Transform* transform, float deltaTime) override
        {
            m_velocity.y -= m_gravity * deltaTime;

            engine::Vector3 currentPos = transform->GetLocalPosition();
            currentPos += m_velocity * deltaTime;

            transform->SetLocalPosition(currentPos);

            // 진행 방향을 바라보도록 회전 업데이트
            if (m_velocity.LengthSquared() > 0.001f)
            {
                // 속도 벡터 방향으로 정렬 로직 추가 가능
            }
        }

        engine::Vector3 GetVelocity() const override
        {
            return m_velocity;
        }
	};

    // ═══════════════════════════════════════════════════════════════
    // CurvedMovement - 
    // 
    //  
    // ═══════════════════════════════════════════════════════════════

    class CurvedMovement : public IBulletMovement
    {
    private:
        engine::Rigidbody* m_rigidbody = nullptr;
        engine::Vector3 m_velocity = engine::Vector3::Zero;
        float m_curveSpeed = 0.0f;

    public:
        CurvedMovement(float curveSpeed) : m_curveSpeed(curveSpeed) {}

        void Initialize(engine::GameObject* owner, const engine::Vector3& direction, float speed) override
        {
            if (owner) m_rigidbody = owner->GetComponent<engine::Rigidbody>();

            m_velocity = direction;
            m_velocity.Normalize();
            m_velocity *= speed;
        }

        void Update(engine::Transform* transform, float deltaTime) override
        {
            if (transform)
            {
                float angle = m_curveSpeed * deltaTime;
                auto rot = DirectX::SimpleMath::Matrix::CreateRotationY(angle);
                m_velocity = DirectX::SimpleMath::Vector3::Transform(m_velocity, rot);

                if (m_rigidbody)
                {
                    m_rigidbody->SetLinearVelocity(m_velocity);
                }
                else if (transform)
                {
                    engine::Vector3 pos = transform->GetLocalPosition();
                    pos += m_velocity * deltaTime;
                    transform->SetLocalPosition(pos);
                }

                /* 진행 방향(속도 벡터)을 바라보도록 회전
                if (m_velocity.LengthSquared() > 0.001f) {
                    transform->SetForward(m_velocity.Normalized()); 
                }*/
            }
        }

        engine::Vector3 GetVelocity() const override
        {
            return m_velocity;
        }
    };


    // ═══════════════════════════════════════════════════════════════
    // 추후 구현 예정:
    // - SpiralMovement: 나선 궤도
    // - HomingMovement: 유도 미사일
    // ═══════════════════════════════════════════════════════════════
}
