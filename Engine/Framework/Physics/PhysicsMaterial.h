#pragma once

#include <PxPhysicsAPI.h>

namespace engine
{
    // ═══════════════════════════════════════════════════════════════
    // 물리 재질 (마찰, 반발 계수)
    // 
    // 현재: 기본값 사용
    // 확장: Asset 시스템에 통합하여 파일로 저장/로드 가능
    // ═══════════════════════════════════════════════════════════════

    struct PhysicsMaterialDesc
    {
        float staticFriction = 0.5f;    // 정지 마찰 계수
        float dynamicFriction = 0.5f;   // 동적 마찰 계수
        float restitution = 0.1f;       // 반발 계수 (탄성, 0=완전 비탄성, 1=완전 탄성)
    };

    class PhysicsMaterial
    {
    private:
        physx::PxMaterial* m_pxMaterial = nullptr;
        PhysicsMaterialDesc m_desc;
        bool m_isOwned = false;  // 직접 생성한 경우 true (소멸 시 release)

    public:
        PhysicsMaterial() = default;
        ~PhysicsMaterial();

        // PxMaterial 생성 (PhysicsSystem 초기화 후 호출)
        bool Create(const PhysicsMaterialDesc& desc);
        
        // 기존 PxMaterial 래핑 (기본 재질 등)
        void Wrap(physx::PxMaterial* pxMaterial);

        // 속성 변경
        void SetStaticFriction(float value);
        void SetDynamicFriction(float value);
        void SetRestitution(float value);

        // 속성 조회
        float GetStaticFriction() const { return m_desc.staticFriction; }
        float GetDynamicFriction() const { return m_desc.dynamicFriction; }
        float GetRestitution() const { return m_desc.restitution; }

        // PhysX 재질 접근
        physx::PxMaterial* GetPxMaterial() const { return m_pxMaterial; }

        // 유효성 확인
        bool IsValid() const { return m_pxMaterial != nullptr; }
    };

    // ═══════════════════════════════════════════════════════════════
    // 기본 재질 프리셋 (확장용)
    // ═══════════════════════════════════════════════════════════════

    namespace PhysicsMaterialPreset
    {
        inline PhysicsMaterialDesc Default()
        {
            return { 0.5f, 0.5f, 0.1f };
        }

        // 추가 프리셋은 필요 시 정의 가능:
        // inline PhysicsMaterialDesc Ice() { return { 0.05f, 0.02f, 0.1f }; }
        // inline PhysicsMaterialDesc Rubber() { return { 0.9f, 0.8f, 0.8f }; }
        // inline PhysicsMaterialDesc Metal() { return { 0.4f, 0.3f, 0.3f }; }
    }
}
