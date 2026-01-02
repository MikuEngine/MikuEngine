#pragma once

#include "Core/System/ProjectSettings.h"

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
        std::unique_ptr<EditorCamera> m_editorCamera = nullptr;
        EditorState m_editorState = EditorState::Edit;

        ProjectSettings m_projectSettings;
        std::vector<std::string> m_cachedSceneFiles;
        std::string m_nextScenePending;
        std::string m_sceneToDelete;
        std::string m_sceneToRename;

        int m_selectedSceneIndex = -1;
        int m_selectedBuildSceneIndex = -1;

        bool m_shouldOpenUnsavedPopup = false;


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
        void DrawPlayController();
        void DrawEditorController();
        void DrawHierarchy();
        void DrawEntityNode(GameObject* gameObject);
        void DrawInspector();
        void DrawDebugInfo();

        void ValidateSettingsList();
        void RefreshFileCache();
        bool IsSceneDirty();
        void RequestSceneChange(const std::string& nextSceneName);
        void RequestNewScene();
        void CreateNewScene();

    private:
        friend class Singleton<EditorManager>;
    };
}