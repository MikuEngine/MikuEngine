#include "GamePCH.h"
#include "MessageCatalog.h"
#include <Common/Utility/CSVReader.h>
#include <algorithm>

namespace game
{
    template<>
    bool FromFields<MessageRow>(const std::vector<std::string>& fields, MessageRow& out)
    {
        // key, channel, text, iconKey
        if (fields.size() != 4)
            return false;

        try
        {
            out.key = fields[0];

            const int chInt = std::stoi(fields[1]);
            out.ch = (UIMessageChannel)chInt;

            out.text = fields[2];
            out.iconKey = fields[3];
        }
        catch (...)
        {
            return false;
        }

        // 최소 검증
        if (out.key.empty()) return false;
        // text가 비어도 허용할지 여부
        // if (out.text.empty()) return false;

        return true;
    }

    bool MessageCatalog::Load(const std::string& csvPath)
    {
        std::vector<MessageRow> rows;

        auto parser = [](const std::vector<std::string>& fields, MessageRow& out) -> bool
            {
                if (fields.size() != 4)
                    return false;

                return FromFields<MessageRow>(fields, out);
            };

        if (!engine::CSVReader::Load<MessageRow>(csvPath, rows, parser))
        {
            LOG_PRINT("[MessageCatalog] Load failed: {}", csvPath);
            return false;
        }

        m_map.clear();
        m_map.reserve(rows.size());

        int dupCount = 0;
        for (auto& r : rows)
        {
            auto [it, inserted] = m_map.emplace(r.key, r);
            if (!inserted)
            {
                it->second = r; // 덮어쓰기
                dupCount++;
            }
        }

        LOG_PRINT("[MessageCatalog] Loaded: {} rows, map: {}, dup: {}, path: {}",
            (int)rows.size(), (int)m_map.size(), dupCount, csvPath);

        return true;
    }

	const MessageRow* MessageCatalog::Find(const std::string& key) const
	{
        auto it = m_map.find(key);
        if (it == m_map.end())
            return nullptr;
        return &it->second;
	}

	bool MessageCatalog::TryGet(const std::string& key, UIMessageChannel& outCh, std::string& outText, std::string& outIcon) const
	{
        const MessageRow* row = Find(key);
        if (!row) return false;

        outCh = row->ch;
        outIcon = row->iconKey;

        std::string formattedText = row->text;
        size_t pos = 0;
        while ((pos = formattedText.find("\\n", pos)) != std::string::npos)
        {
            formattedText.replace(pos, 2, "\n");
            pos += 1;
        }
        outText = formattedText;

        return true;
	}

    void MessageCatalog::CollectKeys(UIMessageChannel ch, const std::string& prefix, std::vector<std::string>& outKeys) const
    {
        outKeys.clear();

        for (const auto& [key, row] : m_map)
        {
            if (row.ch != ch)
            {
                continue;
            }

            if (!prefix.empty() && key.rfind(prefix, 0) != 0)
            {
                continue;
            }

            outKeys.push_back(key);
        }

        std::sort(outKeys.begin(), outKeys.end());
    }

	void MessageCatalog::Clear()
	{
        m_map.clear();
	}
}