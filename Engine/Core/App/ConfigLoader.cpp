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
			catch (nlohmann::json_abi_v3_12_0::detail::parse_error& e)
			{
				e;

				LOG_INFO("파일 오류. 기본 값으로 다시 저장 후 불러옴.");
				Save(filePath, outSettings);
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