#include "EnginePCH.h"
#include "Transform.h"

#include <imgui.h>

#include "Common/Math/MathUtility.h"
#include "Common/Utility/JsonHelper.h"
#include "Common/Utility/StaticMemoryPool.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/TransformSystem.h"
#include "Framework/Object/GameObject/GameObject.h"

namespace engine
{
    namespace
    {
        StaticMemoryPool<Transform, 4096> g_transformPool;
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

    Vector3 Transform::GetLocalEulerAngles() const
    {
        Vector3 euler = m_localRotation.ToEuler();
        euler.x = ToDegree(euler.x);
        euler.y = ToDegree(euler.y);
        euler.z = ToDegree(euler.z);

        return euler;
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

        MarkDirty();
    }

    void Transform::SetLocalRotation(const Vector3& euler)
    {
        m_localRotation = Quaternion::CreateFromYawPitchRoll(
            ToRadian(euler.y),
            ToRadian(euler.x),
            ToRadian(euler.z));

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

    void Transform::SetParent(Transform* parent)
    {
        if (m_parent != nullptr)
        {
            m_parent->RemoveChild(this);
        }

        m_parent = parent;
        bool parentActive = true;

        if (m_parent != nullptr)
        {
            m_parent->AddChild(this);
            parentActive = m_parent->GetGameObject()->IsActive();
        }

        GetGameObject()->UpdateActiveInHierarchy(parentActive);

        MarkDirty();
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

    void Transform::OnGui()
    {
        if (ImGui::DragFloat3("Position", &m_localPosition.x, 0.1f))
        {
            MarkDirty();
        }

        if (auto euler = GetLocalEulerAngles(); ImGui::DragFloat3("Rotation", &euler.x, 0.1f))
        {
            SetLocalRotation(euler);
        }

        if (ImGui::DragFloat3("Scale", &m_localScale.x, 0.1f))
        {
            MarkDirty();
        }
    }

    void Transform::Save(json& j) const
    {
        j["Type"] = "Transform";
        j["Position"] = m_localPosition;
        j["Rotation"] = m_localRotation;
        j["Scale"] = m_localScale;
    }

    void Transform::Load(const json& j)
    {
        JsonGet(j, "Position", m_localPosition);
        JsonGet(j, "Rotation", m_localRotation);
        JsonGet(j, "Scale", m_localScale);
    }

    std::string Transform::GetType() const
    {
        return "Transform";
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
    }

    void Transform::AddChild(Transform* child)
    {
        m_children.push_back(child);
    }

    void Transform::RemoveChild(Transform* child)
    {
        std::erase(m_children, child);
    }
}