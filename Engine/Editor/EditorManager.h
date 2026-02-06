#pragma once

#include <unordered_set>
#include <ImGuizmo.h>

#include "Core/System/ProjectSettings.h"
#include "Framework/Object/Ptr.h"

namespace engine
{
    class GameObject;
    class EditorCamera;
    class EditorGrid;

    namespace GizmoState
    {
        inline ImGuizmo::OPERATION CurrentOperation = ImGuizmo::TRANSLATE;
        inline ImGuizmo::MODE CurrentMode = ImGuizmo::LOCAL;
    }

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
        Ptr<GameObject> m_selectedObject = nullptr;
        std::unordered_set<GameObject*> m_expandedNodes;  // 자동으로 펼쳐야 할 노드
        std::unique_ptr<EditorCamera> m_editorCamera = nullptr;
        std::unique_ptr<EditorGrid> m_editorGrid = nullptr;
        EditorState m_editorState = EditorState::Edit;

        ProjectSettings m_projectSettings;
        std::vector<std::string> m_cachedSceneFiles;
        std::string m_nextScenePending;
        std::string m_sceneToDelete;
        std::string m_sceneToRename;

        // editor setting
        json m_editorSettings;

        // prefab
        std::vector<std::string> m_cachedPrefabFiles;
        std::string m_prefabOverwriteTarget;
        Ptr<GameObject> m_prefabPendingCreateObj = nullptr;

        int m_selectedSceneIndex = -1;
        int m_selectedBuildSceneIndex = -1;

        bool m_shouldOpenUnsavedPopup = false;
        bool m_showEditorUI = true;

        // hierarchy search
        char m_hierarchySearchFilter[256] = "";

    private:
        EditorManager();
        ~EditorManager();

    public:
        void Initialize();
        void Update();
        void Render();
        void Shutdown();

        EditorState GetEditorState() const;
        EditorCamera* GetEditorCamera() const;

        GameObject* GetSelectedObject() const;
        void SetSelectedObject(GameObject* obj);  // 선택 시 부모 계층 자동 펼침

        void DrawEditorGrid();

    private:
        void DrawPlayController();
        void DrawEditorController();
        void DrawHierarchy();
        bool DrawEntityNode(GameObject* gameObject, int objectIndex);
        bool MatchesSearchFilter(GameObject* gameObject) const;
        void DrawInspector();
        void DrawDebugInfo();
        void DrawPrefabManager();
        void DrawGizmoToolbar();

        void TogglePlayStop();
        void TogglePauseResume();

        void ValidateSettingsList();
        void RefreshSceneFileCache();
        bool IsSceneDirty();
        void RequestSceneChange(const std::string& nextSceneName);
        void RequestNewScene();
        void CreateNewScene();
        void RefreshPrefabCache();

        void LoadEditorSettings();
        void SaveEditorSettings();

        void SaveSceneEditorData(const std::string& sceneName);
        void LoadSceneEditorData(const std::string& sceneName);

    private:
        friend class Singleton<EditorManager>;
    };
}