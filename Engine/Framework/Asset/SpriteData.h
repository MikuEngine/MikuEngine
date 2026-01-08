#pragma once

#include "Framework/Asset/AssetData.h"

namespace engine
{
	struct SpritePiece
	{
		std::string name;
		float x;
		float y;
		float width;
		float height;
		float pivotX;
		float pivotY;
	};

	class SpriteData :
		public AssetData
	{
	private:
		std::string m_name;
		std::vector<SpritePiece> m_pieces;
		std::unordered_map<std::string, size_t> m_pieceIndexMap;
		float m_width = 0.0f;
		float m_height = 0.0f;

	public:
		void Create(const std::string& filePath);

	public:
		float GetWidth() const;
		float GetHeight() const;
		const SpritePiece& GetSpritePiece(size_t index) const;
		const std::vector<SpritePiece>& GetSpritePieces() const;
		size_t GetSpriteIndex(const std::string& pieceName) const;
	};
}