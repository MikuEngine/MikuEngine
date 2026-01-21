#pragma once

#include "Framework/Asset/AssetData.h"

namespace engine
{
	class PrefabData :
		public AssetData
	{
	private:
		json m_prefabData;

	public:
		void Load(const std::string& path);
		void Save(const std::string& path);

		const json& GetData() const;
		void SetData(const json& data);
		void SetData(json&& data);
	};
}