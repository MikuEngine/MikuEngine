#include "EnginePCH.h"
#include "CharacterController.h"

#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/System/SystemManager.h"
#include "Framework/Physics/PhysicsSystem.h"
#include "Framework/Physics/PhysicsUtility.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"

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

        // Scene에 등록
        if (Scene* scene = SceneManager::Get().GetScene())
        {
            scene->RegisterController(this);
        }
    }

    void CharacterController::OnDestroy()
    {
        // Scene 및 PxScene 존재 여부 확인
        Scene* scene = SceneManager::Get().GetScene();
        
        // Scene에서 해제
        if (scene)
        {
            scene->UnregisterController(this);
        }

        // Controller 해제
        // 주의: PxScene이 release되면 ControllerManager와 Controller도 자동 해제됨
        // PxScene이 이미 해제된 경우에는 수동 해제하면 안됨
        if (m_controller)
        {
            if (scene && scene->GetPxScene())
            {
                m_controller->release();
            }
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
        
        // 레이어 기반 필터링 설정
        // CharacterController는 모든 레이어와 충돌 (기본값)
        // 필요시 필터 쿼리 콜백을 통해 제어 가능

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
        
        // 컨트롤러의 Actor에 필터 데이터 설정
        if (m_controller)
        {
            physx::PxRigidDynamic* actor = m_controller->getActor();
            if (actor)
            {
                physx::PxU32 nbShapes = actor->getNbShapes();
                physx::PxShape* shapes[10];
                actor->getShapes(shapes, nbShapes);
                
                for (physx::PxU32 i = 0; i < nbShapes; ++i)
                {
                    physx::PxFilterData filterData;
                    filterData.word0 = m_layer;  // 자신의 레이어
                    filterData.word1 = PhysicsLayer::Mask::All;  // 모든 레이어와 충돌
                    filterData.word2 = 0;
                    filterData.word3 = 0;
                    
                    shapes[i]->setSimulationFilterData(filterData);
                    shapes[i]->setQueryFilterData(filterData);
                }
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════
    // 직렬화
    // ═══════════════════════════════════════════════════════════════

    void CharacterController::OnGui()
    {
        // 레이어 선택
        static const char* layerNames[] = {
            "Default", "Player", "Enemy", "Projectile", "Environment", "Trigger",
            "EnemyProjectile", "Picking", "Field", "Wall", "EnemyParabolicProjectile", "ExplosionTrigger",
            "JumpingEnemy", "RadiusChecker", "SplittingEnemy", "BossBulletProjectile",
            "BossBigProjectile", "Boss", "BBP_Reflected", "SubWall", "EE_DamageTrigger",
            "EnemyProjectileCurve", "EnemyNonPath", "Layer23", "Layer24", "Layer25",
            "Layer26", "Layer27", "Layer28", "Layer29", "Layer30", "Layer31"
        };
        
        int currentLayer = static_cast<int>(m_layer);
        if (currentLayer >= IM_ARRAYSIZE(layerNames)) currentLayer = 0;
        
        if (ImGui::Combo("Physics Layer", &currentLayer, layerNames, IM_ARRAYSIZE(layerNames)))
        {
            SetLayer(static_cast<uint32_t>(currentLayer));
        }

        ImGui::Separator();

        // 형태 설정
        if (ImGui::TreeNode("Shape"))
        {
            float height = m_height;
            if (ImGui::DragFloat("Height", &height, 0.01f, m_radius * 2.0f + 0.01f, 10.0f))
            {
                SetHeight(height);
            }

            float radius = m_radius;
            if (ImGui::DragFloat("Radius", &radius, 0.01f, 0.01f, 5.0f))
            {
                SetRadius(radius);
            }

            float skinWidth = m_skinWidth;
            if (ImGui::DragFloat("Skin Width", &skinWidth, 0.001f, 0.0f, 1.0f))
            {
                SetSkinWidth(skinWidth);
                // Skin Width 변경 시 컨트롤러 재생성 필요
                if (m_controller)
                {
                    // 컨트롤러 재생성
                    OnDestroy();
                    Awake();
                }
            }

            ImGui::TreePop();
        }

        // 이동 설정
        if (ImGui::TreeNode("Movement"))
        {
            float stepOffset = m_stepOffset;
            if (ImGui::DragFloat("Step Offset", &stepOffset, 0.01f, 0.0f, 2.0f))
            {
                SetStepOffset(stepOffset);
            }

            float slopeLimit = m_slopeLimit;
            if (ImGui::DragFloat("Slope Limit (degrees)", &slopeLimit, 1.0f, 0.0f, 90.0f))
            {
                SetSlopeLimit(slopeLimit);
            }

            ImGui::TreePop();
        }

        // 오프셋
        Vector3 center = m_center;
        if (ImGui::DragFloat3("Center", &center.x, 0.01f))
        {
            SetCenter(center);
        }

        ImGui::Separator();

        // 상태 표시 (읽기 전용)
        if (ImGui::TreeNode("Status"))
        {
            ImGui::Text("Grounded: %s", m_isGrounded ? "Yes" : "No");
            
            std::string collisionInfo = "None";
            if (m_lastCollisionFlags & ControllerCollisionFlags::Sides) collisionInfo += " Sides";
            if (m_lastCollisionFlags & ControllerCollisionFlags::Above) collisionInfo += " Above";
            if (m_lastCollisionFlags & ControllerCollisionFlags::Below) collisionInfo += " Below";
            ImGui::Text("Collision: %s", collisionInfo.c_str());

            Vector3 velocity = m_velocity;
            ImGui::Text("Velocity: (%.2f, %.2f, %.2f)", velocity.x, velocity.y, velocity.z);

            if (m_controller)
            {
                physx::PxExtendedVec3 pos = m_controller->getPosition();
                ImGui::Text("Controller Pos: (%.2f, %.2f, %.2f)", 
                    static_cast<float>(pos.x), static_cast<float>(pos.y), static_cast<float>(pos.z));
            }

            ImGui::TreePop();
        }
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

        physx::PxControllerManager* manager = SystemManager::Get().GetPhysicsSystem().GetControllerManager(scene);
        if (!manager)
        {
            return;
        }

        physx::PxMaterial* material = SystemManager::Get().GetPhysicsSystem().GetDefaultMaterial();
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
                
                // 필터 데이터 설정
                physx::PxU32 nbShapes = actor->getNbShapes();
                physx::PxShape* shapes[10];
                actor->getShapes(shapes, nbShapes);
                
                for (physx::PxU32 i = 0; i < nbShapes; ++i)
                {
                    physx::PxFilterData filterData;
                    filterData.word0 = m_layer;  // 자신의 레이어
                    filterData.word1 = PhysicsLayer::Mask::All;  // 모든 레이어와 충돌
                    filterData.word2 = 0;
                    filterData.word3 = 0;
                    
                    shapes[i]->setSimulationFilterData(filterData);
                    shapes[i]->setQueryFilterData(filterData);
                }
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
        Transform* transform = GetTransform();
        Transform* parent = transform->GetParent();
        
        if (parent)
        {
            // 부모가 있는 경우: 월드 좌표를 로컬 좌표로 변환
            Matrix parentWorld = parent->GetWorld();
            Matrix parentWorldInv = parentWorld.Invert();
            Vector3 localPos = Vector3::Transform(worldPos, parentWorldInv);
            transform->SetLocalPosition(localPos);
        }
        else
        {
            // 부모가 없는 경우: 월드 좌표 = 로컬 좌표
            transform->SetLocalPosition(worldPos);
        }
    }
}
