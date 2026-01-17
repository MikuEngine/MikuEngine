#include "EnginePCH.h"
#include "SpriteData.h"

#include <fstream>

namespace engine
{
    void SpriteData::Create(const std::string& filePath)
    {
        std::ifstream f{ filePath };
        if (!f.is_open())
        {
            LOG_INFO("{} 파일 열기 실패", filePath);
            return;
        }

        json j;
        f >> j;
        f.close();

        m_name = j["name"];
        m_width = j["width"];
        m_height = j["height"];

        auto pieces = j["sprites"];
        m_pieces.reserve(pieces.size());

        for (const auto& jj : pieces)
        {
            m_pieces.emplace_back(
                jj["name"],
                jj["x"],
                jj["y"],
                jj["width"],
                jj["height"],
                jj["pivotX"],
                jj["pivotY"]);
        }

        for (size_t i = 0; i < m_pieces.size(); ++i)
        {
            m_pieceIndexMap.emplace(m_pieces[i].name, i);
        }
    }

    float SpriteData::GetWidth() const
    {
        return m_width;
    }

    float SpriteData::GetHeight() const
    {
        return m_height;
    }

    const SpritePiece& engine::SpriteData::GetSpritePiece(size_t index) const
    {
        return m_pieces[index];
    }

    size_t SpriteData::GetSpriteIndex(const std::string& pieceName) const
    {
        return m_pieceIndexMap.at(pieceName);
    }
}