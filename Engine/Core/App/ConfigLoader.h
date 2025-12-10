#pragma once

#include "WinApp.h"

namespace engine
{
	class ConfigLoader
	{
	public:
		static void Load(const std::string& filePath, WindowSettings& outSettings);
		static void Save(const std::string& filePath, const WindowSettings& settings);
	};
}