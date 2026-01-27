#include "GamePCH.h"

#include "App/ComponentRegistry.h"
#include "App/FragileFall.h"

int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	engine::LeakCheck lc;

	game::FragileFall app("Settings.config");

	app.Initialize();
	app.Run();
	app.Shutdown();
}