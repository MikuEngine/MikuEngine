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

        // 주인님 프로젝트 enum 이름에 맞춰 채우시면 됩니다.
        // (여기서는 “문자열 키”를 CSV에서 쓰는 전제로 샘플만 넣었습니다.)
        static bool ParseStatKey(const std::string& s, game::StatType& out)
        {
            const std::string t = Trim(s);

            if (t == "AtkDmg") return (out = game::StatType::AtkDmg), true;
            if (t == "AtkSpeed") return (out = game::StatType::AtkSpeed), true;
            if (t == "BulletRange") return (out = game::StatType::BulletRange), true;
            if (t == "BulletSize") return (out = game::StatType::BulletSize), true;
            if (t == "BulletSpeed") return (out = game::StatType::BulletSpeed), true;

            if (t == "MoveSpeed") return (out = game::StatType::MoveSpeed), true;
            if (t == "Dash_Distance") return (out = game::StatType::Dash_Distance), true;
            if (t == "Dash_Cooldown") return (out = game::StatType::Dash_Cooldown), true;

            if (t == "BulletDouble") return (out = game::StatType::BulletDouble), true;

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

        static bool ParseBuffKey(const std::string& s, BuffKey& out)
        {
            const std::string t = Trim(s);

            if (t == "DashBuffDuration")         return (out = BuffKey::DashDuration), true;
            if (t == "DashBuffMoveSpeedBonus")   return (out = BuffKey::DashMoveSpeedBonus), true;

            if (t == "ExecutionBuffDuration")    return (out = BuffKey::ExecDuration), true;
            if (t == "ExecutionBuffAtkSpeedBonus") return (out = BuffKey::ExecAtkSpeedBonus), true;
            if (t == "ExecutionBuffMaxStacks")   return (out = BuffKey::ExecMaxStacks), true;

            return false;
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
                else
                {
                    (void)keyStr;
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
