#include "GamePCH.h"
#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/CharacterScript/Player/BulletPlayer.h"
#include "Script/CharacterScript/Monster/BulletMonster.h"

#include <Framework/Object/Component/Renderer/StaticMeshRenderer.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/SphereCollider.h>
#include <Framework/Physics/PhysicsLayer.h>
#include <Framework/Asset/Prefab.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 총알 발사
    // ═══════════════════════════════════════════════════════════════
    void BulletFactory::Fire(const engine::Vector3& position,
                              const engine::Vector3& direction,
                              const BulletParams& params)
    {
        //// ─────────────────────────────────────────────
        //// 1. GameObject 생성
        //// ─────────────────────────────────────────────
        //auto* bulletGO = CreateGameObject("Bullet");
        //bulletGO->GetTransform()->SetLocalPosition(position);
        //bulletGO->GetTransform()->SetLocalScale(engine::Vector3(1.2f, 1.2f, 1.2f));

        //// ─────────────────────────────────────────────
        //// 2. StaticMeshRenderer 추가
        //// ─────────────────────────────────────────────
        //auto* renderer = bulletGO->AddComponent<engine::StaticMeshRenderer>();
        //renderer->SetMesh("Resource/Model/Sphere.fbx");
        //renderer->SetVertexShader("Resource/Shader/Vertex/Static_VS.hlsl");
        //renderer->SetOpaquePixelShader("Resource/Shader/Pixel/GBuffer_PS.hlsl");

        //// ─────────────────────────────────────────────
        //// 3. Rigidbody 추가 (Dynamic)
        //// ─────────────────────────────────────────────
        //auto* rb = bulletGO->AddComponent<engine::Rigidbody>();
        //rb->SetRigidbodyType(engine::RigidbodyType::Dynamic);
        //rb->SetUseGravity(false);
        //rb->SetLinearDamping(0.0f);

        //// ─────────────────────────────────────────────
        //// 4. SphereCollider 추가 (Trigger)
        //// ─────────────────────────────────────────────
        //auto* collider = bulletGO->AddComponent<engine::SphereCollider>();
        //collider->SetIsTrigger(true);
        //collider->SetRadius(1.1f);
        //collider->SetLayer(engine::PhysicsLayer::Projectile);
        //collider->SetCollisionMask(engine::PhysicsLayer::EnemyMask);

        auto go = engine::Prefab::Instantiate("Bullet");
        go->GetTransform()->SetLocalPosition(position);

        // ─────────────────────────────────────────────
        // 5. Movement 생성 및 초기화
        // ─────────────────────────────────────────────
        auto movement = CreateMovement(params);
        movement->Initialize(direction, params.speed);

        // ─────────────────────────────────────────────
        // 6. BulletPlayer 컴포넌트 추가 및 설정
        // ─────────────────────────────────────────────
        auto* bullet = go->GetComponent<BulletPlayer>();
        bullet->Setup(std::move(movement), params.lifetime);
    }

    // ═══════════════════════════════════════════════════════════════
    // 몬스터 직선 총알 발사
    // ═══════════════════════════════════════════════════════════════
    void BulletFactory::LinearFireMonster(const engine::Vector3& position,
                                          const engine::Vector3& direction,
                                          const BulletParams& params)
    {
        //// ─────────────────────────────────────────────
        //// 1. GameObject 생성
        //// ─────────────────────────────────────────────
        //auto* bulletGO = CreateGameObject("BulletMonster");
        //bulletGO->GetTransform()->SetLocalPosition(position);
        //bulletGO->GetTransform()->SetLocalScale(engine::Vector3(1.2f, 1.2f, 1.2f));

        //// ─────────────────────────────────────────────
        //// 2. StaticMeshRenderer 추가
        //// ─────────────────────────────────────────────
        //auto* renderer = bulletGO->AddComponent<engine::StaticMeshRenderer>();
        //renderer->SetMesh("Resource/Model/Sphere.fbx");
        //renderer->SetVertexShader("Resource/Shader/Vertex/Static_VS.hlsl");
        //renderer->SetOpaquePixelShader("Resource/Shader/Pixel/GBuffer_PS.hlsl");

        //// ─────────────────────────────────────────────
        //// 3. Rigidbody 추가 (Dynamic)
        //// ─────────────────────────────────────────────
        //auto* rb = bulletGO->AddComponent<engine::Rigidbody>();
        //rb->SetRigidbodyType(engine::RigidbodyType::Dynamic);
        //rb->SetUseGravity(false);
        //rb->SetLinearDamping(0.0f);

        //// ─────────────────────────────────────────────
        //// 4. SphereCollider 추가 (Trigger, EnemyProjectile 레이어)
        //// ─────────────────────────────────────────────
        //auto* collider = bulletGO->AddComponent<engine::SphereCollider>();
        //collider->SetIsTrigger(true);
        //collider->SetRadius(1.1f);
        //collider->SetLayer(engine::PhysicsLayer::EnemyProjectile);  // 몬스터 총알 레이어
        //
        //// 충돌 마스크: Default, Player, Environment와 충돌
        //uint32_t collisionMask = engine::PhysicsLayer::DefaultMask |
        //                          engine::PhysicsLayer::PlayerMask |
        //                          engine::PhysicsLayer::EnvironmentMask;
        //collider->SetCollisionMask(collisionMask);

        auto go = engine::Prefab::Instantiate("BulletLinearMonster");
        
        if (!go)
        {
            LOG_PRINT("[BulletFactory] ERROR: Failed to instantiate 'BulletLinearMonster' prefab!");
            return;
        }
        
        go->GetTransform()->SetLocalPosition(position);

        // ─────────────────────────────────────────────
        // 5. Movement 생성 및 초기화
        // ─────────────────────────────────────────────
        auto movement = CreateMovement(params);
        movement->Initialize(direction, params.speed);

        // ─────────────────────────────────────────────
        // 6. BulletMonster 컴포넌트 추가 및 설정
        // ─────────────────────────────────────────────
        auto* bullet = go->GetComponent<BulletMonster>();
        
        if (!bullet)
        {
            LOG_PRINT("[BulletFactory] ERROR: 'BulletLinearMonster' prefab missing BulletMonster component!");
            return;
        }
        
        bullet->Setup(std::move(movement), params.lifetime);
        
        LOG_PRINT("[BulletFactory] LinearFireMonster: Bullet spawned successfully at ({:.2f}, {:.2f}, {:.2f})", 
                  position.x, position.y, position.z);
    }

    void BulletFactory::ParabolicFireMonster(const engine::Vector3& position, const engine::Vector3& direction, const BulletParams& params)
    {
        auto go = engine::Prefab::Instantiate("ParabolicFireMonster");
        if (!go)
        {
            LOG_PRINT("[BulletFactory] ERROR: Failed to instantiate 'ParabolicMovement' prefab!");
            return;
        }

        go->GetTransform()->SetLocalPosition(position);

        auto movement = CreateMovement(params);
        movement->Initialize(direction, params.speed);

        auto* bullet = go->GetComponent<BulletMonster>();

        if (!bullet)
        {
            LOG_PRINT("[BulletFactory] ERROR: 'ParabolicFireMonster' prefab missing BulletMonster component!");
            return;
        }

        bullet->Setup(std::move(movement), params.lifetime);
    }

    // ═══════════════════════════════════════════════════════════════
    // Movement 생성 (Strategy 패턴)
    // ═══════════════════════════════════════════════════════════════
    std::unique_ptr<IBulletMovement> BulletFactory::CreateMovement(const BulletParams& params)
    {
        switch (params.type)
        {
        case BulletType::Linear:
            return std::make_unique<LinearMovement>();
        case BulletType::Parabolic:
            return std::make_unique<ParabolicMovement> (params.gravity);
        default:
            return std::make_unique<LinearMovement>();
        // 추후 구현:
        // case BulletType::Spiral:
        //     return std::make_unique<SpiralMovement>(params.spiralRadius, params.spiralFrequency);
        // case BulletType::Homing:
        //     return std::make_unique<HomingMovement>(params.target, params.turnSpeed);
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // GUI / 직렬화
    // ═══════════════════════════════════════════════════════════════
    void BulletFactory::OnGui()
    {
        ImGui::Text("BulletFactory");
        ImGui::Text("Call Fire() with BulletParams to create bullets.");
    }

    void BulletFactory::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void BulletFactory::Load(const engine::json& j)
    {
        Object::Load(j);
    }
}
