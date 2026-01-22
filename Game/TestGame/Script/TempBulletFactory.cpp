#include "GamePCH.h"
#include "TempBulletFactory.h"
#include "TempBullet.h"

#include <Framework/Object/Component/Renderer/StaticMeshRenderer.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/SphereCollider.h>
#include <Framework/Physics/PhysicsLayer.h>

namespace game
{
    void TempBulletFactory::Start()
    {
        LOG_PRINT("[TempBulletFactory] Started");
        m_fireCooldown = 0.0f;  // 시작하자마자 발사 가능
    }

    void TempBulletFactory::Update()
    {
        // 쿨다운 감소
        if (m_fireCooldown > 0.0f)
        {
            m_fireCooldown -= engine::Time::DeltaTime();
        }
    }

    bool TempBulletFactory::CanFire() const
    {
        return m_fireCooldown <= 0.0f;
    }

    void TempBulletFactory::Fire(const engine::Vector3& position, const engine::Vector3& direction)
    {
        if (!CanFire())
        {
            return;
        }

        m_fireCooldown = m_fireRate;  // 쿨다운 리셋

        // 총알 GameObject 생성
        auto* bulletGO = CreateGameObject("Bullet");
        bulletGO->GetTransform()->SetLocalPosition(position);
        bulletGO->GetTransform()->SetLocalScale(engine::Vector3(0.2f, 0.2f, 0.2f));

        // StaticMeshRenderer 추가 (Sphere)
        auto* renderer = bulletGO->AddComponent<engine::StaticMeshRenderer>();
        renderer->SetMesh("Resource/Model/Sphere.fbx");
        renderer->SetVertexShader("Resource/Shader/Vertex/Static_VS.hlsl");
        renderer->SetOpaquePixelShader("Resource/Shader/Pixel/GBuffer_PS.hlsl");
        
        // Rigidbody 추가 (Dynamic)
        auto* rb = bulletGO->AddComponent<engine::Rigidbody>();
        rb->SetRigidbodyType(engine::RigidbodyType::Dynamic);
        rb->SetUseGravity(false);
        rb->SetLinearDamping(0.0f);  // 총알은 감속 없음

        // SphereCollider 추가 (Trigger)
        auto* collider = bulletGO->AddComponent<engine::SphereCollider>();
        collider->SetIsTrigger(true);
        collider->SetRadius(0.1f);
        
        // 레이어 설정: Projectile 레이어, Enemy만 충돌
        collider->SetLayer(engine::PhysicsLayer::Projectile);
        collider->SetCollisionMask(engine::PhysicsLayer::EnemyMask);

        // TempBullet 스크립트 추가
        auto* bullet = bulletGO->AddComponent<TempBullet>();
        bullet->Initialize(direction, m_bulletSpeed, m_bulletLifetime);

        LOG_PRINT("[TempBulletFactory] Fired bullet at ({:.2f}, {:.2f}, {:.2f})",
            position.x, position.y, position.z);
    }

    void TempBulletFactory::OnGui()
    {
        ImGui::DragFloat("Bullet Speed", &m_bulletSpeed, 0.5f, 1.0f, 100.0f);
        ImGui::DragFloat("Fire Rate (sec)", &m_fireRate, 0.01f, 0.01f, 2.0f);
        ImGui::DragFloat("Bullet Lifetime", &m_bulletLifetime, 0.1f, 0.5f, 10.0f);
    }

    void TempBulletFactory::Save(engine::json& j) const
    {
        Object::Save(j);
        j["BulletSpeed"] = m_bulletSpeed;
        j["FireRate"] = m_fireRate;
        j["BulletLifetime"] = m_bulletLifetime;
    }

    void TempBulletFactory::Load(const engine::json& j)
    {
        Object::Load(j);
        engine::JsonGet(j, "BulletSpeed", m_bulletSpeed);
        engine::JsonGet(j, "FireRate", m_fireRate);
        engine::JsonGet(j, "BulletLifetime", m_bulletLifetime);
    }
}
