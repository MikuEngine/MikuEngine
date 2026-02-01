#include "GamePCH.h"
#include "CameraEffectScript.h"

#include <Framework/Object/Component/Camera.h>
#include <Framework/Object/Component/Transform.h>
#include <Core/Graphics/Device/GraphicsDevice.h>

namespace game
{
    void CameraEffectScript::Awake()
    {
        // 카메라 컴포넌트 캐시
        m_camera = GetGameObject()->GetComponent<engine::Camera>();
        if (!m_camera)
        {
            // LOG_PRINT("[CameraEffectScript] Camera component not found on this GameObject!");
        }
    }

    void CameraEffectScript::Start()
    {
        // 플레이어 찾기
        m_player = engine::GameObject::Find("Player");
        if (!m_player)
        {
            // LOG_PRINT("[CameraEffectScript] Player GameObject not found!");
        }

        // 초기 카메라 상태 저장 (한 번만, 절대 변경 안 함)
        if (!m_isInitialized)
        {
            m_initialPosition = GetTransform()->GetWorldPosition();
            m_initialFov = m_configuredBaseFov;
            m_currentFov = m_initialFov;
            m_isInitialized = true;
            
            // 원본 시야 영역 계산
            CalculateViewCornersAt(m_initialPosition, m_initialFov, m_baseViewCorners);
        }
    }

    void CameraEffectScript::Update()
    {
        if (m_state == EffectState::Idle)
            return;

        if (!m_camera)
            return;

        // 타임스케일 적용된 DeltaTime 사용
        float dt = engine::Time::DeltaTime();
        
        m_elapsedTime += dt;
        m_stateElapsedTime += dt;

        // 상태별 처리
        switch (m_state)
        {
        case EffectState::FadeIn:
            if (m_stateElapsedTime >= m_fadeInDuration)
            {
                TransitionToNextState();
            }
            break;

        case EffectState::Sustain:
            if (m_stateElapsedTime >= m_sustainDuration)
            {
                TransitionToNextState();
            }
            break;

        case EffectState::FadeOut:
            if (m_stateElapsedTime >= m_fadeOutDuration)
            {
                TransitionToNextState();
            }
            break;

        default:
            break;
        }

        // 블렌드 팩터 계산 및 카메라 업데이트 (Idle이면 스킵)
        if (m_state != EffectState::Idle)
        {
            float blendFactor = CalculateBlendFactor();
            UpdateCameraTransform(blendFactor);
        }
    }

    void CameraEffectScript::StartZoomEffect(float duration)
    {
        if (!m_camera || !m_player)
        {
            // LOG_PRINT("[CameraEffectScript] Cannot start effect - missing Camera or Player!");
            return;
        }

        if (duration <= 0.0f)
        {
            // LOG_PRINT("[CameraEffectScript] Invalid duration: {}", duration);
            return;
        }

        // 이미 이펙트 중이면 현재 상태에서 시작
        if (m_state != EffectState::Idle)
        {
            m_effectStartPosition = GetTransform()->GetWorldPosition();
            m_effectStartFov = m_currentFov;  // 현재 FOV에서 시작
        }
        else
        {
            // 새로 시작하면 초기값에서 시작
            m_effectStartPosition = m_initialPosition;
            m_effectStartFov = m_initialFov;
        }

        // 타이밍 계산
        m_totalDuration = duration;
        m_fadeInDuration = duration * m_fadeInRatio;
        m_fadeOutDuration = duration * m_fadeOutRatio;
        m_sustainDuration = duration - m_fadeInDuration - m_fadeOutDuration;

        // sustain이 음수가 되지 않도록 보정
        if (m_sustainDuration < 0.0f)
        {
            float totalRatio = m_fadeInRatio + m_fadeOutRatio;
            m_fadeInDuration = duration * (m_fadeInRatio / totalRatio);
            m_fadeOutDuration = duration * (m_fadeOutRatio / totalRatio);
            m_sustainDuration = 0.0f;
        }

        // 목표 FOV 계산 (줌 배율 적용)
        m_targetFov = m_initialFov / m_zoomScale;

        // 목표 위치 계산 (클램핑 포함)
        m_targetPosition = CalculateTargetPosition();

        // 타이머 초기화
        m_elapsedTime = 0.0f;
        m_stateElapsedTime = 0.0f;

        // 상태 전이
        m_state = EffectState::FadeIn;

        // LOG_PRINT("[CameraEffectScript] Zoom effect started - Duration: {}, ZoomScale: {}, TargetFOV: {}",
        //     duration, m_zoomScale, m_targetFov);
    }

    void CameraEffectScript::StopEffect()
    {
        if (m_state == EffectState::Idle)
            return;

        // 즉시 초기 상태로 복귀
        if (m_camera)
        {
            GetTransform()->SetLocalPosition(m_initialPosition);
            m_camera->SetFov(m_initialFov);
            m_currentFov = m_initialFov;
        }

        m_state = EffectState::Idle;
        m_elapsedTime = 0.0f;
        m_stateElapsedTime = 0.0f;

        // LOG_PRINT("[CameraEffectScript] Effect stopped");
    }

    void CameraEffectScript::OnGui()
    {
        ImGui::Text("=== Camera Effect Settings ===");
        ImGui::Separator();

        // 기본 카메라 설정
        ImGui::Text("Base Camera Settings:");
        ImGui::DragFloat("Base FOV", &m_configuredBaseFov, 0.5f, 30.0f, 120.0f, "%.1f deg");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Must match Camera component's FOV value");
        }

        ImGui::Separator();

        // 줌 설정
        ImGui::Text("Zoom Settings:");
        ImGui::DragFloat("Zoom Scale", &m_zoomScale, 0.01f, 1.01f, 3.0f, "%.2fx");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Zoom magnification (1.3 = 1.3x zoom)");
        }
        
        float targetFov = m_configuredBaseFov / m_zoomScale;
        ImGui::Text("Target FOV: %.1f deg (auto)", targetFov);

        ImGui::Separator();

        // 크로스페이드 설정
        ImGui::Text("Crossfade Settings:");
        ImGui::DragFloat("Fade In Ratio", &m_fadeInRatio, 0.01f, 0.0f, 0.5f, "%.2f");
        ImGui::DragFloat("Fade Out Ratio", &m_fadeOutRatio, 0.01f, 0.0f, 0.5f, "%.2f");
        
        float sustainRatio = 1.0f - m_fadeInRatio - m_fadeOutRatio;
        if (sustainRatio < 0.0f) sustainRatio = 0.0f;
        ImGui::Text("Sustain Ratio: %.2f (auto)", sustainRatio);

        ImGui::Separator();

        // 런타임 상태 표시
        ImGui::Text("Runtime Status:");
        const char* stateNames[] = { "Idle", "FadeIn", "Sustain", "FadeOut" };
        ImGui::Text("State: %s", stateNames[static_cast<int>(m_state)]);
        
        if (m_state != EffectState::Idle)
        {
            ImGui::Text("Elapsed: %.2f / %.2f", m_elapsedTime, m_totalDuration);
            ImGui::Text("State Elapsed: %.2f", m_stateElapsedTime);
            ImGui::Text("Blend Factor: %.3f", CalculateBlendFactor());
            
            if (m_camera)
            {
                ImGui::Text("Current FOV: %.1f", m_camera->GetTransform()->GetLocalPosition().y); // FOV는 직접 접근 불가하므로 위치 표시
            }
        }

        ImGui::Separator();

        // 초기 상태 정보
        ImGui::Text("Initial State:");
        ImGui::Text("Position: (%.1f, %.1f, %.1f)", m_initialPosition.x, m_initialPosition.y, m_initialPosition.z);
        ImGui::Text("FOV: %.1f", m_initialFov);
        
        ImGui::Separator();
        
        // 이동 가능 경계 설정
        ImGui::Text("Movement Bounds:");
        ImGui::DragFloat("Min X", &m_moveBoundsMinX, 0.5f, -100.0f, 100.0f, "%.1f");
        ImGui::DragFloat("Max X", &m_moveBoundsMaxX, 0.5f, -100.0f, 100.0f, "%.1f");
        ImGui::DragFloat("Min Z", &m_moveBoundsMinZ, 0.5f, -100.0f, 100.0f, "%.1f");
        ImGui::DragFloat("Max Z", &m_moveBoundsMaxZ, 0.5f, -100.0f, 100.0f, "%.1f");

        ImGui::Separator();
        
        // 이펙트 듀레이션 설정
        ImGui::Text("Effect Duration:");
        ImGui::DragFloat("Default Duration", &m_defaultDuration, 0.1f, 0.1f, 10.0f, "%.1f sec");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Default duration when StartZoomEffect() is called without parameter");
        }

        ImGui::Separator();

        // 테스트 버튼
        if (ImGui::Button("Test Effect"))
        {
            StartZoomEffect(m_defaultDuration);
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop Effect"))
        {
            StopEffect();
        }
    }

    void CameraEffectScript::Save(engine::json& j) const
    {
        Object::Save(j);

        j["BaseFov"] = m_configuredBaseFov;
        j["ZoomScale"] = m_zoomScale;
        j["FadeInRatio"] = m_fadeInRatio;
        j["FadeOutRatio"] = m_fadeOutRatio;
        
        j["MoveBoundsMinX"] = m_moveBoundsMinX;
        j["MoveBoundsMaxX"] = m_moveBoundsMaxX;
        j["MoveBoundsMinZ"] = m_moveBoundsMinZ;
        j["MoveBoundsMaxZ"] = m_moveBoundsMaxZ;
        
        j["DefaultDuration"] = m_defaultDuration;
    }

    void CameraEffectScript::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "BaseFov", m_configuredBaseFov);
        engine::JsonGet(j, "ZoomScale", m_zoomScale);
        engine::JsonGet(j, "FadeInRatio", m_fadeInRatio);
        engine::JsonGet(j, "FadeOutRatio", m_fadeOutRatio);
        
        engine::JsonGet(j, "MoveBoundsMinX", m_moveBoundsMinX);
        engine::JsonGet(j, "MoveBoundsMaxX", m_moveBoundsMaxX);
        engine::JsonGet(j, "MoveBoundsMinZ", m_moveBoundsMinZ);
        engine::JsonGet(j, "MoveBoundsMaxZ", m_moveBoundsMaxZ);
        
        engine::JsonGet(j, "DefaultDuration", m_defaultDuration);
    }

    void CameraEffectScript::CalculateBaseViewCorners()
    {
        CalculateViewCornersAt(m_initialPosition, m_initialFov, m_baseViewCorners);
    }

    engine::Vector3 CameraEffectScript::CalculateTargetPosition()
    {
        if (!m_player)
            return m_initialPosition;

        // 플레이어 위치 (Y=0 평면에서)
        engine::Vector3 playerPos = m_player->GetTransform()->GetWorldPosition();
        playerPos.y = 0.0f;

        // 카메라의 시선 방향 (XZ 평면에 투영)
        engine::Vector3 forward = GetTransform()->GetForward();
        engine::Vector2 forwardXZ(forward.x, forward.z);
        forwardXZ.Normalize();

        // 초기 카메라 위치에서 바닥으로 내린 수직점
        engine::Vector3 camPos = m_initialPosition;
        
        // 카메라가 바라보는 방향으로 ray를 쏴서 Y=0과 교차점 찾기
        // 이 교차점이 현재 화면 중앙
        float t = -camPos.y / forward.y;
        engine::Vector3 screenCenter(
            camPos.x + forward.x * t,
            0.0f,
            camPos.z + forward.z * t
        );

        // 플레이어를 화면 중앙에 두려면 카메라를 얼마나 이동해야 하는지 계산
        engine::Vector3 offset = playerPos - screenCenter;
        
        // 이상적인 목표 위치 (Y는 유지)
        engine::Vector3 idealPosition(
            m_initialPosition.x + offset.x,
            m_initialPosition.y,
            m_initialPosition.z + offset.z
        );

        // 클램핑
        return ClampPositionToBounds(idealPosition);
    }

    bool CameraEffectScript::IsPositionWithinBounds(const engine::Vector3& position)
    {
        // 해당 위치에서 줌인된 카메라의 4개 코너 계산
        engine::Vector3 zoomedCorners[4];
        CalculateViewCornersAt(position, m_targetFov, zoomedCorners);

        // 모든 코너가 원본 영역 안에 있는지 확인
        for (int i = 0; i < 4; ++i)
        {
            engine::Vector2 cornerXZ(zoomedCorners[i].x, zoomedCorners[i].z);
            if (!IsPointInQuad(cornerXZ, m_baseViewCorners))
            {
                return false;
            }
        }

        return true;
    }

    engine::Vector3 CameraEffectScript::ClampPositionToBounds(const engine::Vector3& idealPosition)
    {
        // 경계 박스 기반 단순 클램핑
        engine::Vector3 clampedPosition = idealPosition;
        
        clampedPosition.x = std::clamp(clampedPosition.x, m_moveBoundsMinX, m_moveBoundsMaxX);
        clampedPosition.z = std::clamp(clampedPosition.z, m_moveBoundsMinZ, m_moveBoundsMaxZ);
        clampedPosition.y = m_initialPosition.y;  // Y는 항상 초기값
        
        return clampedPosition;
    }

    void CameraEffectScript::CalculateViewCornersAt(const engine::Vector3& position, float fov, engine::Vector3 outCorners[4])
    {
        // 뷰포트 정보
        const auto& vp = engine::GraphicsDevice::Get().GetViewport();
        float aspectRatio = vp.Width / vp.Height;

        // 카메라 방향 벡터들
        engine::Transform* transform = GetTransform();
        engine::Vector3 forward = transform->GetForward();
        engine::Vector3 right = transform->GetRight();
        engine::Vector3 up = transform->GetUp();

        // FOV를 라디안으로 변환 (수직 FOV 기준)
        float fovRad = DirectX::XMConvertToRadians(fov);
        float halfFovV = fovRad * 0.5f;
        float halfFovH = std::atan(std::tan(halfFovV) * aspectRatio);

        // 4개 코너 방향 벡터 계산
        // 순서: 좌하단, 우하단, 우상단, 좌상단
        engine::Vector3 directions[4];
        float tanV = std::tan(halfFovV);
        float tanH = std::tan(halfFovH);

        // 각 코너 방향
        directions[0] = forward - right * tanH - up * tanV; // 좌하단
        directions[1] = forward + right * tanH - up * tanV; // 우하단
        directions[2] = forward + right * tanH + up * tanV; // 우상단
        directions[3] = forward - right * tanH + up * tanV; // 좌상단

        // 각 방향을 정규화하고 Y=0 평면과의 교점 계산
        for (int i = 0; i < 4; ++i)
        {
            directions[i].Normalize();

            // Y=0 평면과의 교점 (position.y + t * direction.y = 0)
            if (std::abs(directions[i].y) > 0.0001f)
            {
                float t = -position.y / directions[i].y;
                if (t > 0.0f) // 카메라 앞에 있어야 함
                {
                    outCorners[i] = engine::Vector3(
                        position.x + directions[i].x * t,
                        0.0f,
                        position.z + directions[i].z * t
                    );
                }
                else
                {
                    // 카메라 뒤에 있는 경우 (발생하면 안 됨)
                    outCorners[i] = engine::Vector3(position.x, 0.0f, position.z);
                }
            }
            else
            {
                // 수평 방향 (교점 없음)
                outCorners[i] = engine::Vector3(position.x, 0.0f, position.z);
            }
        }
    }

    bool CameraEffectScript::IsPointInQuad(const engine::Vector2& point, const engine::Vector3 corners[4])
    {
        // 사각형 내부 판정 (Cross product 방식)
        // 모든 edge에 대해 같은 방향이면 내부
        
        for (int i = 0; i < 4; ++i)
        {
            int next = (i + 1) % 4;
            
            engine::Vector2 edge(
                corners[next].x - corners[i].x,
                corners[next].z - corners[i].z
            );
            
            engine::Vector2 toPoint(
                point.x - corners[i].x,
                point.y - corners[i].z
            );
            
            // 2D cross product (z 성분)
            float cross = edge.x * toPoint.y - edge.y * toPoint.x;
            
            // 시계방향 기준으로 오른쪽에 있으면 외부
            if (cross < 0.0f)
            {
                return false;
            }
        }
        
        return true;
    }

    float CameraEffectScript::CalculateBlendFactor() const
    {
        switch (m_state)
        {
        case EffectState::Idle:
            return 0.0f;

        case EffectState::FadeIn:
            if (m_fadeInDuration <= 0.0f) return 1.0f;
            return std::min(m_stateElapsedTime / m_fadeInDuration, 1.0f);

        case EffectState::Sustain:
            return 1.0f;

        case EffectState::FadeOut:
            if (m_fadeOutDuration <= 0.0f) return 0.0f;
            return 1.0f - std::min(m_stateElapsedTime / m_fadeOutDuration, 1.0f);

        default:
            return 0.0f;
        }
    }

    void CameraEffectScript::UpdateCameraTransform(float blendFactor)
    {
        if (!m_camera)
            return;

        // FOV 보간
        float newFov;
        if (m_state == EffectState::FadeOut)
        {
            // FadeOut: targetFov → initialFov
            // blendFactor: 1→0 이므로 (1-blendFactor): 0→1
            float fadeOutProgress = 1.0f - blendFactor;
            newFov = m_targetFov + (m_initialFov - m_targetFov) * fadeOutProgress;
        }
        else
        {
            // FadeIn/Sustain: effectStartFov → targetFov
            newFov = m_effectStartFov + (m_targetFov - m_effectStartFov) * blendFactor;
            // 설정된 목표 FOV에 도달하면 더 이상 줌인하지 않음
            newFov = std::max(newFov, m_targetFov);
        }
        
        m_currentFov = newFov;
        m_camera->SetFov(m_currentFov);

        // 위치 보간 (XZ만, Y는 고정)
        engine::Vector3 currentPos;
        if (m_state == EffectState::FadeOut)
        {
            // FadeOut: targetPosition → initialPosition
            // blendFactor: 1→0 이므로 (1-blendFactor): 0→1
            float fadeOutProgress = 1.0f - blendFactor;
            currentPos.x = m_targetPosition.x + (m_initialPosition.x - m_targetPosition.x) * fadeOutProgress;
            currentPos.z = m_targetPosition.z + (m_initialPosition.z - m_targetPosition.z) * fadeOutProgress;
        }
        else
        {
            // FadeIn/Sustain: effectStartPosition → targetPosition
            currentPos.x = m_effectStartPosition.x + (m_targetPosition.x - m_effectStartPosition.x) * blendFactor;
            currentPos.z = m_effectStartPosition.z + (m_targetPosition.z - m_effectStartPosition.z) * blendFactor;
        }
        currentPos.y = m_initialPosition.y; // Y는 항상 초기값

        GetTransform()->SetLocalPosition(currentPos);
    }

    void CameraEffectScript::TransitionToNextState()
    {
        m_stateElapsedTime = 0.0f;

        switch (m_state)
        {
        case EffectState::FadeIn:
            m_state = EffectState::Sustain;
            // LOG_PRINT("[CameraEffectScript] Transition: FadeIn -> Sustain");
            break;

        case EffectState::Sustain:
            m_state = EffectState::FadeOut;
            // LOG_PRINT("[CameraEffectScript] Transition: Sustain -> FadeOut");
            break;

        case EffectState::FadeOut:
            // 이펙트 종료 - 초기 상태로 복귀
            GetTransform()->SetLocalPosition(m_initialPosition);
            m_camera->SetFov(m_initialFov);
            m_currentFov = m_initialFov;
            m_state = EffectState::Idle;
            // LOG_PRINT("[CameraEffectScript] Effect completed - returned to initial state");
            break;

        default:
            break;
        }
    }
}
