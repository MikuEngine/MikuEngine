#include "EnginePCH.h"
#include "PhysicsUtility.h"

#include "Framework/Object/Component/Transform.h"

namespace engine
{
    namespace PhysicsUtility
    {
        physx::PxTransform ToPxTransform(const Transform* transform)
        {
            if (!transform)
            {
                return physx::PxTransform(physx::PxIdentity);
            }

            // 월드 위치와 월드 회전 사용
            Transform* nonConstTransform = const_cast<Transform*>(transform);
            Vector3 worldPos = nonConstTransform->GetWorldPosition();
            Quaternion worldRot = nonConstTransform->GetWorldRotation();

            return ToPxTransform(worldPos, worldRot);
        }

        void ApplyPxTransformToTransform(const physx::PxTransform& pxTransform, Transform* transform)
        {
            if (!transform)
            {
                return;
            }

            Vector3 worldPosition = ToVector3(pxTransform.p);
            Quaternion worldRotation = ToQuaternion(pxTransform.q);

            // 부모가 있는 경우 로컬 좌표로 변환
            Transform* parent = transform->GetParent();
            if (parent)
            {
                // 부모의 월드 역행렬 사용
                Matrix parentWorldInverse = parent->GetWorld().Invert();
                
                // 월드 위치를 로컬로 변환
                Vector3 localPosition = Vector3::Transform(worldPosition, parentWorldInverse);
                transform->SetLocalPosition(localPosition);
                
                // 월드 회전을 로컬로 변환
                Quaternion parentWorldRot = parent->GetWorldRotation();
                Quaternion invParentRot = parentWorldRot;
                invParentRot.Conjugate();
                Quaternion localRotation = invParentRot * worldRotation;
                localRotation.Normalize();
                transform->SetLocalRotation(localRotation);
            }
            else
            {
                // 부모 없음: 월드 = 로컬
                transform->SetLocalPosition(worldPosition);
                transform->SetLocalRotation(worldRotation);
            }
        }
    }
}
