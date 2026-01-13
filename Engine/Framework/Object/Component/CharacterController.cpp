#include "EnginePCH.h"
#include "CharacterController.h"

#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Physics/PhysicsSystem.h"
#include "Framework/Physics/PhysicsUtility.h"
#include "Framework/Scene/SceneManager.h"

namespace engine
{
    CharacterController::~CharacterController()
    {
        // OnDestroy에서 정리됨
    }

    void CharacterController::Initialize()
    {
        Component::Initialize();
    }

    void CharacterController::Awake()
    {
        Component::Awake();

        CreateController();

        if (!m_controller)
        {
            LOG_ERROR("[CharacterController] Failed to create controller");
            return;
        }

        // PhysicsSystem에 등록
        PhysicsSystem::Get().RegisterController(this);
    }

    void CharacterController::OnDestroy()
    {
        // PhysicsSystem에서 해제
        PhysicsSystem::Get().UnregisterController(this);

        // Controller 해제
        if (m_controller)
        {
            m_controller->release();
            m_controller = nullptr;
        }

        Component::OnDestroy();
    }

    // ═══════════════════════════════════════════════════════════════
    // 이동
    // ═══════════════════════════════════════════════════════════════

    ControllerCollisionFlags CharacterController::Move(const Vector3& motion, float deltaTime)
    {
        if (!m_controller)
        {
            return ControllerCollisionFlags::None;
        }

        // 속도 기반 이동 추가
        Vector3 totalMotion = motion + m_velocity * deltaTime;

        // PhysX 좌표계로 변환
        physx::PxVec3 pxMotion = PhysicsUtility::ToPxVec3(totalMotion);

        // 필터 설정
        physx::PxControllerFilters filters;
        // TODO: 레이어 기반 필터링

        // 이동 수행
        physx::PxControllerCollisionFlags flags = m_controller->move(
            pxMotion,
            m_minMoveDistance,
            deltaTime,
            filters
        );

        // 충돌 플래그 변환
        m_lastCollisionFlags = ControllerCollisionFlags::None;
        
        if (flags & physx::PxControllerCollisionFlag::eCOLLISION_SIDES)
        {
            m_lastCollisionFlags = m_lastCollisionFlags | ControllerCollisionFlags::Sides;
        }
        if (flags & physx::PxControllerCollisionFlag::eCOLLISION_UP)
        {
            m_lastCollisionFlags = m_lastCollisionFlags | ControllerCollisionFlags::Above;
        }
        if (flags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN)
        {
            m_lastCollisionFlags = m_lastCollisionFlags | ControllerCollisionFlags::Below;
        }

        // 착지 상태 갱신
        m_isGrounded = (m_lastCollisionFlags & ControllerCollisionFlags::Below);

        // 천장 충돌 시 수직 속도 리셋
        if (m_lastCollisionFlags & ControllerCollisionFlags::Above)
        {
            if (m_velocity.y > 0)
            {
                m_velocity.y = 0;
            }
        }

        // Transform 동기화
        SyncTransformFromController();

        // 속도 감쇠 (간단한 마찰)
        m_velocity.x *= (1.0f - 3.0f * deltaTime);
        m_velocity.z *= (1.0f - 3.0f * deltaTime);

        return m_lastCollisionFlags;
    }

    void CharacterController::SetPosition(const Vector3& position)
    {
        if (!m_controller)
        {
            return;
        }

        physx::PxExtendedVec3 pxPos = PhysicsUtility::ToPxExtendedVec3(position + m_center);
        m_controller->setPosition(pxPos);

        SyncTransformFromController();
    }

    Vector3 CharacterController::GetPosition() const
    {
        if (!m_controller)
        {
            return GetTransform()->GetWorldPosition();
        }

        physx::PxExtendedVec3 pxPos = m_controller->getPosition();
        return PhysicsUtility::FromPxExtendedVec3(pxPos) - m_center;
    }

    // ═══════════════════════════════════════════════════════════════
    // 형태 설정
    // ═══════════════════════════════════════════════════════════════

    void CharacterController::SetHeight(float height)
    {
        m_height = std::max(m_radius * 2.0f + 0.01f, height);

        if (m_controller)
        {
            // 캡슐 컨트롤러의 높이 변경
            physx::PxCapsuleController* capsule = static_cast<physx::PxCapsuleController*>(m_controller);
            if (capsule)
            {
                float halfHeight = (m_height - m_radius * 2.0f) * 0.5f;
                capsule->setHeight(halfHeight * 2.0f);
            }
        }
    }

    void CharacterController::SetRadius(float radius)
    {
        m_radius = std::max(0.01f, radius);

        if (m_controller)
        {
            physx::PxCapsuleController* capsule = static_cast<physx::PxCapsuleController*>(m_controller);
            if (capsule)
            {
                capsule->setRadius(m_radius);
            }
        }
    }

    void CharacterController::SetSkinWidth(float width)
    {
        m_skinWidth = std::max(0.0f, width);
        // 런타임 변경은 컨트롤러 재생성 필요
    }

    void CharacterController::SetStepOffset(float offset)
    {
        m_stepOffset = std::max(0.0f, offset);

        if (m_controller)
        {
            m_controller->setStepOffset(m_stepOffset);
        }
    }

    void CharacterController::SetSlopeLimit(float limitDegrees)
    {
        m_slopeLimit = std::clamp(limitDegrees, 0.0f, 90.0f);

        if (m_controller)
        {
            m_controller->setSlopeLimit(cosf(DirectX::XMConvertToRadians(m_slopeLimit)));
        }
    }

    void CharacterController::SetCenter(const Vector3& center)
    {
        m_center = center;
        // 컨트롤러 위치 갱신
        if (m_controller)
        {
            Vector3 worldPos = GetTransform()->GetWorldPosition();
            SetPosition(worldPos);
        }
    }

    void CharacterController::SetLayer(uint32_t layer)
    {
        m_layer = layer;
        // TODO: 컨트롤러 필터 데이터 갱신
    }

    // ═══════════════════════════════════════════════════════════════
    // 직렬화
    // ═══════════════════════════════════════════════════════════════

    void CharacterController::OnGui()
    {
        // TODO: ImGui 편집
    }

    void CharacterController::Save(json& j) const
    {
        Object::Save(j);  // Type 필드 저장
        j["height"] = m_height;
        j["radius"] = m_radius;
        j["skinWidth"] = m_skinWidth;
        j["stepOffset"] = m_stepOffset;
        j["slopeLimit"] = m_slopeLimit;
        j["layer"] = m_layer;
        j["center"] = { m_center.x, m_center.y, m_center.z };
    }

    void CharacterController::Load(const json& j)
    {
        if (j.contains("height")) m_height = j["height"].get<float>();
        if (j.contains("radius")) m_radius = j["radius"].get<float>();
        if (j.contains("skinWidth")) m_skinWidth = j["skinWidth"].get<float>();
        if (j.contains("stepOffset")) m_stepOffset = j["stepOffset"].get<float>();
        if (j.contains("slopeLimit")) m_slopeLimit = j["slopeLimit"].get<float>();
        if (j.contains("layer")) m_layer = j["layer"].get<uint32_t>();
        if (j.contains("center"))
        {
            auto& c = j["center"];
            m_center = Vector3(c[0].get<float>(), c[1].get<float>(), c[2].get<float>());
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // Private
    // ═══════════════════════════════════════════════════════════════

    void CharacterController::CreateController()
    {
        Scene* scene = SceneManager::Get().GetScene();
        if (!scene)
        {
            return;
        }

        physx::PxControllerManager* manager = PhysicsSystem::Get().GetControllerManager(scene);
        if (!manager)
        {
            return;
        }

        physx::PxMaterial* material = PhysicsSystem::Get().GetDefaultMaterial();
        if (!material)
        {
            return;
        }

        // 초기 위치
        Vector3 worldPos = GetTransform()->GetWorldPosition();
        physx::PxExtendedVec3 pxPos = PhysicsUtility::ToPxExtendedVec3(worldPos + m_center);

        // 캡슐 컨트롤러 설정
        physx::PxCapsuleControllerDesc desc;
        desc.height = m_height - m_radius * 2.0f;  // 실린더 부분 높이
        desc.radius = m_radius;
        desc.stepOffset = m_stepOffset;
        desc.slopeLimit = cosf(DirectX::XMConvertToRadians(m_slopeLimit));
        desc.contactOffset = m_skinWidth;
        desc.material = material;
        desc.position = pxPos;
        desc.upDirection = physx::PxVec3(0, 1, 0);
        desc.userData = this;

        // 컨트롤러 생성
        m_controller = manager->createController(desc);

        if (m_controller)
        {
            // Actor의 userData 설정
            physx::PxRigidDynamic* actor = m_controller->getActor();
            if (actor)
            {
                actor->userData = this;
            }
        }
    }

    void CharacterController::SyncTransformFromController()
    {
        if (!m_controller)
        {
            return;
        }

        physx::PxExtendedVec3 pxPos = m_controller->getPosition();
        Vector3 worldPos = PhysicsUtility::FromPxExtendedVec3(pxPos) - m_center;

        // Transform에 적용
        // TODO: 부모가 있는 경우 로컬 좌표로 변환
        GetTransform()->SetLocalPosition(worldPos);
    }
}
