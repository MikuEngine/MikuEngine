#include "EnginePCH.h"
#include "PhysicsSystem.h"

#include "Framework/System/SystemManager.h"
#include "Framework/system/ScriptSystem.h"
#include "Framework/Physics/PhysicsUtility.h"
#include "Framework/Physics/CollisionSystem.h"
#include "Framework/Object/Component/Rigidbody.h"
#include "Framework/Object/Component/Collider.h"
#include "Framework/Object/Component/CharacterController.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"
#include "Framework/Physics/PhysicsCallback.h"

namespace engine
{
    // ═══════════════════════════════════════════════════════════════
    // 소멸자 - PhysX 코어 리소스만 정리
    // ═══════════════════════════════════════════════════════════════

    PhysicsSystem::~PhysicsSystem()
    {
        ReleasePhysXResources();
    }

    void PhysicsSystem::ReleasePhysXResources()
    {
        if (!m_isInitialized)
        {
            return;
        }

        // 재질 해제
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
    // 초기화
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

        // 6. 재질 생성
        // 기본 재질 (일반 충돌)
        m_defaultMaterial = m_physics->createMaterial(0.5f, 0.5f, 0.1f);
        
        // 7. 레이어 매트릭스 기본 설정
        m_layerMatrix.SetupDefault();

        m_isInitialized = true;
        LOG_PRINT("[PhysicsSystem] Initialized successfully");
    }

    // ═══════════════════════════════════════════════════════════════
    // 씬 물리 관리
    // ═══════════════════════════════════════════════════════════════

    void PhysicsSystem::CreateScenePhysics(const PhysicsSceneSettings& settings)
    {
        if (!m_isInitialized)
        {
            return;
        }

        Scene* scene = SceneManager::Get().GetScene();
        if (!scene)
        {
            return;
        }

        // 이미 PxScene이 있으면 스킵
        if (scene->GetPxScene())
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
        
        // 참고: Solver Iterations는 Scene 레벨이 아닌 Actor 레벨에서 설정됨
        // Rigidbody::CreatePxActor()에서 setSolverIterationCounts() 호출

        // PxScene 생성
        physx::PxScene* pxScene = m_physics->createScene(sceneDesc);
        
        if (!pxScene)
        {
            LOG_ERROR("[PhysicsSystem] Failed to create PxScene");
            return;
        }

        // CharacterController Manager 생성
        physx::PxControllerManager* controllerManager = PxCreateControllerManager(*pxScene);

        // PVD 클라이언트 설정
        if (m_pvd && m_pvd->isConnected())
        {
            physx::PxPvdSceneClient* pvdClient = pxScene->getScenePvdClient();
            if (pvdClient)
            {
                pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
                pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
                pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
            }
        }

        // Scene에 물리 데이터 전달
        scene->CreatePhysicsScene(pxScene, controllerManager);
        
        LOG_PRINT("[PhysicsSystem] Created physics scene");

        // 이미 씬에 존재하는 물리 컴포넌트들을 등록
        RegisterExistingPhysicsComponents(scene);
    }

    void PhysicsSystem::ClearScenePhysics()
    {
        Scene* scene = SceneManager::Get().GetScene();
        if (scene)
        {
            scene->ClearPhysicsScene();
        }
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
                scene->RegisterRigidbody(rb);
            }

            // Collider 등록 및 초기화
            if (Collider* collider = go->GetComponent<Collider>())
            {
                if (!collider->GetPxShape())
                {
                    collider->Awake();  // Shape 생성
                }
                scene->RegisterCollider(collider);
            }

            // CharacterController 등록 및 초기화
            if (CharacterController* controller = go->GetComponent<CharacterController>())
            {
                if (!controller->GetPxController())
                {
                    controller->Awake();  // Controller 생성
                }
                scene->RegisterController(controller);
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 업데이트
    // ═══════════════════════════════════════════════════════════════

    void PhysicsSystem::Update(float deltaTime)
    {
        Scene* scene = SceneManager::Get().GetScene();
        if (!scene || !scene->GetPxScene())
        {
            return;
        }

        // 1. Transform → PhysX 동기화 (Kinematic, 수동 이동)
        SyncTransformsToPhysics(scene);

        // 2. Fixed Timestep 시뮬레이션 (단일 Accumulator 패턴)
        // - PhysicsSystem이 fixed timestep의 유일한 관리자
        // - ScriptSystem.CallFixedUpdate()도 여기서 호출
        float& accumulator = scene->GetPhysicsAccumulator();
        accumulator += deltaTime;
        
        uint32_t steps = 0;
        while (accumulator >= m_settings.fixedTimeStep && 
               steps < m_settings.maxSubSteps)
        {
            // Script FixedUpdate 호출 (물리 시뮬레이션 전)
            SystemManager::Get().GetScriptSystem().CallFixedUpdate();
            
            // Physics 시뮬레이션
            Simulate(scene, m_settings.fixedTimeStep);
            accumulator -= m_settings.fixedTimeStep;
            steps++;
        }

        // 3. PhysX → Transform 동기화 (시뮬레이션이 실행된 경우만)
        // 참고: getActiveActors는 simulate/fetchResults 후에만 유효한 결과 반환
        if (steps > 0)
        {
            SyncPhysicsToTransforms(scene);
        }

        // 4. 충돌 이벤트 처리
        SystemManager::Get().GetCollisionSystem().ProcessEvents();
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
        std::vector<physx::PxRaycastHit> hitBuffer(maxHits);
        physx::PxRaycastBuffer hits(hitBuffer.data(), maxHits);

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

    // ═══════════════════════════════════════════════════════════════
    // QueryFilter를 사용하는 SphereCast
    // 자기 자신 제외, 레이어 필터링, 바닥/천장 필터링 지원
    // ═══════════════════════════════════════════════════════════════
    bool PhysicsSystem::SphereCast(
        const Vector3& origin,
        float radius,
        const Vector3& direction,
        float maxDistance,
        RaycastHit& outHit,
        const QueryFilter& filter)
    {
        outHit = RaycastHit{};

        physx::PxScene* pxScene = GetActivePxScene();
        if (!pxScene) return false;

        physx::PxVec3 pxOrigin = PhysicsUtility::ToPxVec3(origin);
        physx::PxVec3 pxDir = PhysicsUtility::ToPxDirection(direction);
        pxDir.normalize();

        physx::PxSphereGeometry sphere(radius);
        physx::PxTransform pose(pxOrigin);

        // 모든 충돌을 수집하기 위해 멀티 히트 사용
        const physx::PxU32 maxHits = 64;
        physx::PxSweepHit hitBuffer[maxHits];
        physx::PxSweepBuffer hits(hitBuffer, maxHits);

        physx::PxQueryFilterData filterData;
        filterData.flags = physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC;

        bool hasHit = pxScene->sweep(
            sphere, pose,
            pxDir, maxDistance,
            hits,
            physx::PxHitFlag::eDEFAULT,
            filterData
        );

        if (!hasHit) return false;

        // 모든 히트를 순회하며 필터 조건에 맞는 가장 가까운 것 찾기
        float closestDist = FLT_MAX;
        physx::PxSweepHit* bestHit = nullptr;

        // 블록 히트와 터치 히트 모두 확인
        // filter의 Ptr을 로컬 raw pointer로 캐싱 (람다 캡처 시점에 유효성 검증)
        Collider* ignoreCol = filter.ignoreCollider ? filter.ignoreCollider.Get() : nullptr;
        GameObject* ignoreObj = filter.ignoreObject ? filter.ignoreObject.Get() : nullptr;
        
        auto processHit = [&](physx::PxSweepHit& hit) {
            if (!hit.shape) return;

            Collider* col = static_cast<Collider*>(hit.shape->userData);
            if (!col) return;

            // 제외할 오브젝트 확인 (캐싱된 포인터 사용)
            if (ignoreCol && col == ignoreCol) return;
            if (ignoreObj && col->GetGameObject() == ignoreObj) return;

            // 레이어 마스크 확인
            uint32_t layer = col->GetLayer();
            uint32_t layerBit = (1u << layer);
            if (!(filter.layerMask & layerBit)) return;

            // 바닥/천장 필터링
            Vector3 normal = PhysicsUtility::ToDirection(hit.normal);
            if (filter.ignoreFloor && normal.y > filter.floorCeilingThreshold) return;
            if (filter.ignoreCeiling && normal.y < -filter.floorCeilingThreshold) return;

            // 가장 가까운 히트 업데이트
            if (hit.distance < closestDist)
            {
                closestDist = hit.distance;
                bestHit = &hit;
            }
        };

        // 블록 히트 확인
        if (hits.hasBlock)
        {
            processHit(hits.block);
        }

        // 터치 히트들 확인
        for (physx::PxU32 i = 0; i < hits.nbTouches; ++i)
        {
            processHit(hits.touches[i]);
        }

        // 유효한 히트가 없으면 실패
        if (!bestHit) return false;

        // 결과 채우기
        outHit.hasHit = true;
        outHit.point = PhysicsUtility::ToVector3(bestHit->position);
        outHit.normal = PhysicsUtility::ToDirection(bestHit->normal);
        outHit.distance = bestHit->distance;

        if (bestHit->shape)
        {
            Collider* col = static_cast<Collider*>(bestHit->shape->userData);
            outHit.collider = Ptr<Collider>(col);
            if (col)
            {
                outHit.rigidbody = Ptr<Rigidbody>(col->GetAttachedRigidbody());
                outHit.gameObject = Ptr<GameObject>(col->GetGameObject());
            }
        }

        return outHit.hasHit;
    }

    // ═══════════════════════════════════════════════════════════════
    // 모든 충돌을 반환하는 SphereCastAll (QueryFilter 사용)
    // ═══════════════════════════════════════════════════════════════
    bool PhysicsSystem::SphereCastAll(
        const Vector3& origin,
        float radius,
        const Vector3& direction,
        float maxDistance,
        std::vector<RaycastHit>& outHits,
        const QueryFilter& filter)
    {
        outHits.clear();

        physx::PxScene* pxScene = GetActivePxScene();
        if (!pxScene) return false;

        physx::PxVec3 pxOrigin = PhysicsUtility::ToPxVec3(origin);
        physx::PxVec3 pxDir = PhysicsUtility::ToPxDirection(direction);
        pxDir.normalize();

        physx::PxSphereGeometry sphere(radius);
        physx::PxTransform pose(pxOrigin);

        const physx::PxU32 maxHitCount = 64;
        physx::PxSweepHit hitBuffer[maxHitCount];
        physx::PxSweepBuffer hits(hitBuffer, maxHitCount);

        physx::PxQueryFilterData filterData;
        filterData.flags = physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC;

        bool hasHit = pxScene->sweep(
            sphere, pose,
            pxDir, maxDistance,
            hits,
            physx::PxHitFlag::eDEFAULT,
            filterData
        );

        if (!hasHit) return false;

        // filter의 Ptr을 로컬 raw pointer로 캐싱 (람다 캡처 시점에 유효성 검증)
        Collider* ignoreCol = filter.ignoreCollider ? filter.ignoreCollider.Get() : nullptr;
        GameObject* ignoreObj = filter.ignoreObject ? filter.ignoreObject.Get() : nullptr;

        auto processHit = [&](const physx::PxSweepHit& hit) {
            if (!hit.shape) return;

            Collider* col = static_cast<Collider*>(hit.shape->userData);
            if (!col) return;

            // 제외할 오브젝트 확인 (캐싱된 포인터 사용)
            if (ignoreCol && col == ignoreCol) return;
            if (ignoreObj && col->GetGameObject() == ignoreObj) return;

            // 레이어 마스크 확인
            uint32_t layer = col->GetLayer();
            uint32_t layerBit = (1u << layer);
            if (!(filter.layerMask & layerBit)) return;

            // 바닥/천장 필터링
            Vector3 normal = PhysicsUtility::ToDirection(hit.normal);
            if (filter.ignoreFloor && normal.y > filter.floorCeilingThreshold) return;
            if (filter.ignoreCeiling && normal.y < -filter.floorCeilingThreshold) return;

            // 결과 추가
            RaycastHit result;
            result.hasHit = true;
            result.point = PhysicsUtility::ToVector3(hit.position);
            result.normal = normal;
            result.distance = hit.distance;
            result.collider = Ptr<Collider>(col);
            result.rigidbody = Ptr<Rigidbody>(col->GetAttachedRigidbody());
            result.gameObject = Ptr<GameObject>(col->GetGameObject());

            outHits.push_back(result);
        };

        // 블록 히트 확인
        if (hits.hasBlock)
        {
            processHit(hits.block);
        }

        // 터치 히트들 확인
        for (physx::PxU32 i = 0; i < hits.nbTouches; ++i)
        {
            processHit(hits.touches[i]);
        }

        // 거리순 정렬
        std::sort(outHits.begin(), outHits.end(),
            [](const RaycastHit& a, const RaycastHit& b) {
                return a.distance < b.distance;
            });

        return !outHits.empty();
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

    physx::PxScene* PhysicsSystem::GetActivePxScene() const
    {
        Scene* scene = SceneManager::Get().GetScene();
        return scene ? scene->GetPxScene() : nullptr;
    }

    physx::PxControllerManager* PhysicsSystem::GetActiveControllerManager() const
    {
        Scene* scene = SceneManager::Get().GetScene();
        return scene ? scene->GetControllerManager() : nullptr;
    }

    const std::vector<Collider*>& PhysicsSystem::GetRegisteredColliders() const
    {
        static const std::vector<Collider*> empty;
        Scene* scene = SceneManager::Get().GetScene();
        return scene ? scene->GetColliders() : empty;
    }

    const std::vector<CharacterController*>& PhysicsSystem::GetRegisteredControllers() const
    {
        static const std::vector<CharacterController*> empty;
        Scene* scene = SceneManager::Get().GetScene();
        return scene ? scene->GetControllers() : empty;
    }

    // ═══════════════════════════════════════════════════════════════
    // 시뮬레이션 헬퍼
    // ═══════════════════════════════════════════════════════════════

    void PhysicsSystem::SyncTransformsToPhysics(Scene* scene)
    {
        if (!scene) return;

        const auto& rigidbodies = scene->GetRigidbodies();
        const auto& colliders = scene->GetColliders();

        // Rigidbody 동기화
        for (Rigidbody* rb : rigidbodies)
        {
            if (!rb || !rb->GetPxActor()) continue;

            // Kinematic: 항상 동기화 (위치 및 회전)
            if (rb->IsKinematic())
            {
                physx::PxRigidDynamic* dynamic = rb->GetPxActor()->is<physx::PxRigidDynamic>();
                if (dynamic)
                {
                    physx::PxTransform target = PhysicsUtility::ToPxTransform(rb->GetTransform());
                    dynamic->setGlobalPose(target);
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

        // Collider의 Transform 스케일 동기화
        for (Collider* collider : colliders)
        {
            if (collider)
            {
                collider->CheckAndSyncTransformScale();
            }
        }

        // 독립 Collider 동기화 (Rigidbody 없는 Collider)
        for (Collider* col : colliders)
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

    void PhysicsSystem::Simulate(Scene* scene, float timeStep)
    {
        if (!scene) return;
        
        physx::PxScene* pxScene = scene->GetPxScene();
        if (!pxScene) return;

        scene->SetSimulating(true);
        pxScene->simulate(timeStep);
        pxScene->fetchResults(true);
        scene->SetSimulating(false);
    }

    void PhysicsSystem::SyncPhysicsToTransforms(Scene* scene)
    {
        if (!scene) return;
        
        physx::PxScene* pxScene = scene->GetPxScene();
        if (!pxScene) return;

        // Active Actors만 처리 (최적화)
        physx::PxU32 nbActiveActors = 0;
        physx::PxActor** activeActors = pxScene->getActiveActors(nbActiveActors);

        for (physx::PxU32 i = 0; i < nbActiveActors; ++i)
        {
            // 안전 체크: Actor 포인터 유효성
            if (!activeActors[i]) continue;
            
            physx::PxRigidDynamic* dynamic = activeActors[i]->is<physx::PxRigidDynamic>();
            if (!dynamic) continue;

            // 안전 체크: Actor가 여전히 Scene에 있는지 확인
            if (!dynamic->getScene()) continue;

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
}
