#pragma once

#include <json.hpp>
#include <directxtk/SimpleMath.h>

namespace DirectX::SimpleMath
{
    inline void to_json(nlohmann::ordered_json& j, const Vector3& v)
    {
        j = nlohmann::ordered_json{
            { "x", v.x },
            { "y", v.y },
            { "z", v.z }
        };
    }

    inline void from_json(const nlohmann::ordered_json& j, Vector3& v)
    {
        v.x = j.at("x");
        v.y = j.at("y");
        v.z = j.at("z");
    }

    inline void to_json(nlohmann::ordered_json& j, const Quaternion& q)
    {
        j = nlohmann::ordered_json{
            { "x", q.x },
            { "y", q.y },
            { "z", q.z },
            { "w", q.w }
        };
    }

    inline void from_json(const nlohmann::ordered_json& j, Quaternion& q)
    {
        q.x = j.at("x");
        q.y = j.at("y");
        q.z = j.at("z");
        q.w = j.at("w");
    }
}

namespace engine
{
    template <typename T>
    inline void JsonGet(const nlohmann::ordered_json& j, const std::string& key, T& outValue)
    {
        if (auto iter = j.find(key); iter != j.end())
        {
            outValue = iter.value();
        }
    }

    template <typename Func>
    inline void JsonArrayForEach(const nlohmann::ordered_json& j, const std::string& key, Func func)
    {
        if (auto iter = j.find(key); iter != j.end() && iter->is_array())
        {
            for (const auto& item : iter.value())
            {
                func(item);
            }
        }
    }
}