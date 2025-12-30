#pragma once

#include "Framework/Object/Component/Component.h"

namespace engine
{
    class Transform :
        public Component
    {
    private:
        Vector3 m_localPosition{ 0.0f, 0.0f, 0.0f };
        Quaternion m_localRotation = Quaternion::Identity;
        Vector3 m_localScale{ 1.0f, 1.0f, 1.0f };

        Matrix m_world{ Matrix::Identity };

        Transform* m_parent = nullptr;
        std::vector<Transform*> m_children;

        bool m_isDirty = true;
        bool m_isDirtyThisFrame = true;

    public:
        Transform() = default;
        ~Transform();

        static void* operator new(size_t size);
        static void operator delete(void* ptr);

    public:
        void Initialize() override;

    public:
        const Vector3& GetLocalPosition() const;
        const Quaternion& GetLocalRotation() const;
        const Vector3& GetLocalScale() const;

        Vector3 GetLocalEulerAngles() const;

        const Matrix& GetWorld();

        Vector3 GetForward() const;
        Vector3 GetUp() const;
        Vector3 GetRight() const;

        const std::vector<Transform*>& GetChildren() const;
        Transform* GetParent() const;

        bool IsDirtyThisFrame() const;

        void SetLocalPosition(const Vector3& position);
        void SetLocalRotation(const Quaternion& rotation);
        void SetLocalRotation(const Vector3& euler);
        void SetLocalScale(const Vector3& scale);
        void SetLocalScale(float scale);

        void SetParent(Transform* parent);

        void UnmarkDirtyThisFrame();
        bool IsAncestorOf(Transform* other) const;
        bool IsDescendantOf(Transform* other) const;

    public:
        void OnGui() override;
        void Save(json& j) const override;
        void Load(const json& j) override;
        std::string GetType() const override;

    private:
        void RecalculateWorldMatrix();
        void MarkDirty();
        void AddChild(Transform* child);
        void RemoveChild(Transform* child);
    };
}