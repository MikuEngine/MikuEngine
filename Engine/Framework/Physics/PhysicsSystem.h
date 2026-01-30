#pragma once

#include <PxPhysicsAPI.h>
#include <vector>
#include <memory>

#include "Framework/Physics/PhysicsLayer.h"
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
        
        // 솔버 (Actor 레벨에서 적용됨 - Rigidbody::CreatePxActor)
        uint32_t solverIterations = 6;          // 위치 솔버 반복 횟수 (penetration 해결)
        uint32_t velocitySolverIterations = 2;  // 속도 솔버 반복 횟수 (마찰/반발 정확도)
        
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
    // 
    // 설계:
    //   - Scene이 물리 컴포넌트 참조와 이벤트 큐를 소유
    //   - PhysicsSystem은 PhysX 코어만 관리하고, Scene의 데이터 참조
    //   - 씬 전환 시 Scene::Clear()에서 자동 정리
    //   - 소멸자에서 PhysX 코어 리소스만 해제
    // ═══════════════════════════════════════════════════════════════

    class PhysicsSystem
    {
    public:
        PhysicsSystem() = default;
        ~PhysicsSystem();  // 소멸자에서 PhysX 코어 리소스 정리
        
        // 복사/이동 금지
        PhysicsSystem(const PhysicsSystem&) = delete;
        PhysicsSystem& operator=(const PhysicsSystem&) = delete;
        PhysicsSystem(PhysicsSystem&&) = delete;
        PhysicsSystem& operator=(PhysicsSystem&&) = delete;

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
        
        // 재질 (프리셋)
        physx::PxMaterial* m_defaultMaterial = nullptr;
        physx::PxMaterial* m_slipperyMaterial = nullptr;  // Ice 프리셋 (미끄러운 충돌용)
        
        // 레이어 매트릭스
        PhysicsLayerMatrix m_layerMatrix;
        
        // ═══════════════════════════════════════
        // 설정
        // ═══════════════════════════════════════
        PhysicsSettings m_settings;
        bool m_isInitialized = false;

    public:
        // ═══════════════════════════════════════
        // 생명주기
        // ═══════════════════════════════════════
        void Initialize(const PhysicsSettings& settings = {});
        
        // Shutdown()은 더 이상 필요 없음 - 소멸자에서 처리
        // 하위 호환성을 위해 빈 함수로 유지
        void Shutdown() {}
        
        bool IsInitialized() const { return m_isInitialized; }

        // ═══════════════════════════════════════
        // 씬 물리 관리
        // Scene에 PxScene과 ControllerManager 생성하여 전달
        // ═══════════════════════════════════════
        void CreateScenePhysics(const PhysicsSceneSettings& settings = {});
        void ClearScenePhysics();  // 씬 전환 시 호출
        
        // 하위 호환성 (Scene* 파라미터 무시)
        void CreateScenePhysics(Scene* /*scene*/, const PhysicsSceneSettings& settings = {}) 
        { 
            CreateScenePhysics(settings); 
        }
        void DestroyScenePhysics(Scene* /*scene*/) 
        { 
            ClearScenePhysics(); 
        }
        
        // ═══════════════════════════════════════
        // 업데이트
        // ═══════════════════════════════════════
        void Update(float deltaTime);
        
        // 하위 호환성 (Scene* 파라미터 무시)
        void Update(Scene* /*scene*/, float deltaTime) { Update(deltaTime); }

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
        
        // QueryFilter를 사용하는 SphereCast (자기 자신 제외, 레이어 필터링 등)
        bool SphereCast(
            const Vector3& origin,
            float radius,
            const Vector3& direction,
            float maxDistance,
            RaycastHit& outHit,
            const QueryFilter& filter
        );
        
        // 모든 충돌을 반환하는 SphereCastAll (QueryFilter 사용)
        bool SphereCastAll(
            const Vector3& origin,
            float radius,
            const Vector3& direction,
            float maxDistance,
            std::vector<RaycastHit>& outHits,
            const QueryFilter& filter
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
        physx::PxMaterial* GetDefaultMaterial() const { return m_defaultMaterial; }
        physx::PxMaterial* GetSlipperyMaterial() const { return m_slipperyMaterial; }
        
        // Scene에서 PxScene 접근 (Scene을 통해)
        physx::PxScene* GetActivePxScene() const;
        physx::PxControllerManager* GetActiveControllerManager() const;
        
        // 하위 호환성
        physx::PxScene* GetPxScene() const { return GetActivePxScene(); }
        physx::PxScene* GetPxScene(Scene* /*scene*/) const { return GetActivePxScene(); }
        physx::PxControllerManager* GetControllerManager() const { return GetActiveControllerManager(); }
        physx::PxControllerManager* GetControllerManager(Scene* /*scene*/) const { return GetActiveControllerManager(); }
        
        const PhysicsSettings& GetSettings() const { return m_settings; }
        PhysicsLayerMatrix& GetLayerMatrix() { return m_layerMatrix; }
        const PhysicsLayerMatrix& GetLayerMatrix() const { return m_layerMatrix; }

        // 디버그 렌더링 토글
        void SetDebugRenderEnabled(bool enabled) { m_settings.enableDebugRender = enabled; }
        bool IsDebugRenderEnabled() const { return m_settings.enableDebugRender; }

        // 등록된 컴포넌트 접근 (디버그 렌더링용, Scene에서 가져옴)
        const std::vector<Collider*>& GetRegisteredColliders() const;
        const std::vector<CharacterController*>& GetRegisteredControllers() const;

    private:
        // 시뮬레이션 헬퍼
        void SyncTransformsToPhysics(Scene* scene);
        void Simulate(Scene* scene, float timeStep);
        void SyncPhysicsToTransforms(Scene* scene);
        
        // PhysX 리소스 정리 (소멸자에서 호출)
        void ReleasePhysXResources();
        
        // 기존 물리 컴포넌트 등록 (씬 로드 후)
        void RegisterExistingPhysicsComponents(Scene* scene);
    };
}
