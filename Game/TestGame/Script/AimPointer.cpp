#include "GamePCH.h"
#include "AimPointer.h"

#include <Framework/Scene/SceneManager.h>
#include <Framework/Scene/Scene.h>

namespace game
{
    void AimPointer::Start()
    {
        LOG_PRINT("[AimPointer] Started");
    }

    void AimPointer::Update()
    {
        auto* scene = engine::SceneManager::Get().GetScene();
        if (scene)
        {
            if (auto* camPos = scene->FindGameObject("MainCamera"))
            {
                m_worldPosition = camPos->GetTransform()->GetWorldPosition();
                m_worldPosition.z += 0.2f;
            }
        }

        // 마우스 스크린 좌표 가져오기
        engine::Vector2 mousePos = engine::Input::GetMousePosition();
        LOG_PRINT("{} , {}", mousePos.x, mousePos.y);
        //// 탑다운 뷰: 스크린 좌표를 월드 좌표로 변환 (단순화된 버전)
        //// TODO: 카메라 뷰/프로젝션 행렬을 사용한 정확한 변환 필요
        //// 현재는 대략적인 스케일로 변환 (테스트용)
        //
        //// 화면 중앙을 (0, 0)으로, 스케일 조정
        float screenWidth = 1920;   // 임시 값
        float screenHeight = 1080.0f;   // 임시 값
        float worldScaleX = 0.02f;      // 스크린 → 월드 변환 스케일 (조정 필요)
        float worldScaleY = 0.02f; 

        worldScaleX = 0.32f / screenWidth;
        worldScaleY = 0.18f / screenHeight;

        m_worldPosition.x += (mousePos.x - screenWidth * 0.5f) * worldScaleX;
        m_worldPosition.y += -(mousePos.y - screenHeight * 0.5f) * worldScaleY;  
        //m_worldPosition.z = -19.8f;  // 탑다운이므로 Z=0 평면
        
        LOG_PRINT("{} , {}", m_worldPosition.x, m_worldPosition.y);
        // 에임포인터 오브젝트 위치 업데이트
                
        GetTransform()->SetLocalPosition(m_worldPosition);
    }

    engine::Vector3 AimPointer::GetDirectionFrom(const engine::Vector3& fromPosition) const
    {
        engine::Vector3 direction = m_worldPosition - fromPosition;
        direction.z = 0.0f;  // 2D 평면에서의 방향
        //direction.z = 19.8f;
        direction.Normalize();
        return direction;
    }

    void AimPointer::OnGui()
    {
        ImGui::Text("World Position: (%.2f, %.2f, %.2f)", 
            m_worldPosition.x, m_worldPosition.y, m_worldPosition.z);
    }

    void AimPointer::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void AimPointer::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    std::string AimPointer::GetType() const
    {
        return "AimPointer";
    }
}
