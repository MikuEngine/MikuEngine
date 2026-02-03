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
    // 
    // 참고:
    //   - launchAngle, ownGravity는 Parabolic 타입에서만 사용
    //   - 다른 타입에서는 이 파라미터들을 명시적으로 무시함
    // ═══════════════════════════════════════════════════════════════
    class IBulletMovement
    {
    public:
        virtual ~IBulletMovement() = default;

        // 초기화 (발사 방향, 속도 설정)
        virtual void Initialize(engine::GameObject* owner, const engine::Vector3& direction, float speed) = 0;

        // 매 프레임 이동 업데이트
        // - transform: 위치 업데이트용 (일부 타입만 사용)
        // - deltaTime: 프레임 간격
        virtual void Update(engine::Transform* transform, float deltaTime) = 0;

        // 현재 속도 벡터 반환 (Rigidbody 초기 속도 설정용)
        virtual engine::Vector3 GetVelocity() const = 0;

        // PhysX 물리를 사용하는지 여부 (Parabolic 등)
        // - true: BulletMonster에서 Transform 직접 조작 안 함
        // - false: Update()에서 Transform 직접 조작
        virtual bool UsesPhysics() const { return false; }
    };


    // ═══════════════════════════════════════════════════════════════
    // 플레이어용 BulletMovement 별도 설정
    // 
    // 가장 기본적인 총알 이동: 일정 방향으로 일정 속도로 직진
    // 
    // 참고: launchAngle, ownGravity는 사용하지 않음 (직선 이동)
    // ═══════════════════════════════════════════════════════════════
    class BulletPlayerMovement : public IBulletMovement
    {
    private:
        engine::Vector3 m_velocity = engine::Vector3::Zero;

    public:
        BulletPlayerMovement() = default;

        // launchAngle, ownGravity: 이 타입에서는 무시됨
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

        bool UsesPhysics() const override { return false; }
    };


    // ═══════════════════════════════════════════════════════════════
    // LinearMovement - 직선 이동 (몬스터용)
    // 
    // 가장 기본적인 총알 이동: 일정 방향으로 일정 속도로 직진
    // 
    // 참고: launchAngle, ownGravity는 사용하지 않음 (직선 이동)
    // ═══════════════════════════════════════════════════════════════
    class LinearMovement : public IBulletMovement
    {
    private:
        engine::Vector3 m_velocity = engine::Vector3::Zero;

    public:
        LinearMovement() = default;

        // launchAngle, ownGravity: 이 타입에서는 무시됨
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

        bool UsesPhysics() const override { return false; }
    };

    // ═══════════════════════════════════════════════════════════════
    // ParabolicMovement - 포물선 이동
    // 
    // 포물선 궤적을 따라 이동하는 총알
    // - PhysX Rigidbody의 AddForce()를 사용하여 중력 적용
    // - Transform 직접 조작 없음 (PhysX가 위치 업데이트)
    // - launchAngle: 발사 상방 각도 (0~89도)
    // - ownGravity: 자체 중력 가속도 (PhysX 글로벌 중력과 별개)
    // ═══════════════════════════════════════════════════════════════

    class ParabolicMovement : public IBulletMovement
    {
    private:
        engine::Rigidbody* m_rigidbody = nullptr;
        engine::Vector3 m_velocity = engine::Vector3::Zero;
        float m_ownGravity = 9.81f;
        float m_launchAngle = 45.0f;  // 도(degree) 단위

    public:
        // ownGravity: 자체 중력 가속도
        // launchAngle: 발사 상방 각도 (0~89도)
        ParabolicMovement(float ownGravity, float launchAngle) 
            : m_ownGravity(ownGravity)
            , m_launchAngle(launchAngle) 
        {}

        void Initialize(engine::GameObject* owner, const engine::Vector3& direction, float speed) override
        {
            if (owner)
            {
                m_rigidbody = owner->GetComponent<engine::Rigidbody>();
            }

            // ─────────────────────────────────────────────
            // launchAngle 적용하여 초기 속도 계산
            // direction은 XZ 평면의 수평 발사 방향으로 가정
            // ─────────────────────────────────────────────
            
            // 수평 방향 (XZ 평면)
            engine::Vector3 horizontalDir = direction;
            horizontalDir.y = 0.0f;
            if (horizontalDir.LengthSquared() > 0.0001f)
            {
                horizontalDir.Normalize();
            }
            else
            {
                // direction이 순수 Y축인 경우 (드문 케이스)
                horizontalDir = engine::Vector3::UnitZ;
            }

            // 라디안 변환
            constexpr float kDegToRad = 3.14159265f / 180.0f;
            float angleRad = m_launchAngle * kDegToRad;

            // 초기 속도 벡터 계산
            // v = speed * (horizontalDir * cos(angle) + up * sin(angle))
            float cosAngle = std::cos(angleRad);
            float sinAngle = std::sin(angleRad);

            m_velocity = horizontalDir * (speed * cosAngle);
            m_velocity.y = speed * sinAngle;
        }

        void Update(engine::Transform* transform, float deltaTime) override
        {
            // ─────────────────────────────────────────────
            // PhysX AddForce로 자체 중력 적용
            // Transform 직접 조작 안 함 (PhysX가 위치 처리)
            // ─────────────────────────────────────────────
            if (m_rigidbody)
            {
                // 중력 힘 적용 (ForceMode::Acceleration = 질량 무시)
                engine::Vector3 gravityForce(0.0f, -m_ownGravity, 0.0f);
                m_rigidbody->AddForce(gravityForce, engine::ForceMode::Acceleration);

                // 현재 속도 동기화 (GetVelocity 정확도용)
                m_velocity = m_rigidbody->GetLinearVelocity();
            }
        }

        engine::Vector3 GetVelocity() const override
        {
            return m_velocity;
        }

        bool UsesPhysics() const override { return true; }

        // ─────────────────────────────────────────────
        // 포물선 궤적 미리보기용 (에디터)
        // ─────────────────────────────────────────────
        float GetOwnGravity() const { return m_ownGravity; }
        float GetLaunchAngle() const { return m_launchAngle; }
    };

    // ═══════════════════════════════════════════════════════════════
    // CurvedMovement - 곡선 이동
    // 
    // Y축 회전을 통해 곡선 궤적으로 이동
    // 
    // 참고: launchAngle, ownGravity는 사용하지 않음 (곡선 이동)
    // ═══════════════════════════════════════════════════════════════

    class CurvedMovement : public IBulletMovement
    {
    private:
        engine::Rigidbody* m_rigidbody = nullptr;
        engine::Vector3 m_velocity = engine::Vector3::Zero;
        float m_curveSpeed = 0.0f;

    public:
        // launchAngle, ownGravity: 이 타입에서는 무시됨
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
            }
        }

        engine::Vector3 GetVelocity() const override
        {
            return m_velocity;
        }

        bool UsesPhysics() const override { return false; }
    };


    // ═══════════════════════════════════════════════════════════════
    // 추후 구현 예정:
    // - SpiralMovement: 나선 궤도
    // - HomingMovement: 유도 미사일
    // ═══════════════════════════════════════════════════════════════
}
