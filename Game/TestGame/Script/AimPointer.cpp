#include "GamePCH.h"
#include "AimPointer.h"

namespace game
{
    void AimPointer::Start()
    {
        LOG_PRINT("[AimPointer] Started");
    }

    void AimPointer::Update()
    {
        // 마우스 스크린 좌표 가져오기
        engine::Vector2 mousePos = engine::Input::GetMousePosition();
        LOG_PRINT("{} , {}", mousePos.x, mousePos.y);
        // 탑다운 뷰: 스크린 좌표를 월드 좌표로 변환 (단순화된 버전)
        // TODO: 카메라 뷰/프로젝션 행렬을 사용한 정확한 변환 필요
        // 현재는 대략적인 스케일로 변환 (테스트용)
        
        // 화면 중앙을 (0, 0)으로, 스케일 조정
        float screenWidth = 1280.0f;   // 임시 값
        float screenHeight = 720.0f;   // 임시 값
        float worldScale = 0.02f;      // 스크린 → 월드 변환 스케일 (조정 필요)
        
        m_worldPosition.x = (mousePos.x - screenWidth * 0.5f) * worldScale;
        m_worldPosition.y = -(mousePos.y - screenHeight * 0.5f) * worldScale;  // Y축 반전
        m_worldPosition.z = 0.0f;  // 탑다운이므로 Z=0 평면
        
        // 에임포인터 오브젝트 위치 업데이트
        GetTransform()->SetLocalPosition(m_worldPosition);
    }

    engine::Vector3 AimPointer::GetDirectionFrom(const engine::Vector3& fromPosition) const
    {
        engine::Vector3 direction = m_worldPosition - fromPosition;
        direction.z = 0.0f;  // 2D 평면에서의 방향
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
