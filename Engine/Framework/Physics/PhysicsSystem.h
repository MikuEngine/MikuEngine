#pragma once

#include <PxPhysicsAPI.h>
#include <unordered_map>
#include <vector>
#include <memory>

#include "Common/Utility/Singleton.h"
#include "Framework/Physics/PhysicsLayer.h"
#include "Framework/Physics/PhysicsCallback.h"
#include "Framework/Physics/CollisionTypes.h"

namespace engine
{
    class Scene;
    class Rigidbody;
    class Collider;
    class CharacterController;
    class PhysicsMaterial;

    // ═══════════════════════════════════════════════════════════════
    // 물리 시스템 설정
    // ═══════════════════════════════════════════════════════════════

    struct PhysicsSettings
    {
        // 시뮬레이션
        float fixedTimeStep = 1.0f / 60.0f;     // 고정 시간 간격
        uint32_t maxSubSteps = 8;               // 최대 서브스텝 수
        uint32_t workerThreads = 4;             // CPU 워커 스레드 수
        
        // 중력
        Vector3 defaultGravity{ 0.0f, -9.81f, 0.0f };
        
        // 솔버
        uint32_t solverIterations = 6;          // 위치 솔버 반복 횟수
        uint32_t velocitySolverIterations = 1;  // 속도 솔버 반복 횟수
        
        // 디버그
        bool enablePvd = true;                  // PhysX Visual Debugger
        bool enableDebugRender = false;         // 디버그 렌더링 (기본 OFF)
        
        // GPU (현재 비활성화)
        bool enableGpuDynamics = false;
    };

    struct PhysicsSceneSettings
    {
        Vector3 gravity{ 0.0f, -9.81f, 0.0f };
        bool enableGpuDynamics = false;
    };

    // ═══════════════════════════════════════════════════════════════
    // PhysicsSystem - PhysX 래핑 및 물리 시뮬레이션 관리
    // ═══════════════════════════════════════════════════════════════

    class PhysicsSystem : public Singleton<PhysicsSystem>
    {
    private:
        // ═══════════════════════════════════════
        // PhysX 코어 (전역, 1개)
        // ═══════════════════════════════════════
        physx::PxFoundation* m_foundation = nullptr;
        physx::PxPhysics* m_physics = nullptr;
        physx::PxDefaultCpuDispatcher* m_cpuDispatcher = nullptr;
        physx::PxDefaultAllocator m_allocator;
        physx::PxDefaultErrorCallback m_errorCallback;
        
        // PVD (Visual Debugger)
        physx::PxPvd* m_pvd = nullptr;
        
        // 기본 재질
        physx::PxMaterial* m_defaultMaterial = nullptr;
        
        // 레이어 매트릭스
        PhysicsLayerMatrix m_layerMatrix;

        // ═══════════════════════════════════════
        // Scene별 데이터
        // ═══════════════════════════════════════
        struct PxSceneData
        {
            physx::PxScene* pxScene = nullptr;
            physx::PxControllerManager* controllerManager = nullptr;
            PhysicsEventCallback eventCallback;
            
            // 컴포넌트 컨테이너
            std::vector<Rigidbody*> rigidbodies;
            std::vector<Collider*> colliders;
            std::vector<CharacterController*> controllers;
            
            // 시뮬레이션 상태
            float accumulator = 0.0f;
            bool isSimulating = false;
        };
        
        std::unordered_map<Scene*, PxSceneData> m_sceneDataMap;
        
        // ═══════════════════════════════════════
        // 설정
        // ═══════════════════════════════════════
        PhysicsSettings m_settings;
        bool m_isInitialized = false;

    private:
        PhysicsSystem() = default;
        ~PhysicsSystem() = default;

    public:
        // ═══════════════════════════════════════
        // 생명주기
        // ═══════════════════════════════════════
        void Initialize(const PhysicsSettings& settings = {});
        void Shutdown();
        
        bool IsInitialized() const { return m_isInitialized; }

        // ═══════════════════════════════════════
        // Scene 관리
        // ═══════════════════════════════════════
        void CreateScenePhysics(Scene* scene, const PhysicsSceneSettings& settings = {});
        void DestroyScenePhysics(Scene* scene);
        
        // ═══════════════════════════════════════
        // 업데이트
        // ═══════════════════════════════════════
        
        // 현재 활성 Scene의 물리 업데이트
        void Update(float deltaTime);
        
        // 특정 Scene 업데이트
        void Update(Scene* scene, float deltaTime);

        // ═══════════════════════════════════════
        // 컴포넌트 등록/해제
        // ═══════════════════════════════════════
        void RegisterRigidbody(Rigidbody* rb);
        void UnregisterRigidbody(Rigidbody* rb);
        
        void RegisterCollider(Collider* collider);
        void UnregisterCollider(Collider* collider);
        
        void RegisterController(CharacterController* controller);
        void UnregisterController(CharacterController* controller);

        // ═══════════════════════════════════════
        // 쿼리 (Raycast, Overlap 등)
        // ═══════════════════════════════════════
        bool Raycast(
            const Vector3& origin,
            const Vector3& direction,
            float maxDistance,
            RaycastHit& outHit,
            uint32_t layerMask = PhysicsLayer::Mask::All
        );
        
        bool RaycastAll(
            const Vector3& origin,
            const Vector3& direction,
            float maxDistance,
            std::vector<RaycastHit>& outHits,
            uint32_t layerMask = PhysicsLayer::Mask::All
        );
        
        bool SphereCast(
            const Vector3& origin,
            float radius,
            const Vector3& direction,
            float maxDistance,
            RaycastHit& outHit,
            uint32_t layerMask = PhysicsLayer::Mask::All
        );
        
        bool OverlapSphere(
            const Vector3& center,
            float radius,
            std::vector<Collider*>& outColliders,
            uint32_t layerMask = PhysicsLayer::Mask::All
        );
        
        bool OverlapBox(
            const Vector3& center,
            const Vector3& halfExtents,
            const Quaternion& rotation,
            std::vector<Collider*>& outColliders,
            uint32_t layerMask = PhysicsLayer::Mask::All
        );

        // ═══════════════════════════════════════
        // 접근자
        // ═══════════════════════════════════════
        physx::PxPhysics* GetPxPhysics() const { return m_physics; }
        physx::PxScene* GetPxScene(Scene* scene) const;
        physx::PxScene* GetActivePxScene() const;
        physx::PxControllerManager* GetControllerManager(Scene* scene) const;
        physx::PxMaterial* GetDefaultMaterial() const { return m_defaultMaterial; }
        
        const PhysicsSettings& GetSettings() const { return m_settings; }
        PhysicsLayerMatrix& GetLayerMatrix() { return m_layerMatrix; }
        const PhysicsLayerMatrix& GetLayerMatrix() const { return m_layerMatrix; }

        // 디버그 렌더링 토글
        void SetDebugRenderEnabled(bool enabled) { m_settings.enableDebugRender = enabled; }
        bool IsDebugRenderEnabled() const { return m_settings.enableDebugRender; }

        // 등록된 컴포넌트 접근 (디버그 렌더링용)
        const std::vector<Collider*>& GetRegisteredColliders() const;
        const std::vector<CharacterController*>& GetRegisteredControllers() const;

    private:
        // 시뮬레이션 헬퍼
        void SyncTransformsToPhysics(PxSceneData& data);
        void Simulate(PxSceneData& data, float timeStep);
        void SyncPhysicsToTransforms(PxSceneData& data);
        
        // Scene 데이터 접근
        PxSceneData* GetSceneData(Scene* scene);
        PxSceneData* GetActiveSceneData();
        
        // 기존 물리 컴포넌트 등록 (씬 로드 후)
        void RegisterExistingPhysicsComponents(Scene* scene);

        friend class Singleton<PhysicsSystem>;
    };
}
