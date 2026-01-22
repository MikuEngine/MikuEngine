#include "GamePCH.h"
#include "Rotator.h"

#include "TestScript.h"
#include <Framework/Object/Component/Rigidbody.h>

namespace game
{
    void Rotator::Awake()
    {
        auto gameObject = CreateGameObject("TestScript Awake");
        gameObject->AddComponent<TestScript>();

        //gameObject->Destroy();
    }

    void Rotator::Start()
    {
        LOG_PRINT("Rotator Start");

        // Rigidbody가 Dynamic인 경우 중력 비활성화
        if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
        {
            if (rb->IsDynamic())
            {
                rb->SetUseGravity(false);
                // 회전 외의 이동 제한 (선택적)
                // rb->SetConstraints(engine::RigidbodyConstraints::FreezePosition);
            }
        }
    }

    void Rotator::Update()
    {
        // 초당 회전 각도 (degree/s)
        float degreesPerSecond = 90.0f * m_speed;
        
        // 이번 프레임의 회전량 (degree)
        float rotationAmount = degreesPerSecond * engine::Time::DeltaTime();

        // Rigidbody가 있는지 확인
        auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>();
        
        if (rb && rb->IsDynamic())
        {
            // Dynamic: Angular Velocity로 회전 (PhysX 통해)            
            // 방향 통일을 위해 부호 반전
            float angularSpeed = -(degreesPerSecond / 10.0f) * DirectX::XM_PI / 180.0f;  // degree/s → rad/s, 1/10 속도
            rb->SetAngularVelocity(engine::Vector3(0.0f, 0.0f, angularSpeed));
        }
        else if (rb && rb->IsKinematic())
        {
            // Kinematic: Transform 직접 회전 (PhysX가 자동 동기화)
            GetTransform()->Rotate(engine::Vector3::UnitZ, rotationAmount);
        }
        else
        {
            // Rigidbody 없음 (Static 포함): Transform 직접 회전
            GetTransform()->Rotate(engine::Vector3::UnitZ, rotationAmount);
        }

        if (engine::Input::IsKeyPressed(engine::Keys::G))
        {
            auto gameObject = CreateGameObject("TestScript Update");
            gameObject->AddComponent<TestScript>();

            //gameObject->Destroy();
        }
    }

    void Rotator::OnGui()
    {
        ImGui::InputFloat("Rotate Speed", &m_speed);
    }

    void Rotator::Save(engine::json& j) const
    {
        Object::Save(j);

        j["Speed"] = m_speed;
    }

    void Rotator::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "Speed", m_speed);
    }
}