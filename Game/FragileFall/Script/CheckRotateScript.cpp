#include "GamePCH.h"
#include "CheckRotateScript.h"

#include <Framework/Object/Component/Rigidbody.h>

namespace game
{
    void CheckRotateScript::Start()
    {
        if (!InitializeObjects())
        {
            LOG_ERROR("[CheckRotateScript] Failed to initialize objects!");
            return;
        }

        // 초기 방향 벡터 저장 (Y=0으로 투영 후 정규화)
        engine::Vector3 posP1 = m_bulletP1->GetTransform()->GetWorldPosition();
        engine::Vector3 posQ1 = m_bulletQ1->GetTransform()->GetWorldPosition();
        engine::Vector3 posP2 = m_bulletP2->GetTransform()->GetWorldPosition();
        engine::Vector3 posQ2 = m_bulletQ2->GetTransform()->GetWorldPosition();

        // Y 성분 제거
        posP1.y = 0.0f;
        posQ1.y = 0.0f;
        posP2.y = 0.0f;
        posQ2.y = 0.0f;

        // 반지름 저장
        m_radiusP1 = posP1.Length();
        m_radiusP2 = posP2.Length();

        // 정규화된 방향 벡터
        m_initialDirP1 = posP1;
        m_initialDirP1.Normalize();

        m_targetDirQ1 = posQ1;
        m_targetDirQ1.Normalize();

        m_initialDirP2 = posP2;
        m_initialDirP2.Normalize();

        m_targetDirQ2 = posQ2;
        m_targetDirQ2.Normalize();

        // 회전 방향 계산 (외적 Y성분의 부호)
        m_rotationDirP1 = CalculateRotationDirection(m_initialDirP1, m_targetDirQ1);
        m_rotationDirP2 = CalculateRotationDirection(m_initialDirP2, m_targetDirQ2);

        // 총 회전 각도 계산
        m_totalAngleP1 = CalculateAngleBetween(m_initialDirP1, m_targetDirQ1);
        m_totalAngleP2 = CalculateAngleBetween(m_initialDirP2, m_targetDirQ2);

        // Rigidbody 설정 (등속 원운동을 위해 damping 조정)
        if (m_rbP1)
        {
            m_rbP1->SetLinearDamping(0.0f);
            m_rbP1->SetAngularDamping(100.0f);
            m_rbP1->SetUseGravity(false);
        }
        if (m_rbP2)
        {
            m_rbP2->SetLinearDamping(0.0f);
            m_rbP2->SetAngularDamping(100.0f);
            m_rbP2->SetUseGravity(false);
        }

        LOG_INFO("[CheckRotateScript] Initialized");
        LOG_INFO("  P1: radius=%.2f, totalAngle=%.2f, dir=%s", 
            m_radiusP1, m_totalAngleP1, m_rotationDirP1 > 0 ? "CW" : "CCW");
        LOG_INFO("  P2: radius=%.2f, totalAngle=%.2f, dir=%s", 
            m_radiusP2, m_totalAngleP2, m_rotationDirP2 > 0 ? "CW" : "CCW");
    }

    void CheckRotateScript::Update()
    {
        // 대기 시간 체크
        if (m_stateP1 == RotateState::Waiting || m_stateP2 == RotateState::Waiting || m_stateCube == RotateState::Waiting)
        {
            m_waitTimer += engine::Time::DeltaTime();
            
            if (m_waitTimer >= m_waitDuration)
            {
                if (m_stateP1 == RotateState::Waiting)
                    m_stateP1 = RotateState::Rotating;
                if (m_stateP2 == RotateState::Waiting)
                    m_stateP2 = RotateState::Rotating;
                if (m_stateCube == RotateState::Waiting)
                    m_stateCube = RotateState::Rotating;
                
                LOG_INFO("[CheckRotateScript] Rotation started!");
            }
        }

        // P1 회전 업데이트
        if (m_rbP1 && m_stateP1 == RotateState::Rotating)
        {
            UpdateCircularMotion(
                m_rbP1,
                m_rotationDirP1,
                m_radiusP1,
                m_stateP1,
                m_rotatedAngleP1,
                m_totalAngleP1,
                m_targetDirQ1
            );
        }

        // P2 회전 업데이트
        if (m_rbP2 && m_stateP2 == RotateState::Rotating)
        {
            UpdateCircularMotion(
                m_rbP2,
                m_rotationDirP2,
                m_radiusP2,
                m_stateP2,
                m_rotatedAngleP2,
                m_totalAngleP2,
                m_targetDirQ2
            );
        }

        // Cube Y축 자전 (P1과 같은 각속도로)
        if (m_rbCube && m_stateCube == RotateState::Rotating)
        {
            // P1의 회전 속도(도/초)를 라디안/초로 변환
            float angularSpeedRad = DirectX::XMConvertToRadians(m_rotationSpeed);
            
            // P1과 같은 방향으로 회전
            // 주의: PhysX의 Angular Velocity는 오른손 법칙을 따름
            //       Y축 양의 각속도 = 위에서 봤을 때 CCW
            //       왼손 좌표계에서 CW = PhysX에서 음의 각속도
            // 따라서 부호를 반대로 해야 P1 공전 방향과 Cube 자전 방향이 일치
            float yAngularVelocity = -angularSpeedRad * m_rotationDirP1;
            
            // 각속도 설정 (AddTorque 대신 SetAngularVelocity로 즉시 설정)
            engine::Vector3 currentAngVel = m_rbCube->GetAngularVelocity();
            engine::Vector3 targetAngVel = engine::Vector3(0.0f, yAngularVelocity, 0.0f);
            
            // 빠르게 목표 각속도에 도달하도록 큰 토크 적용
            engine::Vector3 angVelDiff = targetAngVel - currentAngVel;
            m_rbCube->AddTorque(angVelDiff * m_accelerationForce, engine::ForceMode::Force);
            
            // 각속도 캡 적용
            engine::Vector3 newAngVel = m_rbCube->GetAngularVelocity();
            if (std::abs(newAngVel.y) > std::abs(yAngularVelocity) * 1.1f)
            {
                newAngVel.y = yAngularVelocity;
                m_rbCube->SetAngularVelocity(newAngVel);
            }
            
            // P1이 완료되면 Cube도 완료
            if (m_stateP1 == RotateState::Completed)
            {
                m_stateCube = RotateState::Completed;
            }
        }

        // 완료된 오브젝트 속도 제거
        if (m_rbP1 && m_stateP1 == RotateState::Completed)
        {
            m_rbP1->SetLinearVelocity(engine::Vector3::Zero);
        }
        if (m_rbP2 && m_stateP2 == RotateState::Completed)
        {
            m_rbP2->SetLinearVelocity(engine::Vector3::Zero);
        }
        if (m_rbCube && m_stateCube == RotateState::Completed)
        {
            m_rbCube->SetAngularVelocity(engine::Vector3::Zero);
        }
    }

    bool CheckRotateScript::InitializeObjects()
    {
        // 게임오브젝트 찾기
        m_bulletP1 = engine::GameObject::Find("BulletP1");
        m_bulletQ1 = engine::GameObject::Find("BulletQ1");
        m_bulletP2 = engine::GameObject::Find("BulletP2");
        m_bulletQ2 = engine::GameObject::Find("BulletQ2");

        if (!m_bulletP1 || !m_bulletQ1 || !m_bulletP2 || !m_bulletQ2)
        {
            LOG_ERROR("[CheckRotateScript] Could not find all Bullet objects!");
            return false;
        }

        // Rigidbody 컴포넌트 가져오기
        m_rbP1 = m_bulletP1->GetComponent<engine::Rigidbody>();
        m_rbP2 = m_bulletP2->GetComponent<engine::Rigidbody>();

        if (!m_rbP1 || !m_rbP2)
        {
            LOG_ERROR("[CheckRotateScript] P1 or P2 does not have Rigidbody!");
            return false;
        }

        // Cube 오브젝트 찾기 (옵션)
        m_cube = engine::GameObject::Find("Cube");
        if (m_cube)
        {
            m_rbCube = m_cube->GetComponent<engine::Rigidbody>();
            if (m_rbCube)
            {
                m_rbCube->SetAngularDamping(0.0f);  // 등속 회전을 위해
                m_rbCube->SetLinearDamping(100.0f); // 이동은 억제
                m_rbCube->SetUseGravity(false);
                LOG_INFO("[CheckRotateScript] Cube found and initialized");
            }
        }

        return true;
    }

    float CheckRotateScript::CalculateRotationDirection(const engine::Vector3& from, const engine::Vector3& to)
    {
        // 외적 계산: from × to
        engine::Vector3 cross = from.Cross(to);

        // 왼손 좌표계에서:
        // Y성분이 양수 → 양의 방향(CW) 회전이 최단
        // Y성분이 음수 → 음의 방향(CCW) 회전이 최단
        return (cross.y >= 0.0f) ? 1.0f : -1.0f;
    }

    float CheckRotateScript::CalculateAngleBetween(const engine::Vector3& from, const engine::Vector3& to)
    {
        float dot = from.Dot(to);
        dot = std::clamp(dot, -1.0f, 1.0f);
        return DirectX::XMConvertToDegrees(std::acos(dot));
    }

    void CheckRotateScript::UpdateCircularMotion(
        engine::Rigidbody* rb,
        float rotationDir,
        float radius,
        RotateState& state,
        float& rotatedAngle,
        float totalAngle,
        const engine::Vector3& targetDir)
    {
        // 현재 위치
        engine::Vector3 currentPos = rb->GetGameObject()->GetTransform()->GetWorldPosition();
        currentPos.y = 0.0f;

        // 현재 방향 (정규화)
        engine::Vector3 currentDir = currentPos;
        currentDir.Normalize();

        // 목표 도달 체크
        float dot = currentDir.Dot(targetDir);
        if (dot >= m_completionThreshold)
        {
            state = RotateState::Completed;
            rotatedAngle = totalAngle;
            rb->SetLinearVelocity(engine::Vector3::Zero);
            LOG_INFO("[CheckRotateScript] Rotation completed!");
            return;
        }

        // 현재까지 회전한 각도 업데이트
        // (초기 방향과 현재 방향 사이의 각도)
        // 참고: 이 방식은 180도를 넘으면 부정확해질 수 있음
        rotatedAngle = totalAngle - CalculateAngleBetween(currentDir, targetDir);

        // 목표 접선 속도 계산
        float angularSpeedRad = DirectX::XMConvertToRadians(m_rotationSpeed);  // rad/s
        float targetTangentSpeed = angularSpeedRad * radius;

        // 속도 캡 적용
        targetTangentSpeed = std::min(targetTangentSpeed, m_maxLinearSpeed);

        // 접선 방향 계산
        engine::Vector3 tangent = GetTangentDirection(currentPos, rotationDir);

        // 목표 속도 벡터
        engine::Vector3 targetVelocity = tangent * targetTangentSpeed;

        // 현재 속도
        engine::Vector3 currentVelocity = rb->GetLinearVelocity();
        currentVelocity.y = 0.0f;

        // 속도 차이
        engine::Vector3 velocityDiff = targetVelocity - currentVelocity;

        // 구심력 계산 (원형 궤도 유지를 위해)
        // F_centripetal = m * v^2 / r, 방향은 중심(원점) 방향
        float currentSpeed = currentVelocity.Length();
        engine::Vector3 toCenter = -currentDir;  // 원점 방향
        float centripetalMagnitude = (currentSpeed * currentSpeed) / radius;
        engine::Vector3 centripetalForce = toCenter * centripetalMagnitude * rb->GetMass();

        // 접선 방향 힘 (목표 속도에 빠르게 도달하기 위한 큰 힘)
        engine::Vector3 tangentialForce = velocityDiff * m_accelerationForce;

        // 총 힘 적용
        engine::Vector3 totalForce = tangentialForce + centripetalForce;
        rb->AddForce(totalForce, engine::ForceMode::Force);

        // 속도 캡 적용 (수동 클램핑)
        engine::Vector3 newVelocity = rb->GetLinearVelocity();
        newVelocity.y = 0.0f;
        float speed = newVelocity.Length();
        if (speed > m_maxLinearSpeed)
        {
            newVelocity.Normalize();
            newVelocity *= m_maxLinearSpeed;
            rb->SetLinearVelocity(newVelocity);
        }

        // Y 속도 제거 (XZ 평면에서만 이동)
        engine::Vector3 vel = rb->GetLinearVelocity();
        if (std::abs(vel.y) > 0.001f)
        {
            vel.y = 0.0f;
            rb->SetLinearVelocity(vel);
        }
    }

    engine::Vector3 CheckRotateScript::GetTangentDirection(const engine::Vector3& position, float rotationDir)
    {
        // 왼손 좌표계에서 Y축 기준 회전의 접선 방향
        // 
        // 접선 = 위치 벡터를 90도 회전한 방향
        // - CW 접선 (양의 회전): (z, 0, -x)  ← 위치를 CW 90도 회전
        // - CCW 접선 (음의 회전): (-z, 0, x) ← 위치를 CCW 90도 회전
        //
        // 물리적 유도: v = ω × r
        // ω = (0, +ω, 0) 일 때, v = (ωz, 0, -ωx) → 방향: (z, 0, -x)
        
        engine::Vector3 tangent;
        if (rotationDir > 0.0f)
        {
            // CW (양의 방향): (z, 0, -x)
            tangent = engine::Vector3(position.z, 0.0f, -position.x);
        }
        else
        {
            // CCW (음의 방향): (-z, 0, x)
            tangent = engine::Vector3(-position.z, 0.0f, position.x);
        }
        tangent.Normalize();
        return tangent;
    }

    const char* CheckRotateScript::StateToString(RotateState state)
    {
        switch (state)
        {
        case RotateState::Waiting:   return "Waiting";
        case RotateState::Rotating:  return "Rotating";
        case RotateState::Completed: return "Completed";
        default:                     return "Unknown";
        }
    }

    void CheckRotateScript::OnGui()
    {
        ImGui::Text("=== Check Rotate Script ===");
        ImGui::Separator();

        // 설정
        ImGui::Text("Settings:");
        ImGui::InputFloat("Wait Duration (s)", &m_waitDuration);
        ImGui::InputFloat("Rotation Speed (deg/s)", &m_rotationSpeed);
        ImGui::InputFloat("Max Linear Speed", &m_maxLinearSpeed);
        ImGui::InputFloat("Acceleration Force", &m_accelerationForce);
        ImGui::Separator();

        // 타이머
        ImGui::Text("Wait Timer: %.2f / %.2f", m_waitTimer, m_waitDuration);
        ImGui::Separator();

        // P1, Q1 정보
        if (m_bulletP1 && m_bulletQ1)
        {
            engine::Vector3 posP1 = m_bulletP1->GetTransform()->GetWorldPosition();
            engine::Vector3 posQ1 = m_bulletQ1->GetTransform()->GetWorldPosition();
            
            // 현재 방향 (정규화)
            engine::Vector3 dirP1 = posP1; dirP1.y = 0; dirP1.Normalize();
            engine::Vector3 dirQ1 = posQ1; dirQ1.y = 0; dirQ1.Normalize();

            // 내적, 외적
            float dotP1Q1 = dirP1.Dot(dirQ1);
            engine::Vector3 crossP1Q1 = dirP1.Cross(dirQ1);

            ImGui::Text("--- P1 -> Q1 ---");
            ImGui::Text("P1 Position: (%.2f, %.2f, %.2f)", posP1.x, posP1.y, posP1.z);
            ImGui::Text("Q1 Position: (%.2f, %.2f, %.2f)", posQ1.x, posQ1.y, posQ1.z);
            ImGui::Text("P1 Direction: (%.3f, %.3f, %.3f)", dirP1.x, dirP1.y, dirP1.z);
            ImGui::Text("Q1 Direction: (%.3f, %.3f, %.3f)", dirQ1.x, dirQ1.y, dirQ1.z);
            ImGui::Separator();
            
            ImGui::Text("Dot(P1, Q1): %.4f", dotP1Q1);
            ImGui::Text("Cross(P1, Q1): (%.4f, %.4f, %.4f)", crossP1Q1.x, crossP1Q1.y, crossP1Q1.z);
            ImGui::TextColored(
                crossP1Q1.y >= 0 ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.5f, 0.2f, 1.0f),
                "Cross Y: %.4f -> %s", crossP1Q1.y, crossP1Q1.y >= 0 ? "CW (Positive)" : "CCW (Negative)");
            ImGui::Separator();
            
            ImGui::Text("State: %s", StateToString(m_stateP1));
            ImGui::Text("Rotation Direction: %s", m_rotationDirP1 > 0 ? "CW (Positive)" : "CCW (Negative)");
            ImGui::Text("Rotated Angle: %.2f / %.2f deg", m_rotatedAngleP1, m_totalAngleP1);
            ImGui::Text("Remaining Angle: %.2f deg", m_totalAngleP1 - m_rotatedAngleP1);
            
            if (m_rbP1)
            {
                engine::Vector3 vel = m_rbP1->GetLinearVelocity();
                ImGui::Text("P1 Velocity: (%.2f, %.2f, %.2f) |%.2f|", vel.x, vel.y, vel.z, vel.Length());
            }
        }

        ImGui::Separator();
        ImGui::Separator();

        // P2, Q2 정보
        if (m_bulletP2 && m_bulletQ2)
        {
            engine::Vector3 posP2 = m_bulletP2->GetTransform()->GetWorldPosition();
            engine::Vector3 posQ2 = m_bulletQ2->GetTransform()->GetWorldPosition();
            
            // 현재 방향 (정규화)
            engine::Vector3 dirP2 = posP2; dirP2.y = 0; dirP2.Normalize();
            engine::Vector3 dirQ2 = posQ2; dirQ2.y = 0; dirQ2.Normalize();

            // 내적, 외적
            float dotP2Q2 = dirP2.Dot(dirQ2);
            engine::Vector3 crossP2Q2 = dirP2.Cross(dirQ2);

            ImGui::Text("--- P2 -> Q2 ---");
            ImGui::Text("P2 Position: (%.2f, %.2f, %.2f)", posP2.x, posP2.y, posP2.z);
            ImGui::Text("Q2 Position: (%.2f, %.2f, %.2f)", posQ2.x, posQ2.y, posQ2.z);
            ImGui::Text("P2 Direction: (%.3f, %.3f, %.3f)", dirP2.x, dirP2.y, dirP2.z);
            ImGui::Text("Q2 Direction: (%.3f, %.3f, %.3f)", dirQ2.x, dirQ2.y, dirQ2.z);
            ImGui::Separator();
            
            ImGui::Text("Dot(P2, Q2): %.4f", dotP2Q2);
            ImGui::Text("Cross(P2, Q2): (%.4f, %.4f, %.4f)", crossP2Q2.x, crossP2Q2.y, crossP2Q2.z);
            ImGui::TextColored(
                crossP2Q2.y >= 0 ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : ImVec4(1.0f, 0.5f, 0.2f, 1.0f),
                "Cross Y: %.4f -> %s", crossP2Q2.y, crossP2Q2.y >= 0 ? "CW (Positive)" : "CCW (Negative)");
            ImGui::Separator();
            
            ImGui::Text("State: %s", StateToString(m_stateP2));
            ImGui::Text("Rotation Direction: %s", m_rotationDirP2 > 0 ? "CW (Positive)" : "CCW (Negative)");
            ImGui::Text("Rotated Angle: %.2f / %.2f deg", m_rotatedAngleP2, m_totalAngleP2);
            ImGui::Text("Remaining Angle: %.2f deg", m_totalAngleP2 - m_rotatedAngleP2);
            
            if (m_rbP2)
            {
                engine::Vector3 vel = m_rbP2->GetLinearVelocity();
                ImGui::Text("P2 Velocity: (%.2f, %.2f, %.2f) |%.2f|", vel.x, vel.y, vel.z, vel.Length());
            }
        }

        ImGui::Separator();
        ImGui::Separator();

        // Cube 정보
        if (m_cube && m_rbCube)
        {
            ImGui::Text("--- Cube (Y-Axis Spin) ---");
            ImGui::Text("State: %s", StateToString(m_stateCube));
            
            engine::Vector3 angVel = m_rbCube->GetAngularVelocity();
            float angSpeedDeg = DirectX::XMConvertToDegrees(angVel.y);
            ImGui::Text("Angular Velocity Y: %.4f rad/s (%.2f deg/s)", angVel.y, angSpeedDeg);
            ImGui::Text("Target Speed: %.2f deg/s", m_rotationSpeed * m_rotationDirP1);
            ImGui::Text("Rotation Direction: %s", m_rotationDirP1 > 0 ? "CW (Positive)" : "CCW (Negative)");
        }
        else
        {
            ImGui::Text("--- Cube: Not Found ---");
        }
    }

    void CheckRotateScript::Save(engine::json& j) const
    {
        Object::Save(j);

        j["WaitDuration"] = m_waitDuration;
        j["RotationSpeed"] = m_rotationSpeed;
        j["MaxLinearSpeed"] = m_maxLinearSpeed;
        j["AccelerationForce"] = m_accelerationForce;
    }

    void CheckRotateScript::Load(const engine::json& j)
    {
        Object::Load(j);

        engine::JsonGet(j, "WaitDuration", m_waitDuration);
        engine::JsonGet(j, "RotationSpeed", m_rotationSpeed);
        engine::JsonGet(j, "MaxLinearSpeed", m_maxLinearSpeed);
        engine::JsonGet(j, "AccelerationForce", m_accelerationForce);
    }
}
