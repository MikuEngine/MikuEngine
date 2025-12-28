#pragma once

namespace engine
{
    class GameObject;
    class EditorCamera;

    enum class EditorState
    {
        Edit,
        Play,
        Pause
    };

    class EditorManager :
        public Singleton<EditorManager>
    {
    private:
        GameObject* m_selectedObject = nullptr;
        EditorState m_editorState = EditorState::Edit;
        std::unique_ptr<EditorCamera> m_editorCamera = nullptr;

    private:
        EditorManager();
        ~EditorManager();

    public:
        void Initialize();
        void Update();
        void Render();

        GameObject* GetSelectedObject() const;
        EditorState GetEditorState() const;
        EditorCamera* GetEditorCamera() const;

        void SetSelectedObject(GameObject* gameObject);

    private:
        void DrawEditorController();
        void DrawHierarchy();
        void DrawEntityNode(GameObject* gameObject);
        void DrawInspector();

    private:
        friend class Singleton<EditorManager>;
    };
}