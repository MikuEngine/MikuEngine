#include "pch.h"
#include "ConfigLoader.h"

#include <json.hpp>

namespace engine
{
	using json = nlohmann::json;

	NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WindowSettings, width, height, isFullScreen, isResizable, useVsync)

	void ConfigLoader::Load(const std::string& filePath, WindowSettings& outSettings)
	{
		if (!std::filesystem::exists(filePath))
		{
			Save(filePath, outSettings);
			return;
		}

		std::ifstream file(filePath);
		if (file.is_open())
		{
			try
			{
				json j;
				file >> j;
				outSettings = j.get<WindowSettings>();
			}
			catch (...)
			{
				FATAL_CHECK(false, std::format("config 파일 로드 실패. {}을 지웠다가 다시 시작해주세요.", filePath));
			}
		}
	}

	void ConfigLoader::Save(const std::string& filePath, const WindowSettings& settings)
	{
		std::filesystem::path path(filePath);

		if (path.has_parent_path())
		{
			std::error_code ec;
			std::filesystem::create_directories(path.parent_path(), ec);
		}

		std::ofstream file(filePath);
		if (file.is_open())
		{
			json j = settings;
			file << j.dump(4);
		}
	}
}