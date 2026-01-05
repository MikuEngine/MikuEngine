#pragma once

#include <filesystem>

#include "Core/App/WindowSettings.h"

namespace engine
{
    struct ViewportTransformData
    {
        float viewX = 0.0f;
        float viewY = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float scaleX = 1.0f;
        float scaleY = 1.0f;
    };

    class WinApp
    {
    protected:
        HWND m_hWnd = nullptr;
        HINSTANCE m_hInstance = nullptr;
        HICON m_hIcon = nullptr;
        HCURSOR m_hCursor = nullptr;
        HICON m_hIconSmall = nullptr;

        std::string m_className = "game123";
        std::string m_windowName = "Default";
        std::filesystem::path m_settingFilePath;
        WindowSettings m_settings;
        ViewportTransformData m_viewportData;

        UINT m_classStyle = 0;
        DWORD m_windowStyle = 0;
        int m_x = 0;
        int m_y = 0;
        int m_screenWidth = 0;
        int m_screenHeight = 0;

    protected:
        WinApp(const std::filesystem::path& settingFilePath = "config.json", const WindowSettings& defaultSetting = {});
        virtual ~WinApp();

    public:
        virtual void Initialize();
        virtual void Shutdown();
        void Run();

    private:
        void Update();
        void Render();

        void GamePlayUpdate();

        void ValidateSettings();
        void SetWindowMode(bool isFullscreen);
        void SetResolution(int width, int height, bool isFullscreen);
        void UpdateViewportTransformData();

        bool HandleImGuiInput(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    protected:
        virtual LRESULT MessageProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        friend LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
    };
}
