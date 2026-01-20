#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

namespace engine
{
	class Texture;

	struct FontGlyph
	{
		uint32_t codePoint = 0;

		float advance = 0.0f;
		float bearingX = 0.0f;
		float bearingY = 0.0f;

		float width = 0.0f;
		float height = 0.0f;

		int page = 0;
		int x = 0;
		int y = 0;
		int w = 0;
		int h = 0;

		bool IsEmptyBitmap() const { return w <= 0 || h <= 0; }
	};

	class AtlasPaker
	{
	public:
		void Reset(int atlasW, int atlasH, int padding);
		bool Allocate(int w, int h, int& outX, int& outY);

	private:
		int m_w = 0, m_h = 0;
		int m_padding = 1;
		int m_cursorX = 0;
		int m_cursorY = 0;
		int m_rowH = 0;
	};

	class FontData
	{
	public:
		struct Desc
		{
			std::string ttfPath;
			int pixelSize = 32;

			int atlasWidth = 1024;
			int atlasHeight = 1024;
			int padding = 1;

			DXGI_FORMAT atlasFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

			int maxPages = 4;
		};

	private:
		struct AtlasPage
		{
			Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
			Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
			AtlasPaker packer;

			std::vector<uint8_t> stagingRGBA;
		};

	public:
		FontData() = default;
		~FontData();

		FontData(const FontData&) = delete;
		FontData& operator=(const FontData&) = delete;

	public:
		bool Initialize(ID3D11Device* device, const Desc& desc);
		void Shutdown();

		const Desc& GetDesc() const { return m_desc; }

		float GetAscenderPx() const { return m_ascender; }
		float GetDescenderPx() const { return m_descender; }
		float GetLineHeightPx() const { return m_lineHeight; }

		// Atlas
		int GetPageCount() const { return static_cast<int>(m_pages.size()); }
		ID3D11ShaderResourceView* GetAtlasSRV(int page) const;

		// Glyph
		const FontGlyph* GetGlyph(uint32_t codepoint) const;
		const FontGlyph& EnsureGlyph(ID3D11DeviceContext* dc, uint32_t codepoint);

	private:
		bool LoadFace();
		void UpdateFontMetrics();

		bool CreateNewPage(ID3D11Device* device);

		FontGlyph BakeGlyph(uint32_t codePoint) const;

		bool UploadGlyphToAtlas(ID3D11DeviceContext* dc, FontGlyph& g, const FT_Bitmap& bm);

		void ConvertBitmapToRGBA(const FT_Bitmap& bm, std::vector<uint8_t>& outRGBA)const;

	private:
		Desc m_desc{};

		FT_Library m_ftLib = nullptr;
		FT_Face m_face = nullptr;

		float m_ascender = 0.0f;
		float m_descender = 0.0f;
		float m_lineHeight = 0.0f;

		std::unordered_map<uint32_t, FontGlyph> m_glyphs;
		std::vector<AtlasPage> m_pages;

		uint32_t m_fallbackCodepoint = static_cast<uint32_t>('?');

		//int m_atlasW = 0;
		//int m_atlasH = 0;

		//float m_emSize = 1.0f;
		//float m_pxRange = 4.0f;
	};
}