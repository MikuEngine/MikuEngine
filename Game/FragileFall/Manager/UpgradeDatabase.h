#pragma once
#include "Script/UpgradeNodeView.h"

namespace engine
{
	class CSVReader;
}

namespace game
{
    enum class UpgradeCategory;

    class UpgradeNodeView;

    struct UpgradeNodeRow
    {
        int nodeId = 0;
        UpgradeCategory category = UpgradeCategory::Attack;

        std::string name;
        std::string desc;

        int ruby = 0;
        int sapphire = 0;
        int emerald = 0;

        // "100|101|" 같은 raw 문자열
        std::string parentsRaw;

        // 파싱된 결과(편의)
        std::vector<int> parents;

        // 효과(파싱된 결과)
        std::vector<game::TemperEffect> effects;
    };

    template<typename T>
    bool FromFields(const std::vector<std::string>& fields, T& out);

    template<>
    bool FromFields<UpgradeNodeRow>(const std::vector<std::string>& fields, UpgradeNodeRow& out);

	class UpgradeDatabase
	{
    public:
        bool Load(const std::string& csvPath);

        const UpgradeNodeRow* Find(int nodeId) const;

        bool TryGet(int nodeId, const UpgradeNodeRow*& outRow) const;

        void Clear();

    private:
        std::unordered_map<int, UpgradeNodeRow> m_map;
	};
}