#include "EnginePCH.h"
#include "MousePicking.h"

#include "Core/System/Input.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Camera.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"
#include "Framework/System/SystemManager.h"
#include "Framework/Physics/PhysicsSystem.h"
#include "Framework/Physics/CollisionTypes.h"

namespace engine
{
    namespace MousePicking
    {
        // ═══════════════════════════════════════
        // 내부 상태
        // ═══════════════════════════════════════

        static float s_defaultMaxDistance = 10000.0f;

        // ═══════════════════════════════════════
        // 설정
        // ═══════════════════════════════════════

        void SetDefaultMaxDistance(float distance)
        {
            s_defaultMaxDistance = distance;
        }

        float GetDefaultMaxDistance()
        {
            return s_defaultMaxDistance;
        }

        // ═══════════════════════════════════════
        // 유틸리티 함수
        // ═══════════════════════════════════════

        Camera* GetMainCamera()
        {
            // MainCamera 검색
            if (auto* camGO = GameObject::Find("MainCamera"))
            {
                return camGO->GetComponent<Camera>();
            }
            return nullptr;
        }

        bool GetMouseRay(Vector3& outOrigin, Vector3& outDirection, Camera* camera)
        {
            Vector2 mousePos = Input::GetMousePosition();
            return GetScreenRay(mousePos, outOrigin, outDirection, camera);
        }

        bool GetScreenRay(const Vector2& screenPos, Vector3& outOrigin, Vector3& outDirection, Camera* camera)
        {
            // 카메라 확인
            Camera* cam = camera ? camera : GetMainCamera();
            if (!cam)
            {
                return false;
            }

            return cam->ScreenToWorldRay(screenPos, outOrigin, outDirection);
        }

        // ═══════════════════════════════════════
        // 기본 피킹 함수
        // ═══════════════════════════════════════

        GameObject* GetObjectUnderMouse(uint32_t layerMask, float maxDistance, Camera* camera)
        {
            RaycastHit hit;
            if (GetObjectUnderMouseDetailed(hit, layerMask, maxDistance, camera))
            {
                return hit.gameObject.Get();
            }
            return nullptr;
        }

        std::vector<GameObject*> GetAllObjectsUnderMouse(uint32_t layerMask, float maxDistance, Camera* camera)
        {
            std::vector<GameObject*> result;
            std::vector<RaycastHit> hits;
            
            if (GetAllObjectsUnderMouseDetailed(hits, layerMask, maxDistance, camera))
            {
                for (const auto& hit : hits)
                {
                    if (hit.gameObject)
                    {
                        result.push_back(hit.gameObject.Get());
                    }
                }
            }
            
            return result;
        }

        bool GetObjectUnderMouseDetailed(RaycastHit& outHit, uint32_t layerMask, float maxDistance, Camera* camera)
        {
            Vector2 mousePos = Input::GetMousePosition();
            
            // 레이 계산
            Vector3 rayOrigin, rayDirection;
            if (!GetScreenRay(mousePos, rayOrigin, rayDirection, camera))
            {
                return false;
            }

            // 물리 시스템 가져오기
            PhysicsSystem& physicsSystem = SystemManager::Get().GetPhysicsSystem();

            // 레이캐스트 수행
            float distance = (maxDistance > 0.0f) ? maxDistance : s_defaultMaxDistance;
            return physicsSystem.Raycast(rayOrigin, rayDirection, distance, outHit, layerMask);
        }

        bool GetAllObjectsUnderMouseDetailed(std::vector<RaycastHit>& outHits, uint32_t layerMask, float maxDistance, Camera* camera)
        {
            Vector2 mousePos = Input::GetMousePosition();
            
            // 레이 계산
            Vector3 rayOrigin, rayDirection;
            if (!GetScreenRay(mousePos, rayOrigin, rayDirection, camera))
            {
                return false;
            }

            // 물리 시스템 가져오기
            PhysicsSystem& physicsSystem = SystemManager::Get().GetPhysicsSystem();

            // 레이캐스트 수행
            float distance = (maxDistance > 0.0f) ? maxDistance : s_defaultMaxDistance;
            return physicsSystem.RaycastAll(rayOrigin, rayDirection, distance, outHits, layerMask);
        }

        // ═══════════════════════════════════════
        // 스크린 좌표 기반 피킹
        // ═══════════════════════════════════════

        GameObject* GetObjectAtScreenPosition(const Vector2& screenPos, uint32_t layerMask, float maxDistance, Camera* camera)
        {
            // 레이 계산
            Vector3 rayOrigin, rayDirection;
            if (!GetScreenRay(screenPos, rayOrigin, rayDirection, camera))
            {
                return nullptr;
            }

            // 물리 시스템 가져오기
            PhysicsSystem& physicsSystem = SystemManager::Get().GetPhysicsSystem();

            // 레이캐스트 수행
            float distance = (maxDistance > 0.0f) ? maxDistance : s_defaultMaxDistance;
            RaycastHit hit;
            if (physicsSystem.Raycast(rayOrigin, rayDirection, distance, hit, layerMask))
            {
                return hit.gameObject.Get();
            }

            return nullptr;
        }

        std::vector<GameObject*> GetAllObjectsAtScreenPosition(const Vector2& screenPos, uint32_t layerMask, float maxDistance, Camera* camera)
        {
            std::vector<GameObject*> result;

            // 레이 계산
            Vector3 rayOrigin, rayDirection;
            if (!GetScreenRay(screenPos, rayOrigin, rayDirection, camera))
            {
                return result;
            }

            // 물리 시스템 가져오기
            PhysicsSystem& physicsSystem = SystemManager::Get().GetPhysicsSystem();

            // 레이캐스트 수행
            float distance = (maxDistance > 0.0f) ? maxDistance : s_defaultMaxDistance;
            std::vector<RaycastHit> hits;
            if (physicsSystem.RaycastAll(rayOrigin, rayDirection, distance, hits, layerMask))
            {
                for (const auto& hit : hits)
                {
                    if (hit.gameObject)
                    {
                        result.push_back(hit.gameObject.Get());
                    }
                }
            }

            return result;
        }
    }
}
