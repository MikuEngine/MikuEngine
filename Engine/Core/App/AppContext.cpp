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
		return *g_app;
	}
}