#include "GamePCH.h"
#include "Script/CharacterScript/Monster/BulletMonster.h"
#include "Script/CharacterScript/Common/BulletFactory.h"
#include "Script/CharacterScript/Common/BulletMovement.h"
#include "Script/CharacterScript/Player/PlayerControllerScript.h"

#include <Framework/Object/GameObject/GameObject.h>
#include <Framework/Object/Component/Rigidbody.h>
#include <Framework/Object/Component/Collider.h>

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>

#include <Framework/Object/Component/Renderer/DebugRenderer.h>

namespace game
{
    // ═══════════════════════════════════════════════════════════════
    // 초기화 (Factory에서 호출)
    // ═══════════════════════════════════════════════════════════════
    void BulletMonster::Setup(std::unique_ptr<IBulletMovement> movement, const BulletParams& params, BulletFactory* factory)
    {
        m_movement = std::move(movement);
        m_lifetime = params.lifetime;
		m_params = params;
        m_cachedFactory = factory;
    }

    void BulletMonster::SetupField(float radius, const BulletParams& params)
    {
        m_isFieldType = true;
        m_radius = radius;
        m_lifetime = params.lifetime;
        m_params = params;

        auto* collider = GetGameObject()->GetComponent<engine::Collider>();
        if (collider)
        {
            collider->SetLayer(engine::PhysicsLayer::Field);
            collider->SetIsTrigger(true);
        }
     
        auto* scene = engine::SceneManager::Get().GetScene();
        if (scene)
        {
            m_targetPlayer = scene->FindGameObject("Player");
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════
    void BulletMonster::Start()
    {
        LOG_PRINT("[BulletMonster::Start] Called! Type: {}", static_cast<int>(m_params.type));
        
        m_elapsedTime = 0.0f;

        auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>();
        if (!rb)
        {
            LOG_PRINT("[BulletMonster::Start] ERROR: Rigidbody not found!");
            return;
        }
        
        LOG_PRINT("[BulletMonster::Start] Rigidbody found. IsKinematic: {}", rb->IsKinematic());
        
        // ═══════════════════════════════════════════════════════════════
        // [DEBUG] 포괄적인 PhysX 상태 진단
        // ═══════════════════════════════════════════════════════════════
        {
            LOG_PRINT("═══════════════════════════════════════════════════════════════");
            LOG_PRINT("[DEBUG] ===== PhysX Actor/Shape 상태 진단 시작 =====");
            
            // 1. PxActor 존재 여부
            physx::PxRigidActor* actor = rb->GetPxActor();
            LOG_PRINT("[DEBUG] 1. PxActor 포인터: {}", actor ? "유효함" : "NULL!");
            
            if (actor)
            {
                // 2. PxScene 등록 여부 (핵심!)
                physx::PxScene* scene = actor->getScene();
                LOG_PRINT("[DEBUG] 2. PxScene 등록 여부: {}", scene ? "등록됨" : "미등록! (시뮬레이션 불가)");
                
                // 3. Shape 개수 (핵심! 0이면 시뮬레이션 안됨)
                physx::PxU32 shapeCount = actor->getNbShapes();
                LOG_PRINT("[DEBUG] 3. Shape 개수: {} {}", shapeCount, 
                    shapeCount == 0 ? "(경고: Shape 없음! 시뮬레이션 불가)" : "(OK)");
                
                // 4. Actor 타입
                physx::PxRigidDynamic* dynamic = actor->is<physx::PxRigidDynamic>();
                if (dynamic)
                {
                    LOG_PRINT("[DEBUG] 4. Actor 타입: PxRigidDynamic (OK)");
                    
                    // 5. Kinematic 플래그
                    bool isKinematic = dynamic->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC;
                    LOG_PRINT("[DEBUG] 5. Kinematic 플래그: {} {}", 
                        isKinematic, isKinematic ? "(경고: 속도로 이동 안됨)" : "(OK)");
                    
                    // 6. Sleep 상태
                    bool isSleeping = dynamic->isSleeping();
                    LOG_PRINT("[DEBUG] 6. Sleep 상태: {} {}", 
                        isSleeping, isSleeping ? "(경고: 깨워야 함)" : "(깨어있음)");
                    
                    // 7. 중력 비활성화 여부
                    bool gravityDisabled = actor->getActorFlags() & physx::PxActorFlag::eDISABLE_GRAVITY;
                    LOG_PRINT("[DEBUG] 7. 중력 비활성화: {} (자체 중력 사용 예정이므로 OK)", gravityDisabled);
                    
                    // 8. Mass
                    LOG_PRINT("[DEBUG] 8. Mass: {:.2f}", dynamic->getMass());
                    
                    // 9. LinearDamping
                    LOG_PRINT("[DEBUG] 9. LinearDamping: {:.2f}", dynamic->getLinearDamping());
                }
                else
                {
                    LOG_PRINT("[DEBUG] 4. Actor 타입: PxRigidStatic (경고: Dynamic이어야 함!)");
                }
                
                // 10. Shape 상세 정보
                if (shapeCount > 0)
                {
                    physx::PxShape* shapes[8];
                    physx::PxU32 retrieved = actor->getShapes(shapes, 8);
                    for (physx::PxU32 i = 0; i < retrieved; ++i)
                    {
                        physx::PxShapeFlags flags = shapes[i]->getFlags();
                        bool isSim = flags.isSet(physx::PxShapeFlag::eSIMULATION_SHAPE);
                        bool isTrigger = flags.isSet(physx::PxShapeFlag::eTRIGGER_SHAPE);
                        bool isQuery = flags.isSet(physx::PxShapeFlag::eSCENE_QUERY_SHAPE);
                        LOG_PRINT("[DEBUG] 10. Shape[{}] 플래그: SIMULATION={}, TRIGGER={}, QUERY={}",
                            i, isSim, isTrigger, isQuery);
                    }
                }
            }
            
            // 11. Collider 연결 상태
            auto* collider = GetGameObject()->GetComponent<engine::Collider>();
            if (collider)
            {
                LOG_PRINT("[DEBUG] 11. Collider 존재: 예");
                LOG_PRINT("[DEBUG] 12. Collider->HasRigidbody(): {}", collider->HasRigidbody());
                LOG_PRINT("[DEBUG] 13. Collider->IsTrigger(): {}", collider->IsTrigger());
                LOG_PRINT("[DEBUG] 14. Collider->GetPxShape(): {}", collider->GetPxShape() ? "유효함" : "NULL!");
                
                // Shape가 Actor에 실제로 연결되어 있는지 확인
                if (collider->GetPxShape() && actor)
                {
                    physx::PxRigidActor* shapeActor = collider->GetPxShape()->getActor();
                    bool attachedToCorrectActor = (shapeActor == actor);
                    LOG_PRINT("[DEBUG] 15. Shape가 올바른 Actor에 연결됨: {} {}", 
                        attachedToCorrectActor,
                        attachedToCorrectActor ? "(OK)" : "(경고: 다른 Actor에 연결됨!)");
                    
                    if (!attachedToCorrectActor && shapeActor)
                    {
                        // Shape가 어디에 연결되어 있는지
                        physx::PxRigidStatic* staticActor = shapeActor->is<physx::PxRigidStatic>();
                        LOG_PRINT("[DEBUG]    -> Shape가 연결된 Actor 타입: {}", 
                            staticActor ? "PxRigidStatic (독립 Static Actor)" : "다른 Dynamic Actor");
                    }
                }
            }
            else
            {
                LOG_PRINT("[DEBUG] 11. Collider 존재: 아니오 (경고!)");
            }
            
            LOG_PRINT("[DEBUG] ===== PhysX 상태 진단 완료 =====");
            LOG_PRINT("═══════════════════════════════════════════════════════════════");
        }
        // ═══════════════════════════════════════════════════════════════
        
        // ─────────────────────────────────────────────
        // Rigidbody 깨우기 (Sleep 상태 해제)
        // PhysX는 움직임이 없는 물체를 Sleep 상태로 전환함
        // ─────────────────────────────────────────────
        rb->WakeUp();
        
        // ─────────────────────────────────────────────
        // Parabolic/Field 타입: 자체 중력 사용 (PhysX 글로벌 중력 OFF)
        // ─────────────────────────────────────────────
        if (m_params.type == BulletType::Parabolic || m_params.type == BulletType::Field)
        {
            rb->SetUseGravity(false);
            LOG_PRINT("[BulletMonster::Start] SetUseGravity(false)");
            
            // ═══════════════════════════════════════════════════════════════
            // [핵심 수정] 포물선 운동을 위해 LinearDamping을 0으로 설정
            // LinearDamping > 0이면 속도가 지수적으로 감소하여 포물선이 짧아짐
            // ═══════════════════════════════════════════════════════════════
            rb->SetLinearDamping(0.0f);
            rb->SetAngularDamping(0.0f);
            LOG_PRINT("[BulletMonster::Start] Set LinearDamping=0, AngularDamping=0 for parabolic motion");
        }

        // Rigidbody에 초기 속도 설정
        if (!m_movement)
        {
            LOG_PRINT("[BulletMonster::Start] ERROR: m_movement is null!");
            return;
        }
        
        engine::Vector3 initVel = m_movement->GetVelocity();
        LOG_PRINT("[BulletMonster::Start] Initial velocity: ({:.2f}, {:.2f}, {:.2f}), magnitude: {:.2f}",
            initVel.x, initVel.y, initVel.z, initVel.Length());
        
        rb->SetLinearVelocity(initVel);
        
        // 다시 한번 깨우기 (SetLinearVelocity 후에도 Sleep될 수 있음)
        rb->WakeUp();
        
        // 디버그: 실제 적용된 속도 확인
        engine::Vector3 actualVel = rb->GetLinearVelocity();
        LOG_PRINT("[BulletMonster::Start] Actual velocity after set: ({:.2f}, {:.2f}, {:.2f}), magnitude: {:.2f}",
            actualVel.x, actualVel.y, actualVel.z, actualVel.Length());
    }

    void BulletMonster::Update()
    {
        // 죽는 중이면 타이머만 체크
        if (m_isDying)
        {
            m_deathTimer += engine::Time::DeltaTime();
            if (m_deathTimer >= m_deathDelay)
            {
                GetGameObject()->Destroy();
            }
            return;
        }

        // 생존 시간 누적
        float dt = engine::Time::DeltaTime();
        m_elapsedTime += dt;

        // ─────────────────────────────────────────────
        // Movement 업데이트
        // - UsesPhysics() == true: AddForce 적용 (ParabolicMovement 등)
        // - UsesPhysics() == false: Transform 직접 조작
        // ─────────────────────────────────────────────
        if (m_movement && !m_isFieldType)
        {
            m_movement->Update(GetTransform(), dt);
        }

        // 수명 체크
        if (m_elapsedTime >= m_lifetime)
        {
            GetGameObject()->Destroy();
            return;
        }

        // 장판형 총알일 경우 주기적으로 데미지 적용
        if (m_isFieldType)
        {

#ifdef _DEBUG  // 장판 범위 디버그 렌더링
            engine::DebugRenderer::Get().AddDebugCircle(
                GetTransform()->GetWorldPosition() + engine::Vector3(0, 0.05f, 0),
                m_radius,
                engine::Vector3::UnitY,
                DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 1.0f),
                32
            );
#endif

            m_tickTimer += engine::Time::DeltaTime();

            if (m_tickTimer >= m_tickInterval)
            {
                m_tickTimer = 0.0f;

                if (m_targetPlayer)
                {
                    float distance = engine::Vector3::Distance(GetTransform()->GetWorldPosition(), m_targetPlayer->GetTransform()->GetWorldPosition());

                    if (distance <= m_params.radius)
                    {
                        if (auto* playerScript = m_targetPlayer->GetComponent<PlayerControllerScript>())
                        {
                            // playerScript->OnHit(m_params.damage);
                        }
                    }
                }  
            }
            return;
        }

        // ─────────────────────────────────────────────
        // 소멸 체크
        // ─────────────────────────────────────────────
        engine::Vector3 pos = GetTransform()->GetWorldPosition();

        // Y좌표 기반 소멸 (바닥 아래로 떨어지면 제거)
        // [DEBUG] 테스트를 위해 비활성화 - 로그만 출력
        constexpr float kGroundY = -1.0f;
        if (pos.y < kGroundY)
        {
            LOG_PRINT("[BulletMonster] Y좌표 조건 도달: pos.y={:.2f} < {:.2f}", pos.y, kGroundY);
            // GetGameObject()->Destroy();
            // return;
        }

        // 화면 밖 체크 (XZ 범위)
        float boundary = 50.0f;
        if (std::abs(pos.x) > boundary || std::abs(pos.z) > boundary)
        {
            GetGameObject()->Destroy();
            return;
        }
    }

    void BulletMonster::FixedUpdate()
    {
        // ─────────────────────────────────────────────
        // Movement의 FixedUpdate 호출 (물리 연산)
        // PhysX simulate()와 동기화됨
        // ─────────────────────────────────────────────
        if (m_movement)
        {
            m_movement->FixedUpdate();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 충돌 콜백
    // ═══════════════════════════════════════════════════════════════
    void BulletMonster::OnTriggerEnter(const engine::CollisionInfo& info)
    {
        if (m_isDying || m_isFieldType) return;
        if (!info.gameObject) return;

        bool isPlayer = (info.gameObject->GetComponent<PlayerControllerScript>() != nullptr);
        bool isEnvironment = (info.gameObject->GetComponent<engine::Collider>()->GetLayer() == engine::PhysicsLayer::Environment);

        // 플레이어와 충돌했는지 확인
        if (isPlayer || isEnvironment)
        {
            // TODO: 플레이어 OnHit() 구현 시 호출
            // player->OnHit(m_damage);

            if (m_params.type == BulletType::Field && m_cachedFactory)
            {
                m_cachedFactory->FieldFireMonster(GetTransform()->GetWorldPosition(), m_params);
            }

            // dying 상태로 전환
            m_isDying = true;
            m_deathTimer = 0.0f;

            // 속도 정지
            if (auto* rb = GetGameObject()->GetComponent<engine::Rigidbody>())
            {
                rb->SetLinearVelocity(engine::Vector3::Zero);
            }
        }
    }
}
