#include "pch.h"
#include "Transform.h"

#include <imgui.h>

#include "Common/Math/MathUtility.h"
#include "Common/Utility/JsonHelper.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/TransformSystem.h"

namespace engine
{
    Transform::Transform()
    {
        SystemManager::Get().GetTransformSystem().Register(this);
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

    Vector3 Transform::GetForward() const
    {
        return engine::GetForward(m_world);
    }

    Vector3 Transform::GetUp() const
    {
        return engine::GetUp(m_world);
    }

    Vector3 Transform::GetRight() const
    {
        return engine::GetRight(m_world);
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

        if (m_parent != nullptr)
        {
            m_parent->AddChild(this);
        }

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

    bool Transform::IsDescendantOf(Transform* other) const
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

    void Transform::OnGui()
    {
        Vector3 rotation = GetLocalEulerAngles();

        ImGui::DragFloat3("Position##Transform", &m_localPosition.x, 0.1f);
        if (ImGui::DragFloat3("Rotation##Transform", &rotation.x, 0.1f))
        {
            SetLocalRotation(rotation);
        }
        ImGui::DragFloat3("Scale##Transform", &m_localScale.x, 0.1f);
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