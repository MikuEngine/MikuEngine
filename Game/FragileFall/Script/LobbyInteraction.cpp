#include "GamePCH.h"
#include "LobbyInteraction.h"
#include <Framework/Object/Component/UI/UIClickArea.h>

namespace game
{
    void LobbyInteraction::Awake()
    {
        m_player = engine::GameObject::Find("PlayerPreview");
        auto* go = GetGameObject();
        if (go) m_clickArea = go->GetComponent<engine::UIClickArea>();
    }

    void LobbyInteraction::Start()
    {
        if (!m_clickArea || !m_player) return;

        const auto& rot = m_player->GetTransform()->GetLocalEulerAngles();
        m_yawDeg = rot.y;

        m_clickArea->AddOnDrag([self = engine::Ptr<LobbyInteraction>(this)](const engine::Vector2& /*pos*/, const engine::Vector2& delta, int mouseButton)
            {
                if (!self) return;
                if (mouseButton != 0) return;

                self->Interact(delta);
            });

        m_camera = engine::GameObject::Find("MainCamera");
        if (m_camera && m_player)
        {
            engine::Vector3 camPos = m_camera->GetTransform()->GetLocalPosition();
            engine::Vector3 playerPos = m_player->GetTransform()->GetLocalPosition();

            // 1. 기준점 설정
            engine::Vector3 targetPos = playerPos;
            targetPos.y += 1.5f;

            // 2. 타겟에서 카메라로 향하는 벡터 계산
            engine::Vector3 offset = camPos - targetPos;

            // 3. 현재 거리 저장
            m_currentDistance = offset.Length();

            // 4. 방향만 따로 추출 (정규화) - 이제 이 방향으로만 움직입니다.
            if (m_currentDistance > 0.0001f)
            {
                m_dirFromTarget.x = offset.x / m_currentDistance;
                m_dirFromTarget.y = offset.y / m_currentDistance;
                m_dirFromTarget.z = offset.z / m_currentDistance;
            }
        }
    }

    void LobbyInteraction::Update()
    {
        if (!m_isActive) return;

        m_wheelDelta = engine::Input::GetMouseWheelNotch();

        HandleZoom(m_wheelDelta);

        if (m_camera && m_player)
        {
            // 캐릭터의 중심점 (피벗) 설정
            engine::Vector3 targetPos = m_player->GetTransform()->GetLocalPosition();
            targetPos.y += 1.5f; // 캐릭터의 가슴 높이 정도를 바라보게 설정

            // 새 위치 = 타겟 * 거리
            engine::Vector3 newPos = targetPos + (m_dirFromTarget * m_currentDistance);

            engine::Vector3 smoothedPos = engine::Vector3::Lerp(m_camera->GetTransform()->GetLocalPosition(), newPos, 0.1f);
            m_camera->GetTransform()->SetLocalPosition(smoothedPos);
        }
    }

    void LobbyInteraction::OnGui()
    {

    }

    void LobbyInteraction::Save(engine::json& j) const
    {
        Object::Save(j);
    }

    void LobbyInteraction::Load(const engine::json& j)
    {
        Object::Load(j);
    }

    void LobbyInteraction::SetInteractionActive(bool active)
    {
        m_isActive = active;
    }

    void LobbyInteraction::Interact(const engine::Vector2& delta)
    {
        if (!m_isActive) return;

        float degPerPixel = 0.5f;

        m_yawDeg -= delta.x * degPerPixel;

        m_player->GetTransform()->SetLocalRotation({ 0.0f, m_yawDeg, 0.0f });
    }

    void LobbyInteraction::HandleZoom(float wheelDelta)
    {
        const float zoomSpeed = 0.5f;
        const float minDistance = 2.0f; // 캐릭터 코앞
        const float maxDistance = 6.0f; // 캐릭터 전체샷

        // 2. 거리 값 갱신 (wheelDelta는 보통 120, -120 단위나 1, -1 단위)
        m_currentDistance -= wheelDelta * zoomSpeed;

        // 3. 최소/최대 거리 제한 (Clamp)
        m_currentDistance = std::clamp(m_currentDistance, minDistance, maxDistance);
    }
}