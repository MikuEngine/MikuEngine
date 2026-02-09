#include "EnginePCH.h"
#include "Transform.h"

#include <imgui.h>
#include <ImGuizmo.h>

#include "Common/Math/MathUtility.h"
#include "Common/Utility/JsonHelper.h"
#include "Common/Utility/StaticMemoryPool.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/TransformSystem.h"
#include "Framework/System/CameraSystem.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/RectTransform.h"
#include "Framework/Object/Component/Collider.h"
#include "Editor/EditorManager.h"
#include "Editor/EditorCamera.h"

namespace engine
{
    namespace
    {
        StaticMemoryPool<Transform, 1024> g_transformPool;
    }

    Transform::~Transform()
    {
        if (m_parent != nullptr)
        {
            m_parent->RemoveChild(this);
        }

        for (Transform* child : m_children)
        {
            child->m_parent = nullptr;
            child->MarkDirty();
        }

        SystemManager::Get().GetTransformSystem().Unregister(this);
    }

    void* Transform::operator new(size_t size)
    {
        return g_transformPool.Allocate(size);
    }

    void Transform::operator delete(void* ptr)
    {
        g_transformPool.Deallocate(ptr);
    }

    void Transform::Initialize()
    {
        SystemManager::Get().GetTransformSystem().Register(this);
    }

    const Vector3& Transform::GetLocalPosition() const
    {
        return m_localPosition;
    }

    const Quaternion& Transform::GetLocalRotation() const
    {
        return m_localRotation;
    }

    const Vector3& Transform::GetLocalScale() const
    {
        return m_localScale;
    }

    Vector3 Transform::GetWorldPosition()
    {
        return GetWorld().Translation();
    }

    Quaternion Transform::GetWorldRotation()
    {
        if (m_parent != nullptr)
        {
            // World = Parent * Local (부모 회전 먼저, 그 다음 자식 로컬 회전)
            return m_parent->GetWorldRotation() * m_localRotation;
        }
        return m_localRotation;
    }

    Vector3 Transform::GetWorldScale()
    {
        if (m_parent != nullptr)
        {
            Vector3 parentScale = m_parent->GetWorldScale();
            return Vector3(
                m_localScale.x * parentScale.x,
                m_localScale.y * parentScale.y,
                m_localScale.z * parentScale.z
            );
        }
        return m_localScale;
    }

    Vector3 Transform::GetLocalEulerAngles() const
    {
        return m_localEulerRotation;
    }

    const Matrix& Transform::GetWorld()
    {
        if (m_isDirty)
        {
            RecalculateWorldMatrix();
        }

        return m_world;
    }

    Vector3 Transform::GetForward()
    {
        return engine::GetForward(GetWorld());
    }

    Vector3 Transform::GetUp()
    {
        return engine::GetUp(GetWorld());
    }

    Vector3 Transform::GetRight()
    {
        return engine::GetRight(GetWorld());
    }

    bool Transform::IsDirtyThisFrame() const
    {
        return m_isDirtyThisFrame;
    }

    void Transform::SetLocalPosition(const Vector3& position)
    {
        m_localPosition = position;

        MarkDirty();
    }

    void Transform::SetLocalRotation(const Quaternion& rotation)
    {
        m_localRotation = rotation;

        Vector3 euler = rotation.ToEuler();
        m_localEulerRotation.x = ToDegree(euler.x);
        m_localEulerRotation.y = ToDegree(euler.y);
        m_localEulerRotation.z = ToDegree(euler.z);

        MarkDirty();
    }

    void Transform::SetLocalRotation(const Vector3& euler)
    {
        m_localRotation = Quaternion::CreateFromYawPitchRoll(
            ToRadian(euler.y),
            ToRadian(euler.x),
            ToRadian(euler.z));

        m_localEulerRotation = euler;

        MarkDirty();
    }

    void Transform::SetLocalScale(const Vector3& scale)
    {
        m_localScale = scale;

        MarkDirty();
    }

    void Transform::SetLocalScale(float scale)
    {
        m_localScale.x = scale;
        m_localScale.y = scale;
        m_localScale.z = scale;

        MarkDirty();
    }

    void Transform::SetParent(Transform* parent, bool worldPositionStays)
    {
        // World 좌표 저장 (부모 변경 전)
        Vector3 savedWorldPosition;
        Quaternion savedWorldRotation;
        Vector3 savedWorldScale;
        
        if (worldPositionStays)
        {
            savedWorldPosition = GetWorldPosition();
            savedWorldRotation = GetWorldRotation();
            savedWorldScale = GetWorldScale();
        }

        // 기존 부모에서 분리
        if (m_parent != nullptr)
        {
            m_parent->RemoveChild(this);
        }

        // 새 부모 설정
        m_parent = parent;
        bool parentActive = true;

        if (m_parent != nullptr)
        {
            m_parent->AddChild(this);
            parentActive = m_parent->GetGameObject()->IsActive();
        }

        // World 좌표 유지를 위해 Local 좌표 재계산
        if (worldPositionStays)
        {
            if (m_parent != nullptr)
            {
                // 행렬 역변환 방식 사용 (비균등 스케일에 더 정확)
                Matrix parentWorldInverse = m_parent->GetWorld().Invert();
                
                // 저장된 World 변환으로 행렬 생성
                Matrix savedWorldMatrix = Matrix::CreateScale(savedWorldScale) *
                                          Matrix::CreateFromQuaternion(savedWorldRotation) *
                                          Matrix::CreateTranslation(savedWorldPosition);
                
                // Local = World * inv(Parent)
                Matrix localMatrix = savedWorldMatrix * parentWorldInverse;
                
                // 행렬 분해
                Vector3 newLocalScale;
                Quaternion newLocalRotation;
                Vector3 newLocalPosition;
                
                if (localMatrix.Decompose(newLocalScale, newLocalRotation, newLocalPosition))
                {
                    m_localPosition = newLocalPosition;
                    m_localRotation = newLocalRotation;
                    m_localRotation.Normalize();
                    m_localScale = newLocalScale;
                }
                else
                {
                    // Decompose 실패 시 (비균등 스케일 + 회전으로 인한 Shearing)
                    // 간단한 fallback: 위치만 정확히 계산하고 나머지는 유지
                    LOG_INFO("[Transform] Matrix decompose failed - non-uniform scale with rotation may cause shearing");
                    m_localPosition = Vector3(localMatrix._41, localMatrix._42, localMatrix._43);
                    m_localRotation = savedWorldRotation;
                    m_localScale = savedWorldScale;
                }

                // 오일러 각도 업데이트
                m_localEulerRotation = m_localRotation.ToEuler() * (180.0f / DirectX::XM_PI);
            }
            else
            {
                // 부모 없음: World = Local
                m_localPosition = savedWorldPosition;
                m_localRotation = savedWorldRotation;
                m_localScale = savedWorldScale;
                m_localEulerRotation = m_localRotation.ToEuler() * (180.0f / DirectX::XM_PI);
            }
        }

        GetGameObject()->UpdateActiveInHierarchy(parentActive);

        MarkDirty();

        // 물리 컴포넌트에 부모 변경 알림
        NotifyPhysicsComponentsParentChanged();
    }

    void Transform::SetWorldMatrix(const Matrix& worldMatrix)
    {
        Matrix targetLocalMatrix;

        if (m_parent != nullptr)
        {
            Matrix parentWorldInverse = m_parent->GetWorld().Invert();
            targetLocalMatrix = worldMatrix * parentWorldInverse;
        }
        else
        {
            // 부모가 없으면 worldMatrix 자체가 곧 localMatrix입니다.
            targetLocalMatrix = worldMatrix;
        }

        Vector3 newLocalScale;
        Quaternion newLocalRotation;
        Vector3 newLocalPosition;

        if (targetLocalMatrix.Decompose(newLocalScale, newLocalRotation, newLocalPosition))
        {
            m_localPosition = newLocalPosition;
            m_localRotation = newLocalRotation;
            m_localRotation.Normalize();
            m_localScale = newLocalScale;
        }
        else
        {
            // Decompose 실패 시 최소한 위치라도 보정
            m_localPosition = Vector3(targetLocalMatrix._41, targetLocalMatrix._42, targetLocalMatrix._43);
        }

        // 오일러 각도 업데이트
        m_localEulerRotation = m_localRotation.ToEuler() * (180.0f / DirectX::XM_PI);
        MarkDirty();
    }

    void Transform::NotifyPhysicsComponentsParentChanged()
    {
        // 이 GameObject의 모든 Collider에 알림
        for (const auto& component : GetGameObject()->GetComponents())
        {
            if (component->Is<Collider>())
            {
                static_cast<Collider*>(component.get())->OnParentChanged();
            }
        }

        // 자식들에게도 재귀적으로 알림 (자식 Collider가 부모 Rigidbody를 찾아야 함)
        for (Transform* child : m_children)
        {
            child->NotifyPhysicsComponentsParentChanged();
        }
    }

    const std::vector<Transform*>& Transform::GetChildren() const
    {
        return m_children;
    }

    Transform* Transform::GetParent() const
    {
        return m_parent;
    }

    void Transform::UnmarkDirtyThisFrame()
    {
        m_isDirtyThisFrame = false;
    }

    bool Transform::IsAncestorOf(Transform* other) const
    {
        Transform* current = other;
        while (current != nullptr)
        {
            if (current == this)
            {
                return true;
            }

            current = current->GetParent();
        }

        return false;
    }

    bool Transform::IsDescendantOf(Transform* other) const
    {
        if (other == nullptr)
        {
            return false;
        }

        Transform* current = m_parent;
        while (current != nullptr)
        {
            if (current == other)
            {
                return true;
            }

            current = current->GetParent();
        }
        return false;
    }

    void Transform::Rotate(const Vector3& axis, float angleDegree, bool isLocal)
    {
        float radian = ToRadian(angleDegree);

        Vector3 normalizedAxis = axis;
        normalizedAxis.Normalize();

        Quaternion deltaRotation = Quaternion::CreateFromAxisAngle(normalizedAxis, radian);

        if (isLocal)
        {
            m_localRotation = m_localRotation * deltaRotation;
        }
        else
        {
            m_localRotation = deltaRotation * m_localRotation;
        }

        m_localRotation.Normalize();

        MarkDirty();
    }

    void Transform::Translate(const Vector3& translation, bool isLocal)
    {
        if (isLocal)
        {
            Vector3 localTranslation = Vector3::Transform(translation, m_localRotation);

            m_localPosition += localTranslation;
        }
        else
        {
            m_localPosition += translation;
        }

        MarkDirty();
    }

    GameObject* Transform::FindChildByName(const std::string& name)
    {
        for (Transform* child : m_children)
        {
            if (child->GetGameObject()->GetName() == name)
            {
                return child->GetGameObject();
            }
        }
        return nullptr;
    }

    GameObject* Transform::FindChildByNameRecursive(const std::string& name)
    {
        for (Transform* child : m_children)
        {
            // 1. 현재 자식의 이름이 일치하는지 확인
            if (child->GetGameObject()->GetName() == name)
            {
                return child->GetGameObject();
            }

            // 2. 일치하지 않으면 해당 자식의 자식들을 재귀적으로 탐색
            GameObject* found = child->FindChildByNameRecursive(name);
            if (found != nullptr)
            {
                return found;
            }
        }
        return nullptr;
    }

    void Transform::OnGui()
    {
        ImGui::Indent();
        
        if (ImGui::DragFloat3("Position", &m_localPosition.x, 0.1f))
        {
            MarkDirty();
        }

        if (auto euler = GetLocalEulerAngles(); ImGui::DragFloat3("Rotation", &euler.x, 0.1f))
        {
            SetLocalRotation(euler);
        }

        // 자식이 있으면 균등 스케일만 허용 (비균등 스케일 + 회전 시 Shearing 방지)
        bool hasChildren = !m_children.empty();
        
        if (hasChildren)
        {
            // 균등 스케일: 하나의 값으로 모든 축 제어
            float uniformScale = m_localScale.x;
            if (ImGui::DragFloat("Scale (Uniform)", &uniformScale, 0.1f, 0.001f, 1000.0f))
            {
                m_localScale = Vector3(uniformScale, uniformScale, uniformScale);
                MarkDirty();
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("This object has children. Only uniform scale is allowed to prevent shearing.");
            }
        }
        else
        {
            // Leaf 노드: 비균등 스케일 허용
            if (ImGui::DragFloat3("Scale", &m_localScale.x, 0.1f))
            {
                MarkDirty();
            }
        }
        
        ImGui::Unindent();
        
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
        ImGui::Separator();
        ImGui::PopStyleColor();
        ImGui::Spacing();
        ImGui::Spacing();

        Matrix worldMatrix = GetWorld();
        Matrix view;
        Matrix proj;

        if (EditorManager::Get().GetEditorState() != EditorState::Play)
        {
            auto camera = EditorManager::Get().GetEditorCamera();
            view = camera->GetView();
            proj = camera->GetProjection();
        }
        else
        {
            auto camera = SystemManager::Get().GetCameraSystem().GetMainCamera();
            view = camera->GetView();
            proj = camera->GetProjection();
        }

        if (ImGuizmo::Manipulate(
            &view._11, &proj._11,
            GizmoState::CurrentOperation,
            GizmoState::CurrentMode,
            &worldMatrix._11))
        {
            SetWorldMatrix(worldMatrix);
        }
    }

    void Transform::Save(json& j) const
    {
        Object::Save(j);

        j["Position"] = m_localPosition;
        j["Rotation"] = m_localRotation;
        j["Scale"] = m_localScale;
    }

    void Transform::Load(const json& j)
    {
        Object::Load(j);

        JsonGet(j, "Position", m_localPosition);
        JsonGet(j, "Rotation", m_localRotation);
        SetLocalRotation(m_localRotation);
        JsonGet(j, "Scale", m_localScale);
    }

    void Transform::RecalculateWorldMatrix()
    {
        const Matrix local = Matrix::CreateScale(m_localScale) *
            Matrix::CreateFromQuaternion(m_localRotation) *
            Matrix::CreateTranslation(m_localPosition);

        if (m_parent != nullptr)
        {
            m_world = local * m_parent->GetWorld();
        }
        else
        {
            m_world = local;
        }

        m_isDirty = false;
    }

    void Transform::MarkDirty()
    {
        if (m_isDirty)
        {
            return;
        }
        
        m_isDirty = true;
        m_isDirtyThisFrame = true;
        
        for (auto child : m_children)
        {
            child->MarkDirty();
        }

        if (auto* rt = GetGameObject()->GetComponent<RectTransform>())
            rt->MarkUIDirty();
    }

    void Transform::AddChild(Transform* child)
    {
        // 자식 추가 시 비균등 스케일이면 균등 스케일로 강제 변환 (Shearing 방지)
        bool isNonUniformScale = 
            std::abs(m_localScale.x - m_localScale.y) > 0.0001f ||
            std::abs(m_localScale.y - m_localScale.z) > 0.0001f;
        
        if (isNonUniformScale)
        {
            // 가장 큰 값으로 균등 스케일 적용
            float maxScale = std::max({m_localScale.x, m_localScale.y, m_localScale.z});
            m_localScale = Vector3(maxScale, maxScale, maxScale);
            LOG_INFO("[Transform] Non-uniform scale converted to uniform ({}) when adding child", maxScale);
            MarkDirty();
        }
        
        m_children.push_back(child);
    }

    void Transform::RemoveChild(Transform* child)
    {
        std::erase(m_children, child);
    }

    void Transform::ReorderChild(Transform* child, int newIndex)
    {
        if (!child)
        {
            return;
        }

        // 현재 인덱스 찾기
        int currentIndex = -1;
        for (size_t i = 0; i < m_children.size(); ++i)
        {
            if (m_children[i] == child)
            {
                currentIndex = static_cast<int>(i);
                break;
            }
        }

        if (currentIndex == -1)
        {
            return; // 자식을 찾을 수 없음
        }

        // 새 인덱스 범위 검사
        if (newIndex < 0)
        {
            newIndex = 0;
        }
        else if (newIndex >= static_cast<int>(m_children.size()))
        {
            newIndex = static_cast<int>(m_children.size()) - 1;
        }

        // 같은 위치면 변경 불필요
        if (currentIndex == newIndex)
        {
            return;
        }

        // 순서 변경: 현재 요소를 임시로 저장하고 제거한 후 새 위치에 삽입
        Transform* temp = m_children[currentIndex];
        
        if (currentIndex < newIndex)
        {
            // 뒤로 이동: 현재 위치부터 새 위치까지 앞으로 shift
            for (int i = currentIndex; i < newIndex; ++i)
            {
                m_children[i] = m_children[i + 1];
            }
        }
        else
        {
            // 앞으로 이동: 새 위치부터 현재 위치까지 뒤로 shift
            for (int i = currentIndex; i > newIndex; --i)
            {
                m_children[i] = m_children[i - 1];
            }
        }
        
        m_children[newIndex] = temp;
    }
}