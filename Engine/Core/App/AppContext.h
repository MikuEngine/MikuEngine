#pragma once

namespace engine
{
    class WinApp;

    class AppContext
    {
    public:
        static void SetApp(WinApp* app);
        static WinApp& GetApp();
    };
}