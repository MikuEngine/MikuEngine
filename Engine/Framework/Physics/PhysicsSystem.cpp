#include "EnginePCH.h"
#include "PhysicsSystem.h"

#include "Framework/Physics/PhysicsUtility.h"
#include "Framework/Physics/CollisionSystem.h"
#include "Framework/Object/Component/Rigidbody.h"
#include "Framework/Object/Component/Collider.h"
#include "Framework/Object/Component/CharacterController.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"
#include "Framework/Physics/PhysicsDebugRenderer.h"

namespace engine
{
    // ═══════════════════════════════════════════════════════════════
    // 생명주기
    // ═══════════════════════════════════════════════════════════════

    void PhysicsSystem::Initialize(const PhysicsSettings& settings)
    {
        if (m_isInitialized)
        {
            return;
        }

        m_settings = settings;

        // 1. Foundation 생성
        m_foundation = PxCreateFoundation(
            PX_PHYSICS_VERSION,
            m_allocator,
            m_errorCallback
        );

        if (!m_foundation)
        {
            LOG_ERROR("[PhysicsSystem] Failed to create PxFoundation");
            return;
        }

        // 2. PVD 연결 (디버그용)
        if (m_settings.enablePvd)
        {
            m_pvd = physx::PxCreatePvd(*m_foundation);
            physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate(
                "127.0.0.1", 5425, 10
            );
            if (transport)
            {
                m_pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
            }
        }

        // 3. Physics 생성
        m_physics = PxCreatePhysics(
            PX_PHYSICS_VERSION,
            *m_foundation,
            physx::PxTolerancesScale(),
            true,   // trackOutstandingAllocations
            m_pvd
        );

        if (!m_physics)
        {
            LOG_ERROR("[PhysicsSystem] Failed to create PxPhysics");
            return;
        }

        // 4. Extensions 초기화
        if (!PxInitExtensions(*m_physics, m_pvd))
        {
            LOG_ERROR("[PhysicsSystem] Failed to initialize PhysX extensions");
            return;
        }

        // 5. CPU Dispatcher 생성
        m_cpuDispatcher = physx::PxDefaultCpuDispatcherCreate(m_settings.workerThreads);
        if (!m_cpuDispatcher)
        {
            LOG_ERROR("[PhysicsSystem] Failed to create CPU dispatcher");
            return;
        }

        // 6. 기본 재질 생성
        m_defaultMaterial = m_physics->createMaterial(0.5f, 0.5f, 0.1f);

        // 7. 레이어 매트릭스 기본 설정
        m_layerMatrix.SetupDefault();

        // 8. 디버그 렌더러 초기화
        PhysicsDebugRenderer::Get().Initialize();

        m_isInitialized = true;
        LOG_PRINT("[PhysicsSystem] Initialized successfully");
    }

    void PhysicsSystem::Shutdown()
    {
        if (!m_isInitialized)
        {
            return;
        }

        // 디버그 렌더러 정리
        PhysicsDebugRenderer::Get().Shutdown();

        // Scene 데이터 정리
        for (auto& [scene, data] : m_sceneDataMap)
        {
            if (data.controllerManager)
            {
                data.controllerManager->release();
            }
            if (data.pxScene)
            {
                data.pxScene->release();
            }
        }
        m_sceneDataMap.clear();

        // 기본 재질 해제
        if (m_defaultMaterial)
        {
            m_defaultMaterial->release();
            m_defaultMaterial = nullptr;
        }

        // Extensions 종료
        PxCloseExtensions();

        // CPU Dispatcher 해제
        if (m_cpuDispatcher)
        {
            m_cpuDispatcher->release();
            m_cpuDispatcher = nullptr;
        }

        // Physics 해제
        if (m_physics)
        {
            m_physics->release();
            m_physics = nullptr;
        }

        // PVD 해제
        if (m_pvd)
        {
            physx::PxPvdTransport* transport = m_pvd->getTransport();
            m_pvd->release();
            m_pvd = nullptr;
            if (transport)
            {
                transport->release();
            }
        }

        // Foundation 해제
        if (m_foundation)
        {
            m_foundation->release();
            m_foundation = nullptr;
        }

        m_isInitialized = false;
        LOG_PRINT("[PhysicsSystem] Shutdown complete");
    }

    // ═══════════════════════════════════════════════════════════════
    // Scene 관리
    // ═══════════════════════════════════════════════════════════════

    void PhysicsSystem::CreateScenePhysics(Scene* scene, const PhysicsSceneSettings& settings)
    {
        if (!m_isInitialized || !scene)
        {
            return;
        }

        // 이미 존재하면 스킵
        if (m_sceneDataMap.find(scene) != m_sceneDataMap.end())
        {
            return;
        }

        // Scene 설정
        physx::PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
        sceneDesc.gravity = PhysicsUtility::ToPxVec3(settings.gravity);
        sceneDesc.cpuDispatcher = m_cpuDispatcher;
        sceneDesc.filterShader = PhysicsFilterShader;
        sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVE_ACTORS;
        
        // 솔버 설정
        sceneDesc.solverType = physx::PxSolverType::ePGS;

        // PxScene 생성
        PxSceneData data;
        data.pxScene = m_physics->createScene(sceneDesc);
        
        if (!data.pxScene)
        {
            LOG_ERROR("[PhysicsSystem] Failed to create PxScene");
            return;
        }

        // 이벤트 콜백 설정
        data.pxScene->setSimulationEventCallback(&data.eventCallback);

        // CharacterController Manager 생성
        data.controllerManager = PxCreateControllerManager(*data.pxScene);

        // PVD 클라이언트 설정
        if (m_pvd && m_pvd->isConnected())
        {
            physx::PxPvdSceneClient* pvdClient = data.pxScene->getScenePvdClient();
            if (pvdClient)
            {
                pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
                pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
                pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
            }
        }

        m_sceneDataMap[scene] = std::move(data);
        LOG_PRINT("[PhysicsSystem] Created physics scene");

        // 이미 씬에 존재하는 물리 컴포넌트들을 등록
        RegisterExistingPhysicsComponents(scene);
    }

    void PhysicsSystem::RegisterExistingPhysicsComponents(Scene* scene)
    {
        if (!scene) return;

        const auto& gameObjects = scene->GetGameObjects();
        
        for (const auto& go : gameObjects)
        {
            if (!go) continue;

            // Rigidbody 등록 및 초기화
            if (Rigidbody* rb = go->GetComponent<Rigidbody>())
            {
                if (!rb->GetPxActor())
                {
                    rb->Awake();  // Actor 생성
                }
                RegisterRigidbody(rb);
            }

            // Collider 등록 및 초기화
            if (Collider* collider = go->GetComponent<Collider>())
            {
                if (!collider->GetPxShape())
                {
                    collider->Awake();  // Shape 생성
                }
                RegisterCollider(collider);
            }

            // CharacterController 등록 및 초기화
            if (CharacterController* controller = go->GetComponent<CharacterController>())
            {
                if (!controller->GetPxController())
                {
                    controller->Awake();  // Controller 생성
                }
                RegisterController(controller);
            }
        }
    }

    void PhysicsSystem::DestroyScenePhysics(Scene* scene)
    {
        auto it = m_sceneDataMap.find(scene);
        if (it == m_sceneDataMap.end())
        {
            return;
        }

        PxSceneData& data = it->second;

        // 컴포넌트 정리
        data.rigidbodies.clear();
        data.colliders.clear();
        data.controllers.clear();

        // CharacterController Manager 해제
        if (data.controllerManager)
        {
            data.controllerManager->release();
            data.controllerManager = nullptr;
        }

        // PxScene 해제
        if (data.pxScene)
        {
            data.pxScene->release();
            data.pxScene = nullptr;
        }

        m_sceneDataMap.erase(it);
        LOG_PRINT("[PhysicsSystem] Destroyed physics scene");
    }

    // ═══════════════════════════════════════════════════════════════
    // 업데이트
    // ═══════════════════════════════════════════════════════════════

    void PhysicsSystem::Update(float deltaTime)
    {
        Scene* activeScene = SceneManager::Get().GetScene();
        if (activeScene)
        {
            Update(activeScene, deltaTime);
        }
    }

    void PhysicsSystem::Update(Scene* scene, float deltaTime)
    {
        PxSceneData* data = GetSceneData(scene);
        if (!data || !data->pxScene)
        {
            return;
        }

        // 1. Transform → PhysX 동기화 (Kinematic, 수동 이동)
        SyncTransformsToPhysics(*data);

        // 2. Fixed Timestep 시뮬레이션
        data->accumulator += deltaTime;
        
        uint32_t steps = 0;
        while (data->accumulator >= m_settings.fixedTimeStep && 
               steps < m_settings.maxSubSteps)
        {
            Simulate(*data, m_settings.fixedTimeStep);
            data->accumulator -= m_settings.fixedTimeStep;
            steps++;
        }

        // 3. PhysX → Transform 동기화 (Dynamic)
        SyncPhysicsToTransforms(*data);

        // 4. 충돌 이벤트 처리는 CollisionSystem에서
    }

    // ═══════════════════════════════════════════════════════════════
    // 컴포넌트 등록/해제
    // ═══════════════════════════════════════════════════════════════

    void PhysicsSystem::RegisterRigidbody(Rigidbody* rb)
    {
        if (!rb) return;

        PxSceneData* data = GetActiveSceneData();
        if (!data) return;

        // 중복 체크
        auto it = std::find(data->rigidbodies.begin(), data->rigidbodies.end(), rb);
        if (it != data->rigidbodies.end()) return;

        data->rigidbodies.push_back(rb);

        // PxActor를 Scene에 추가
        physx::PxRigidActor* actor = rb->GetPxActor();
        if (actor && data->pxScene)
        {
            data->pxScene->addActor(*actor);
        }
    }

    void PhysicsSystem::UnregisterRigidbody(Rigidbody* rb)
    {
        if (!rb) return;

        PxSceneData* data = GetActiveSceneData();
        if (!data) return;

        // PxActor를 Scene에서 제거
        physx::PxRigidActor* actor = rb->GetPxActor();
        if (actor && data->pxScene)
        {
            data->pxScene->removeActor(*actor);
        }

        // 컨테이너에서 제거 (swap and pop)
        auto it = std::find(data->rigidbodies.begin(), data->rigidbodies.end(), rb);
        if (it != data->rigidbodies.end())
        {
            *it = data->rigidbodies.back();
            data->rigidbodies.pop_back();
        }
    }

    void PhysicsSystem::RegisterCollider(Collider* collider)
    {
        if (!collider) return;

        PxSceneData* data = GetActiveSceneData();
        if (!data) return;

        // 중복 체크
        auto it = std::find(data->colliders.begin(), data->colliders.end(), collider);
        if (it != data->colliders.end()) return;

        data->colliders.push_back(collider);
    }

    void PhysicsSystem::UnregisterCollider(Collider* collider)
    {
        if (!collider) return;

        PxSceneData* data = GetActiveSceneData();
        if (!data) return;

        // 컨테이너에서 제거
        auto it = std::find(data->colliders.begin(), data->colliders.end(), collider);
        if (it != data->colliders.end())
        {
            *it = data->colliders.back();
            data->colliders.pop_back();
        }
    }

    void PhysicsSystem::RegisterController(CharacterController* controller)
    {
        if (!controller) return;

        PxSceneData* data = GetActiveSceneData();
        if (!data) return;

        auto it = std::find(data->controllers.begin(), data->controllers.end(), controller);
        if (it != data->controllers.end()) return;

        data->controllers.push_back(controller);
    }

    void PhysicsSystem::UnregisterController(CharacterController* controller)
    {
        if (!controller) return;

        PxSceneData* data = GetActiveSceneData();
        if (!data) return;

        auto it = std::find(data->controllers.begin(), data->controllers.end(), controller);
        if (it != data->controllers.end())
        {
            *it = data->controllers.back();
            data->controllers.pop_back();
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 쿼리
    // ═══════════════════════════════════════════════════════════════

    bool PhysicsSystem::Raycast(
        const Vector3& origin,
        const Vector3& direction,
        float maxDistance,
        RaycastHit& outHit,
        uint32_t layerMask)
    {
        outHit = RaycastHit{};

        physx::PxScene* pxScene = GetActivePxScene();
        if (!pxScene) return false;

        physx::PxVec3 pxOrigin = PhysicsUtility::ToPxVec3(origin);
        physx::PxVec3 pxDir = PhysicsUtility::ToPxDirection(direction);
        pxDir.normalize();

        physx::PxRaycastBuffer hit;
        
        // 필터 데이터 설정
        physx::PxQueryFilterData filterData;
        filterData.data.word0 = 0;          // 쿼리 레이어 (사용 안 함)
        filterData.data.word1 = layerMask;  // 충돌할 레이어 마스크

        bool hasHit = pxScene->raycast(
            pxOrigin, pxDir, maxDistance,
            hit,
            physx::PxHitFlag::eDEFAULT,
            filterData
        );

        if (hasHit && hit.hasBlock)
        {
            outHit.hasHit = true;
            outHit.point = PhysicsUtility::ToVector3(hit.block.position);
            outHit.normal = PhysicsUtility::ToDirection(hit.block.normal);
            outHit.distance = hit.block.distance;

            if (hit.block.shape)
            {
                Collider* col = static_cast<Collider*>(hit.block.shape->userData);
                outHit.collider = Ptr<Collider>(col);
                if (col)
                {
                    outHit.rigidbody = Ptr<Rigidbody>(col->GetAttachedRigidbody());
                    outHit.gameObject = Ptr<GameObject>(col->GetGameObject());
                }
            }
        }

        return outHit.hasHit;
    }

    bool PhysicsSystem::RaycastAll(
        const Vector3& origin,
        const Vector3& direction,
        float maxDistance,
        std::vector<RaycastHit>& outHits,
        uint32_t layerMask)
    {
        outHits.clear();

        physx::PxScene* pxScene = GetActivePxScene();
        if (!pxScene) return false;

        physx::PxVec3 pxOrigin = PhysicsUtility::ToPxVec3(origin);
        physx::PxVec3 pxDir = PhysicsUtility::ToPxDirection(direction);
        pxDir.normalize();

        const physx::PxU32 maxHits = 256;
        physx::PxRaycastHit hitBuffer[maxHits];
        physx::PxRaycastBuffer hits(hitBuffer, maxHits);

        physx::PxQueryFilterData filterData;
        filterData.data.word1 = layerMask;

        bool hasHit = pxScene->raycast(
            pxOrigin, pxDir, maxDistance,
            hits,
            physx::PxHitFlag::eDEFAULT,
            filterData
        );

        if (hasHit)
        {
            for (physx::PxU32 i = 0; i < hits.nbTouches; ++i)
            {
                const physx::PxRaycastHit& touch = hits.touches[i];
                
                RaycastHit result;
                result.hasHit = true;
                result.point = PhysicsUtility::ToVector3(touch.position);
                result.normal = PhysicsUtility::ToDirection(touch.normal);
                result.distance = touch.distance;

                if (touch.shape)
                {
                    Collider* col = static_cast<Collider*>(touch.shape->userData);
                    result.collider = Ptr<Collider>(col);
                    if (col)
                    {
                        result.rigidbody = Ptr<Rigidbody>(col->GetAttachedRigidbody());
                        result.gameObject = Ptr<GameObject>(col->GetGameObject());
                    }
                }

                outHits.push_back(result);
            }
        }

        return !outHits.empty();
    }

    bool PhysicsSystem::SphereCast(
        const Vector3& origin,
        float radius,
        const Vector3& direction,
        float maxDistance,
        RaycastHit& outHit,
        uint32_t layerMask)
    {
        outHit = RaycastHit{};

        physx::PxScene* pxScene = GetActivePxScene();
        if (!pxScene) return false;

        physx::PxVec3 pxOrigin = PhysicsUtility::ToPxVec3(origin);
        physx::PxVec3 pxDir = PhysicsUtility::ToPxDirection(direction);
        pxDir.normalize();

        physx::PxSphereGeometry sphere(radius);
        physx::PxTransform pose(pxOrigin);

        physx::PxSweepBuffer hit;

        physx::PxQueryFilterData filterData;
        filterData.data.word1 = layerMask;

        bool hasHit = pxScene->sweep(
            sphere, pose,
            pxDir, maxDistance,
            hit,
            physx::PxHitFlag::eDEFAULT,
            filterData
        );

        if (hasHit && hit.hasBlock)
        {
            outHit.hasHit = true;
            outHit.point = PhysicsUtility::ToVector3(hit.block.position);
            outHit.normal = PhysicsUtility::ToDirection(hit.block.normal);
            outHit.distance = hit.block.distance;

            if (hit.block.shape)
            {
                Collider* col = static_cast<Collider*>(hit.block.shape->userData);
                outHit.collider = Ptr<Collider>(col);
                if (col)
                {
                    outHit.rigidbody = Ptr<Rigidbody>(col->GetAttachedRigidbody());
                    outHit.gameObject = Ptr<GameObject>(col->GetGameObject());
                }
            }
        }

        return outHit.hasHit;
    }

    bool PhysicsSystem::OverlapSphere(
        const Vector3& center,
        float radius,
        std::vector<Collider*>& outColliders,
        uint32_t layerMask)
    {
        outColliders.clear();

        physx::PxScene* pxScene = GetActivePxScene();
        if (!pxScene) return false;

        physx::PxSphereGeometry sphere(radius);
        physx::PxTransform pose(PhysicsUtility::ToPxVec3(center));

        const physx::PxU32 maxHits = 256;
        physx::PxOverlapHit hitBuffer[maxHits];
        physx::PxOverlapBuffer hits(hitBuffer, maxHits);

        physx::PxQueryFilterData filterData;
        filterData.data.word1 = layerMask;

        bool hasHit = pxScene->overlap(sphere, pose, hits, filterData);

        if (hasHit)
        {
            for (physx::PxU32 i = 0; i < hits.nbTouches; ++i)
            {
                if (hits.touches[i].shape)
                {
                    Collider* col = static_cast<Collider*>(hits.touches[i].shape->userData);
                    if (col)
                    {
                        outColliders.push_back(col);
                    }
                }
            }
        }

        return !outColliders.empty();
    }

    bool PhysicsSystem::OverlapBox(
        const Vector3& center,
        const Vector3& halfExtents,
        const Quaternion& rotation,
        std::vector<Collider*>& outColliders,
        uint32_t layerMask)
    {
        outColliders.clear();

        physx::PxScene* pxScene = GetActivePxScene();
        if (!pxScene) return false;

        physx::PxBoxGeometry box(PhysicsUtility::ToPxScale(halfExtents));
        physx::PxTransform pose = PhysicsUtility::ToPxTransform(center, rotation);

        const physx::PxU32 maxHits = 256;
        physx::PxOverlapHit hitBuffer[maxHits];
        physx::PxOverlapBuffer hits(hitBuffer, maxHits);

        physx::PxQueryFilterData filterData;
        filterData.data.word1 = layerMask;

        bool hasHit = pxScene->overlap(box, pose, hits, filterData);

        if (hasHit)
        {
            for (physx::PxU32 i = 0; i < hits.nbTouches; ++i)
            {
                if (hits.touches[i].shape)
                {
                    Collider* col = static_cast<Collider*>(hits.touches[i].shape->userData);
                    if (col)
                    {
                        outColliders.push_back(col);
                    }
                }
            }
        }

        return !outColliders.empty();
    }

    // ═══════════════════════════════════════════════════════════════
    // 접근자
    // ═══════════════════════════════════════════════════════════════

    physx::PxScene* PhysicsSystem::GetPxScene(Scene* scene) const
    {
        auto it = m_sceneDataMap.find(scene);
        if (it != m_sceneDataMap.end())
        {
            return it->second.pxScene;
        }
        return nullptr;
    }

    physx::PxScene* PhysicsSystem::GetActivePxScene() const
    {
        Scene* activeScene = SceneManager::Get().GetScene();
        return GetPxScene(activeScene);
    }

    physx::PxControllerManager* PhysicsSystem::GetControllerManager(Scene* scene) const
    {
        auto it = m_sceneDataMap.find(scene);
        if (it != m_sceneDataMap.end())
        {
            return it->second.controllerManager;
        }
        return nullptr;
    }

    // ═══════════════════════════════════════════════════════════════
    // Private 헬퍼
    // ═══════════════════════════════════════════════════════════════

    void PhysicsSystem::SyncTransformsToPhysics(PxSceneData& data)
    {
        // Rigidbody 동기화
        for (Rigidbody* rb : data.rigidbodies)
        {
            if (!rb || !rb->GetPxActor()) continue;

            // Kinematic: 항상 동기화
            if (rb->IsKinematic())
            {
                physx::PxRigidDynamic* dynamic = rb->GetPxActor()->is<physx::PxRigidDynamic>();
                if (dynamic)
                {
                    physx::PxTransform target = PhysicsUtility::ToPxTransform(rb->GetTransform());
                    dynamic->setKinematicTarget(target);
                }
            }
            // Dynamic: Transform이 수정된 경우만 (텔레포트 등)
            else if (rb->IsDynamic() && rb->HasPendingTeleport())
            {
                physx::PxRigidDynamic* dynamic = rb->GetPxActor()->is<physx::PxRigidDynamic>();
                if (dynamic)
                {
                    rb->ApplyPendingTeleport();
                }
            }
        }

        // 독립 Collider 동기화 (Rigidbody 없는 Collider)
        for (Collider* col : data.colliders)
        {
            if (!col || col->HasRigidbody()) continue;

            // Transform이 변경된 경우만
            Transform* transform = col->GetTransform();
            if (transform && transform->IsDirtyThisFrame())
            {
                physx::PxRigidStatic* staticActor = col->GetOwnedStaticActor();
                if (staticActor)
                {
                    staticActor->setGlobalPose(PhysicsUtility::ToPxTransform(transform));
                }
            }
        }
    }

    void PhysicsSystem::Simulate(PxSceneData& data, float timeStep)
    {
        if (!data.pxScene) return;

        data.isSimulating = true;
        data.pxScene->simulate(timeStep);
        data.pxScene->fetchResults(true);
        data.isSimulating = false;
    }

    void PhysicsSystem::SyncPhysicsToTransforms(PxSceneData& data)
    {
        // Active Actors만 처리 (최적화)
        physx::PxU32 nbActiveActors = 0;
        physx::PxActor** activeActors = data.pxScene->getActiveActors(nbActiveActors);

        for (physx::PxU32 i = 0; i < nbActiveActors; ++i)
        {
            physx::PxRigidDynamic* dynamic = activeActors[i]->is<physx::PxRigidDynamic>();
            if (!dynamic) continue;

            // Kinematic은 스킵 (엔진이 제어)
            if (dynamic->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC)
            {
                continue;
            }

            // Rigidbody 찾기
            Rigidbody* rb = static_cast<Rigidbody*>(dynamic->userData);
            if (!rb) continue;

            // PhysX Transform → 엔진 Transform
            physx::PxTransform pxTransform = dynamic->getGlobalPose();
            PhysicsUtility::ApplyPxTransformToTransform(pxTransform, rb->GetTransform());
        }
    }

    PhysicsSystem::PxSceneData* PhysicsSystem::GetSceneData(Scene* scene)
    {
        auto it = m_sceneDataMap.find(scene);
        if (it != m_sceneDataMap.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    PhysicsSystem::PxSceneData* PhysicsSystem::GetActiveSceneData()
    {
        Scene* activeScene = SceneManager::Get().GetScene();
        return GetSceneData(activeScene);
    }

    const std::vector<Collider*>& PhysicsSystem::GetRegisteredColliders() const
    {
        static const std::vector<Collider*> empty;
        
        Scene* activeScene = SceneManager::Get().GetScene();
        auto it = m_sceneDataMap.find(activeScene);
        if (it != m_sceneDataMap.end())
        {
            return it->second.colliders;
        }
        return empty;
    }

    const std::vector<CharacterController*>& PhysicsSystem::GetRegisteredControllers() const
    {
        static const std::vector<CharacterController*> empty;
        
        Scene* activeScene = SceneManager::Get().GetScene();
        auto it = m_sceneDataMap.find(activeScene);
        if (it != m_sceneDataMap.end())
        {
            return it->second.controllers;
        }
        return empty;
    }
}
