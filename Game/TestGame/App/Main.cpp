#include "GamePCH.h"

#include <Core/App/AppContext.h>

#include "App/ComponentRegistry.h"
#include "App/TestGameApp.h"

int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	engine::LeakCheck lc;

	game::TestGameApp app("Windows.config");
	engine::AppContext::SetApp(&app);

	app.Initialize();
	app.Run();
	app.Shutdown();
}
