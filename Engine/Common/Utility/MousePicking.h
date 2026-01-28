#pragma once

#include <vector>

namespace engine
{
    class GameObject;
    class Camera;
    struct RaycastHit;

    // ═══════════════════════════════════════════════════════════════
    // MousePicking - 마우스 피킹 헬퍼 유틸리티
    // 
    // 기능:
    //   - 마우스 커서 위치의 3D 오브젝트 선택
    //   - 스크린 좌표 → 월드 레이 변환 (카메라 기반)
    //   - 레이캐스트를 통한 오브젝트 감지
    // 
    // 사용 예시:
    //   // 마우스 아래의 첫 번째 오브젝트 가져오기
    //   GameObject* obj = MousePicking::GetObjectUnderMouse();
    //   
    //   // 특정 레이어만 검사
    //   GameObject* obj = MousePicking::GetObjectUnderMouse(PhysicsLayer::Mask::Enemy);
    //   
    //   // 마우스 아래의 모든 오브젝트 가져오기
    //   auto objects = MousePicking::GetAllObjectsUnderMouse();
    //   
    //   // 특정 컴포넌트를 가진 오브젝트만 가져오기
    //   MonsterScript* monster = MousePicking::GetComponentUnderMouse<MonsterScript>();
    // ═══════════════════════════════════════════════════════════════

    namespace MousePicking
    {
        // ═══════════════════════════════════════
        // 설정
        // ═══════════════════════════════════════

        // 기본 레이캐스트 최대 거리 설정
        void SetDefaultMaxDistance(float distance);
        float GetDefaultMaxDistance();

        // ═══════════════════════════════════════
        // 기본 피킹 함수
        // ═══════════════════════════════════════

        // 마우스 아래의 첫 번째 오브젝트 반환 (가장 가까운 것)
        // layerMask: 검사할 물리 레이어 마스크 (기본: 모든 레이어)
        // maxDistance: 레이캐스트 최대 거리 (0이면 기본값 사용)
        // camera: 사용할 카메라 (nullptr이면 MainCamera 자동 검색)
        GameObject* GetObjectUnderMouse(
            uint32_t layerMask = 0xFFFFFFFF,
            float maxDistance = 0.0f,
            Camera* camera = nullptr
        );

        // 마우스 아래의 모든 오브젝트 반환
        std::vector<GameObject*> GetAllObjectsUnderMouse(
            uint32_t layerMask = 0xFFFFFFFF,
            float maxDistance = 0.0f,
            Camera* camera = nullptr
        );

        // 상세 정보 포함 (히트 포인트, 노말, 거리 등)
        bool GetObjectUnderMouseDetailed(
            RaycastHit& outHit,
            uint32_t layerMask = 0xFFFFFFFF,
            float maxDistance = 0.0f,
            Camera* camera = nullptr
        );

        bool GetAllObjectsUnderMouseDetailed(
            std::vector<RaycastHit>& outHits,
            uint32_t layerMask = 0xFFFFFFFF,
            float maxDistance = 0.0f,
            Camera* camera = nullptr
        );

        // ═══════════════════════════════════════
        // 컴포넌트 기반 피킹
        // ═══════════════════════════════════════

        // 특정 컴포넌트를 가진 오브젝트의 컴포넌트 반환
        template<typename T>
        T* GetComponentUnderMouse(
            uint32_t layerMask = 0xFFFFFFFF,
            float maxDistance = 0.0f,
            Camera* camera = nullptr
        );

        // 특정 컴포넌트를 가진 모든 오브젝트의 컴포넌트 반환
        template<typename T>
        std::vector<T*> GetAllComponentsUnderMouse(
            uint32_t layerMask = 0xFFFFFFFF,
            float maxDistance = 0.0f,
            Camera* camera = nullptr
        );

        // ═══════════════════════════════════════
        // 스크린 좌표 기반 피킹 (특정 좌표 지정)
        // ═══════════════════════════════════════

        // 지정된 스크린 좌표의 오브젝트 반환
        GameObject* GetObjectAtScreenPosition(
            const Vector2& screenPos,
            uint32_t layerMask = 0xFFFFFFFF,
            float maxDistance = 0.0f,
            Camera* camera = nullptr
        );

        std::vector<GameObject*> GetAllObjectsAtScreenPosition(
            const Vector2& screenPos,
            uint32_t layerMask = 0xFFFFFFFF,
            float maxDistance = 0.0f,
            Camera* camera = nullptr
        );

        // ═══════════════════════════════════════
        // 유틸리티
        // ═══════════════════════════════════════

        // 현재 마우스 위치에서의 월드 레이 가져오기
        bool GetMouseRay(
            Vector3& outOrigin,
            Vector3& outDirection,
            Camera* camera = nullptr
        );

        // 지정된 스크린 좌표에서의 월드 레이 가져오기
        bool GetScreenRay(
            const Vector2& screenPos,
            Vector3& outOrigin,
            Vector3& outDirection,
            Camera* camera = nullptr
        );

        // MainCamera 가져오기 (내부 캐싱)
        Camera* GetMainCamera();
    }
}

// ═══════════════════════════════════════════════════════════════
// 템플릿 구현
// ═══════════════════════════════════════════════════════════════

#include "Framework/Object/GameObject/GameObject.h"

namespace engine
{
    namespace MousePicking
    {
        template<typename T>
        T* GetComponentUnderMouse(uint32_t layerMask, float maxDistance, Camera* camera)
        {
            GameObject* obj = GetObjectUnderMouse(layerMask, maxDistance, camera);
            if (obj)
            {
                return obj->GetComponent<T>();
            }
            return nullptr;
        }

        template<typename T>
        std::vector<T*> GetAllComponentsUnderMouse(uint32_t layerMask, float maxDistance, Camera* camera)
        {
            std::vector<T*> result;
            std::vector<GameObject*> objects = GetAllObjectsUnderMouse(layerMask, maxDistance, camera);
            
            for (GameObject* obj : objects)
            {
                if (T* component = obj->GetComponent<T>())
                {
                    result.push_back(component);
                }
            }
            
            return result;
        }
    }
}
