#include "GamePCH.h"
#include "UpgradeDatabase.h"
#include <Common/Utility/CSVReader.h>

#include "Script/UpgradeNodeView.h"

namespace game
{
    namespace
    {
        static std::string Trim(std::string s)
        {
            auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
            while (!s.empty() && isSpace((unsigned char)s.front())) s.erase(s.begin());
            while (!s.empty() && isSpace((unsigned char)s.back()))  s.pop_back();
            return s;
        }

        static int ToInt(const std::string& s, int fb = 0)
        {
            try { return std::stoi(Trim(s)); }
            catch (...) { return fb; }
        }

        static float ToFloat(const std::string& s, float fb = 0.0f)
        {
            try { return std::stof(Trim(s)); }
            catch (...) { return fb; }
        }

        static bool ToBool01(const std::string& s, bool fb = false)
        {
            const std::string t = Trim(s);
            if (t == "1" || t == "true" || t == "True") return true;
            if (t == "0" || t == "false" || t == "False") return false;
            return fb;
        }

        static UpgradeCategory ParseCategory(const std::string& s)
        {
            const std::string t = Trim(s);
            if (t == "Attack") return UpgradeCategory::Attack;
            if (t == "Skill")  return UpgradeCategory::Skill;
            if (t == "Life" || t == "Survive") return UpgradeCategory::Life;
            if (t == "Move")   return UpgradeCategory::Move;
            return UpgradeCategory::Attack;
        }

        static std::vector<int> ParseParentsPipe(const std::string& raw)
        {
            std::vector<int> out;
            std::string token;

            for (char ch : raw)
            {
                if (ch == '|')
                {
                    token = Trim(token);
                    if (!token.empty())
                    {
                        out.push_back(ToInt(token, 0));
                        token.clear();
                    }
                }
                else
                {
                    token.push_back(ch);
                }
            }
            token = Trim(token);
            if (!token.empty())
                out.push_back(ToInt(token, 0));

            // 잘못된 값만 제거 (예: -1)
            out.erase(std::remove_if(out.begin(), out.end(),
                [](int v) { return v < 0 || v > 99; }), out.end());

            return out;
        }

        // ─────────────────────────────────────────────
        // Effect parsing
        // ─────────────────────────────────────────────
        static bool ParseEffectKind(const std::string& s, game::TemperEffect::Kind& out)
        {
            const std::string t = Trim(s);
            if (t == "Stat") { out = game::TemperEffect::Kind::Stat; return true; }
            if (t == "Buff") { out = game::TemperEffect::Kind::Buff; return true; }
            if (t == "Special") { out = game::TemperEffect::Kind::Special; return true; }
            return false;
        }

        static bool ParseOp(const std::string& s, game::TemperOp& out)
        {
            const std::string t = Trim(s);
            if (t == "Add") { out = game::TemperOp::Add;  return true; }
            if (t == "Mul") { out = game::TemperOp::Mul;  return true; }
            if (t == "Bool") { out = game::TemperOp::Bool; return true; }
            return false;
        }

        static bool ParseStatKey(const std::string& s, game::StatType& out)
        {
            const std::string t = Trim(s);

            // --- Attack ---
            if (t == "AtkDmg")          return (out = game::StatType::AtkDmg), true;
            if (t == "AtkSpeed")       return (out = game::StatType::AtkSpeed), true;
            if (t == "BulletRange")    return (out = game::StatType::BulletRange), true;
            if (t == "BulletSize")     return (out = game::StatType::BulletSize), true;
            if (t == "BulletSpeed")    return (out = game::StatType::BulletSpeed), true;

            // --- Execution ---
            if (t == "Exe_FragileRegen")     return (out = game::StatType::Exe_FragileRegen), true;
            if (t == "Exe_Range")            return (out = game::StatType::Exe_Range), true;
            if (t == "Exe_SplashDmg")        return (out = game::StatType::Exe_SplashDmg), true;
            if (t == "Exe_SplashRange")      return (out = game::StatType::Exe_SplashRange), true;
            if (t == "Exe_DashChargeRegen")  return (out = game::StatType::Exe_DashChargeRegen), true;
            if (t == "Exe_HpRegen")           return (out = game::StatType::Exe_HpRegen), true;

            // --- Vital ---
            if (t == "Hp_Max")               return (out = game::StatType::Hp_Max), true;
            if (t == "Hp_RegenOnClear")      return (out = game::StatType::Hp_RegenOnClear), true;
            if (t == "Fragile_Max")          return (out = game::StatType::Fragile_Max), true;
            if (t == "Fragile_RegenOnClear") return (out = game::StatType::Fragile_RegenOnClear), true;
            if (t == "Fragile_GainRate")     return (out = game::StatType::Fragile_GainRate), true;
            if (t == "InvincibleTime")       return (out = game::StatType::InvincibleTime), true;

            // --- Movement ---
            if (t == "MoveSpeed")            return (out = game::StatType::MoveSpeed), true;
            if (t == "Dash_Distance")        return (out = game::StatType::Dash_Distance), true;
            if (t == "Dash_Cooldown")        return (out = game::StatType::Dash_Cooldown), true;
            if (t == "Dash_InvincibleTime")  return (out = game::StatType::Dash_InvincibleTime), true;

            // --- Special ---
            if (t == "BulletDouble")         return (out = game::StatType::BulletDouble), true;

            return false;
        }

        enum class BuffKey
        {
            DashDuration,
            DashMoveSpeedBonus,

            ExecDuration,
            ExecAtkSpeedBonus,
            ExecMaxStacks,
        };

        static bool ParseBuffId(const std::string& s, game::BuffId& out)
        {
            const std::string t = Trim(s);

            if (t == "Dash_MoveSpeed")     return (out = game::BuffId::Dash_MoveSpeed), true;
            if (t == "Dash_AtkDmg")        return (out = game::BuffId::Dash_AtkDmg), true;
            if (t == "Execution_AtkSpeed") return (out = game::BuffId::Execution_AtkSpeed), true;

            return false;
        }

        static bool ParseBuffField(const std::string& s, game::BuffField& out)
        {
            const std::string t = Trim(s);

            if (t == "Duration") return (out = game::BuffField::Duration), true;
            if (t == "Bonus")    return (out = game::BuffField::Bonus), true;

            return false;
        }

        static bool ParseBuffKey(const std::string& keyStr, game::BuffId& outBuff, game::BuffField& outField)
        {
            const std::string t = Trim(keyStr);

            const size_t dot = t.find('.');
            if (dot == std::string::npos)
                return false;

            const std::string left = t.substr(0, dot);       // BuffId
            const std::string right = t.substr(dot + 1);     // Field

            if (!ParseBuffId(left, outBuff))
                return false;

            if (!ParseBuffField(right, outField))
                return false;

            return true;
        }

        enum class SpecialKey
        {
            BulletDouble
        };

        static bool ParseSpecialKey(const std::string& s, SpecialKey& out)
        {
            const std::string t = Trim(s);
            if (t == "BulletDouble") return (out = SpecialKey::BulletDouble), true;
            return false;
        }

        static void ReplaceAll(std::string& s, const std::string& from, const std::string& to)
        {
            if (from.empty()) return;
            size_t pos = 0;
            while ((pos = s.find(from, pos)) != std::string::npos)
            {
                s.replace(pos, from.size(), to);
                pos += to.size();
            }
        }

        static std::string UnescapeNewlines(std::string s)
        {
            // CSV에 \n 으로 적어둔 것을 실제 개행으로 변환
            ReplaceAll(s, "\\r\\n", "\n");
            ReplaceAll(s, "\\n", "\n");
            ReplaceAll(s, "\\r", "\n");
            return s;
        }
    }

    template<>
    bool FromFields<UpgradeNodeRow>(const std::vector<std::string>& fields, UpgradeNodeRow& out)
    {
        // 고정 포맷(효과 4개)
        // 0 NodeId
        // 1 Category
        // 2 Name
        // 3 Desc
        // 4 Ruby = book
        // 5 Sapphire = crystal1
        // 6 Emerald = crystal2
        // 7 Parents = raw문자열
        // 8~: E1Kind,E1Op,E1Key,E1Value,E1Bool ... (5칸 * 4)
        constexpr int kBaseCols = 8;
        constexpr int kEffectSlots = 3;
        constexpr int kColsPerEffect = 5;

        const int expected = kBaseCols + kEffectSlots * kColsPerEffect;
        if ((int)fields.size() != expected)
            return false;

        try
        {
            const int localId = ToInt(fields[0], -1);
            if (localId < 0 || localId > 99) return false;

            out.category = ParseCategory(fields[1]);
            const int base = (int)out.category;         // Attack=100 같은 값
            out.nodeId = base + localId;                // fullId

            out.name = fields[2];
            out.desc = UnescapeNewlines(fields[3]);
            //out.desc = fields[3];

            out.ruby = std::max(0, ToInt(fields[4], 0));
            out.sapphire = std::max(0, ToInt(fields[5], 0));
            out.emerald = std::max(0, ToInt(fields[6], 0));

            out.parentsRaw = fields[7];

            const auto parentsLocal = ParseParentsPipe(out.parentsRaw);
            out.parents.clear();
            out.parents.reserve(parentsLocal.size());
            for (int pl : parentsLocal)
            {
                // parents도 로컬로 적는 정책이면 같은 카테고리 base를 더해줌
                if (pl < 0 || pl > 99) continue;
                out.parents.push_back(base + pl);
            }

            out.effects.clear();

            int idx = kBaseCols;
            for (int i = 0; i < kEffectSlots; ++i)
            {
                const std::string kindStr = fields[idx + 0];
                const std::string opStr = fields[idx + 1];
                const std::string keyStr = fields[idx + 2];
                const std::string valStr = fields[idx + 3];
                const std::string bStr = fields[idx + 4];
                idx += kColsPerEffect;

                // 빈 슬롯(Kind가 비어있으면 스킵)
                if (Trim(kindStr).empty())
                    continue;

                game::TemperEffect e;
                e.kind = game::TemperEffect::Kind::Stat; // 임시 초기값

                if (!ParseEffectKind(kindStr, e.kind))
                    continue;

                if (!ParseOp(opStr, e.op))
                    continue;

                e.value = ToFloat(valStr, 0.0f);
                e.b = ToBool01(bStr, false);

                // Kind별로 key 해석
                if (e.kind == game::TemperEffect::Kind::Stat)
                {
                    game::StatType st{};
                    if (!ParseStatKey(keyStr, st))
                        continue;

                    e.stat = st;
                }
                else if (e.kind == game::TemperEffect::Kind::Special)
                {
                    // 예: BulletDouble
                    SpecialKey sk{};
                    if (!ParseSpecialKey(keyStr, sk))
                        continue;

                    // Special은 기존 로직과 호환 위해
                    // - BulletDouble이면 op=Bool / b 사용
                    // - 또는 value로도 처리 가능(정책 선택)
                    // 여기서는 “BulletDouble은 Bool로 켜고 끄기”로 가정
                    e.stat = game::StatType::BulletDouble;
                }
                else if (e.kind == game::TemperEffect::Kind::Buff)
                {
                    game::BuffId bid{};
                    game::BuffField bf{};

                    if (!ParseBuffKey(keyStr, bid, bf))
                        continue;

                    e.buff = bid;
                    e.field = bf;
                }

                out.effects.push_back(e);
            }
        }
        catch (...)
        {
            return false;
        }

        // 최소 검증
        if (out.name.empty()) return false;

        return true;
    }

	bool UpgradeDatabase::Load(const std::string& csvPath)
	{
        std::vector<UpgradeNodeRow> rows;

        auto parser = [](const std::vector<std::string>& fields, UpgradeNodeRow& out) -> bool
            {
                return FromFields<UpgradeNodeRow>(fields, out);
            };

        if (!engine::CSVReader::Load<UpgradeNodeRow>(csvPath, rows, parser))
        {
            LOG_PRINT("[UpgradeCatalog] Load failed: {}", csvPath);
            return false;
        }

        m_map.clear();
        m_map.reserve(rows.size());

        int dupCount = 0;
        for (auto& r : rows)
        {
            auto [it, inserted] = m_map.emplace(r.nodeId, r);
            if (!inserted)
            {
                it->second = r; // 덮어쓰기 (MessageCatalog과 동일)
                dupCount++;
            }
        }

        LOG_PRINT("[UpgradeCatalog] Loaded: {} rows, map: {}, dup: {}, path: {}",
            (int)rows.size(), (int)m_map.size(), dupCount, csvPath);

		return true;
	}

	const UpgradeNodeRow* UpgradeDatabase::Find(int nodeId) const
	{
        auto it = m_map.find(nodeId);
        if (it == m_map.end())
            return nullptr;
        return &it->second;
	}

	bool UpgradeDatabase::TryGet(int nodeId, const UpgradeNodeRow*& outRow) const
	{
        outRow = Find(nodeId);
        return (outRow != nullptr);
	}

	void UpgradeDatabase::Clear()
	{
        m_map.clear();
	}
}
