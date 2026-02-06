#pragma once

namespace engine
{
	class CSVReader;
}

namespace game
{
	enum class UIMessageChannel;

	struct MessageRow
	{
		std::string key;
		UIMessageChannel ch{};
		std::string text;
		std::string iconKey;
	};

	template<typename T>
	bool FromFields(const std::vector<std::string>& fields, T& out);

	template<>
	bool FromFields<MessageRow>(const std::vector<std::string>& fields, MessageRow& out);

	class MessageCatalog
	{
	public:
		bool Load(const std::string& csvPath);

		const MessageRow* Find(const std::string& key) const;

		// Key로 접근
		bool TryGet(const std::string& key, UIMessageChannel& outCh, std::string& outText, std::string& outIcon) const;

		void Clear();

	private:
		std::unordered_map<std::string, MessageRow> m_map;
	};

}



