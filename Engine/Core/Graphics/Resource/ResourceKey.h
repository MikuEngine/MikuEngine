#pragma once

#include <string>

#include "Core/Graphics/Data/Vertex.h"

namespace engine
{
    struct VertexBufferKey
    {
        std::string filePath;
        VertexFormat format;

        auto operator<=>(const VertexBufferKey&) const = default;
    };
}

namespace std
{
    inline void HashCombine(size_t& seed, size_t hashValue)
    {
        seed ^= hashValue + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template <>
    struct hash<engine::VertexFormat>
    {
        size_t operator()(const engine::VertexFormat& format) const
        {
            return hash<size_t>()(static_cast<size_t>(format));
        }
    };

    template <>
    struct hash<engine::VertexBufferKey>
    {
        size_t operator()(const engine::VertexBufferKey& key) const
        {
            size_t seed = 0;

            HashCombine(seed, hash<std::string>()(key.filePath));
            HashCombine(seed, hash<engine::VertexFormat>()(key.format));

            return seed;
        }
    };
}