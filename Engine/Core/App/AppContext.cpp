#include "EnginePCH.h"
#include "AppContext.h"

namespace engine
{
	namespace
	{
		WinApp* g_app = nullptr;
	}

	void AppContext::SetApp(WinApp* app)
	{
		g_app = app;
	}

	WinApp& AppContext::GetApp()
	{
		FATAL_CHECK(g_app != nullptr, "AppContext::GetApp() called before SetApp()");
		return *g_app;
	}
}