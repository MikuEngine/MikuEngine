#pragma once

#include <cstdint>
#include <functional>

namespace engine
{
    struct Handle
    {
        std::uint32_t index = 0;
        std::uint32_t generation = 0;

        bool operator==(const Handle& other) const
        {
            return (index == other.index) && (generation == other.generation);
        }

        bool operator!=(const Handle& other) const
        {
            return !(*this == other);
        }

        bool IsValid() const
        {
            return (index != 0) || (generation != 0);
        }
    };
}

// std::hash 특수화 (unordered_set/unordered_map에서 Handle 사용 가능하게)
template<>
struct std::hash<engine::Handle>
{
    size_t operator()(const engine::Handle& h) const noexcept
    {
        // index와 generation을 조합하여 해시 생성
        return std::hash<uint64_t>()(
            (static_cast<uint64_t>(h.index) << 32) | h.generation
        );
    }
};