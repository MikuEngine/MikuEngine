#include "EnginePCH.h"
#include "PhysicsMaterial.h"

#include "Framework/System/SystemManager.h"
#include "Framework/Physics/PhysicsSystem.h"

namespace engine
{
    PhysicsMaterial::~PhysicsMaterial()
    {
        if (m_pxMaterial && m_isOwned)
        {
            m_pxMaterial->release();
            m_pxMaterial = nullptr;
        }
    }

    bool PhysicsMaterial::Create(const PhysicsMaterialDesc& desc)
    {
        // 기존 재질이 있으면 해제
        if (m_pxMaterial && m_isOwned)
        {
            m_pxMaterial->release();
            m_pxMaterial = nullptr;
        }

        m_desc = desc;

        physx::PxPhysics* physics = SystemManager::Get().GetPhysicsSystem().GetPxPhysics();
        if (!physics)
        {
            return false;
        }

        m_pxMaterial = physics->createMaterial(
            desc.staticFriction,
            desc.dynamicFriction,
            desc.restitution
        );

        m_isOwned = true;
        return m_pxMaterial != nullptr;
    }

    void PhysicsMaterial::Wrap(physx::PxMaterial* pxMaterial)
    {
        // 기존 소유 재질 해제
        if (m_pxMaterial && m_isOwned)
        {
            m_pxMaterial->release();
        }

        m_pxMaterial = pxMaterial;
        m_isOwned = false;

        if (m_pxMaterial)
        {
            m_desc.staticFriction = m_pxMaterial->getStaticFriction();
            m_desc.dynamicFriction = m_pxMaterial->getDynamicFriction();
            m_desc.restitution = m_pxMaterial->getRestitution();
        }
    }

    void PhysicsMaterial::SetStaticFriction(float value)
    {
        m_desc.staticFriction = value;
        if (m_pxMaterial)
        {
            m_pxMaterial->setStaticFriction(value);
        }
    }

    void PhysicsMaterial::SetDynamicFriction(float value)
    {
        m_desc.dynamicFriction = value;
        if (m_pxMaterial)
        {
            m_pxMaterial->setDynamicFriction(value);
        }
    }

    void PhysicsMaterial::SetRestitution(float value)
    {
        m_desc.restitution = value;
        if (m_pxMaterial)
        {
            m_pxMaterial->setRestitution(value);
        }
    }
}
