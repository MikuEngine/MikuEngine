#include "EnginePCH.h"
#include "PrefabData.h"

#include <fstream>
#include <iomanip>

#include "Core/System/VirtualFileSystem.h"

namespace engine
{
	void PrefabData::Load(const std::string& path)
	{
		auto& vfs = VirtualFileSystem::Get();
		std::vector<uint8_t> fileData;
		
		if (vfs.LoadFile(path, fileData))
		{
			m_prefabData = json::parse(fileData.begin(), fileData.end());
		}
	}

	void PrefabData::Save(const std::string& path)
	{
		std::ofstream file{ path };
		if (file.is_open())
		{
			file << std::setw(4) << m_prefabData << std::endl;
		}
	}

	const json& PrefabData::GetData() const
	{
		return m_prefabData;
	}

	void PrefabData::SetData(const json& data)
	{
		m_prefabData = data;
	}

	void PrefabData::SetData(json&& data)
	{
		m_prefabData = std::move(data);
	}
}