#include "EnginePCH.h"
#include "FontData.h"

#include <fstream>

#include "Core/System/VirtualFileSystem.h"
#include "Core/Graphics/Resource/ResourceManager.h"
#include "Core/Graphics/Resource/Texture.h"

namespace engine
{
	void AtlasPaker::Reset(int atlasW, int atlasH, int padding)
	{
		m_w = atlasW;
		m_h = atlasH;
		m_padding = std::max(0, padding);

		m_cursorX = 0;
		m_cursorY = 0;
		m_rowH = 0;
	}

	bool AtlasPaker::Allocate(int w, int h, int& outX, int& outY)
	{
		const int W = w + m_padding * 2;
		const int H = h + m_padding * 2;

		if (W > m_w || H > m_h)
			return false;

		if (m_cursorX + W > m_w)
		{
			m_cursorX = 0;
			m_cursorY += m_rowH;
			m_rowH = 0;
		}

		if (m_cursorY + H > m_h)
			return false;

		outX = m_cursorX + m_padding;
		outY = m_cursorY + m_padding;

		m_cursorX += W;
		m_rowH = std::max(m_rowH, H);
		return true;
	}

	// FontData
	FontData::~FontData()
	{
		Shutdown();
	}

	bool FontData::Initialize(ID3D11Device* device, const Desc& desc)
	{
		if (!device)
			return false;

		m_desc = desc;

		if (FT_Init_FreeType(&m_ftLib) != 0)
			return false;

		if (!LoadFace())
			return false;

		if (FT_Set_Pixel_Sizes(m_face, 0, static_cast<FT_UInt>(m_desc.pixelSize)) != 0)
			return false;

		UpdateFontMetrics();

		m_pages.clear();
		m_pages.reserve(std::max(1, m_desc.maxPages));

		if (!CreateNewPage(device))
			return false;

		return true;
	}

	void FontData::Shutdown()
	{
		m_glyphs.clear();
		m_pages.clear();
		m_ttfBuffer.clear();

		if (m_face)
		{
			FT_Done_Face(m_face);
			m_face = nullptr;
		}

		if (m_ftLib)
		{
			FT_Done_FreeType(m_ftLib);
			m_ftLib = nullptr;
		}

		m_ascender = m_descender = m_lineHeight = 0.0f;
	}

	ID3D11ShaderResourceView* FontData::GetAtlasSRV(int page) const
	{
		if (page < 0 || page >= static_cast<int>(m_pages.size()))
			return nullptr;

		return m_pages[page].srv.Get();
	}

	const FontGlyph* FontData::GetGlyph(uint32_t codepoint) const
	{
		auto it = m_glyphs.find(codepoint);
		if (it == m_glyphs.end())
			return nullptr;
		return &it->second;
	}

	const FontGlyph& FontData::EnsureGlyph(ID3D11DeviceContext* dc, uint32_t codepoint)
	{
		if (auto it = m_glyphs.find(codepoint); it != m_glyphs.end())
			return it->second;

		// 글리프 생성
		FontGlyph g = BakeGlyph(codepoint);

		// 비트맵이 비어도(스페이스 등) advance는 유효합니다.
		// 빈 비트맵이면 업로드는 생략하고 캐시에 넣습니다.
		if (!g.IsEmptyBitmap())
		{
			// 현재 face->glyph->bitmap은 BakeGlyph에서 만든 codepoint에 해당
			// BakeGlyph는 metrics만 만들었기 때문에,
			// 여기서 다시 로드해서 bitmap을 얻습니다(안전/명확).
			if (FT_Load_Char(m_face, codepoint, FT_LOAD_RENDER) != 0)
			{
				// fallback
				if (codepoint != m_fallbackCodepoint &&
					FT_Load_Char(m_face, m_fallbackCodepoint, FT_LOAD_RENDER) == 0)
				{
					g = BakeGlyph(m_fallbackCodepoint);
				}
			}

			const FT_Bitmap& bm = m_face->glyph->bitmap;

			// 업로드 실패 시: 새 페이지 만들기 / 또는 fallback
			if (!UploadGlyphToAtlas(dc, g, bm))
			{
				// fallback 재시도
				if (codepoint != m_fallbackCodepoint)
				{
					const FontGlyph& fb = EnsureGlyph(dc, m_fallbackCodepoint);
					return fb;
				}
			}
		}

		auto [it, inserted] = m_glyphs.emplace(codepoint, g);
		return it->second;
	}

	bool FontData::LoadFace()
	{
		if (!m_ftLib)
			return false;

		if (m_face)
		{
			FT_Done_Face(m_face);
			m_face = nullptr;
		}

		if (m_desc.ttfPath.empty())
			return false;

		// VFS로 파일을 메모리로 로드
		m_ttfBuffer.clear();

		const std::string path = m_desc.ttfPath; // 가상 경로 그대로 사용
		if (!VirtualFileSystem::Get().LoadFile(path, m_ttfBuffer) || m_ttfBuffer.empty())
			return false;

		// 메모리에서 Face 생성 (버퍼는 m_ttfBuffer가 들고 있으므로 수명 OK)
		if (FT_New_Memory_Face(
			m_ftLib,
			reinterpret_cast<const FT_Byte*>(m_ttfBuffer.data()),
			static_cast<FT_Long>(m_ttfBuffer.size()),
			0,
			&m_face) != 0)
		{
			m_ttfBuffer.clear();
			return false;
		}

		return true;
	}

	void FontData::UpdateFontMetrics()
	{
		const auto& m = m_face->size->metrics;

		m_ascender = static_cast<float>(m.ascender) / 64.0f;
		m_descender = static_cast<float>(m.descender) / 64.0f;
		m_lineHeight = static_cast<float>(m.height) / 64.0f;

		// height가 0으로 오는 폰트도 드물게 있어 안전장치
		if (m_lineHeight <= 0.0f)
			m_lineHeight = static_cast<float>(m_desc.pixelSize);
	}

	bool FontData::CreateNewPage(ID3D11Device* device)
	{
		if (!device)
			return false;

		if (static_cast<int>(m_pages.size()) >= std::max(1, m_desc.maxPages))
			return false;

		AtlasPage page{};
		page.packer.Reset(m_desc.atlasWidth, m_desc.atlasHeight, m_desc.padding);

		D3D11_TEXTURE2D_DESC td{};
		td.Width = static_cast<UINT>(m_desc.atlasWidth);
		td.Height = static_cast<UINT>(m_desc.atlasHeight);
		td.MipLevels = 1;
		td.ArraySize = 1;
		td.Format = m_desc.atlasFormat;
		td.SampleDesc.Count = 1;
		td.Usage = D3D11_USAGE_DEFAULT;
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		// 초기 클리어(검정, alpha 0)
		std::vector<uint8_t> zero(td.Width * td.Height * 4, 0);
		D3D11_SUBRESOURCE_DATA init{};
		init.pSysMem = zero.data();
		init.SysMemPitch = td.Width * 4;

		if (FAILED(device->CreateTexture2D(&td, &init, page.tex.GetAddressOf())))
			return false;

		D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
		sd.Format = td.Format;
		sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		sd.Texture2D.MipLevels = 1;

		if (FAILED(device->CreateShaderResourceView(page.tex.Get(), &sd, page.srv.GetAddressOf())))
			return false;

		m_pages.push_back(std::move(page));
		return true;
	}

	FontGlyph FontData::BakeGlyph(uint32_t codePoint) const
	{
		FontGlyph g{};
		g.codePoint = codePoint;

		// FT_LOAD_RENDER로 bitmap 생성
		if (FT_Load_Char(m_face, codePoint, FT_LOAD_RENDER) != 0)
		{
			// 실패하면 fallback 시도
			if (codePoint != m_fallbackCodepoint &&
				FT_Load_Char(m_face, m_fallbackCodepoint, FT_LOAD_RENDER) == 0)
			{
				codePoint = m_fallbackCodepoint;
				g.codePoint = codePoint;
			}
		}

		const FT_GlyphSlot slot = m_face->glyph;

		// advance는 26.6
		g.advance = static_cast<float>(slot->advance.x) / 64.0f;

		// bitmap_left는 penX 기준 좌측 오프셋
		g.bearingX = static_cast<float>(slot->bitmap_left);

		// bitmap_top은 baseline 기준 위쪽 오프셋(윗변 위치)
		g.bearingY = static_cast<float>(slot->bitmap_top);

		g.width = static_cast<float>(slot->bitmap.width);
		g.height = static_cast<float>(slot->bitmap.rows);

		g.w = static_cast<int>(slot->bitmap.width);
		g.h = static_cast<int>(slot->bitmap.rows);

		return g;
	}

	bool FontData::UploadGlyphToAtlas(ID3D11DeviceContext* dc, FontGlyph& g, const FT_Bitmap& bm)
	{
		if (!dc)
			return false;

		if (g.IsEmptyBitmap())
			return true;

		// 배치할 페이지 찾기
		for (int attempt = 0; attempt < 2; ++attempt)
		{
			// 현재 페이지들에서 자리 찾기
			for (int pageIdx = 0; pageIdx < static_cast<int>(m_pages.size()); ++pageIdx)
			{
				auto& page = m_pages[pageIdx];

				int x = 0, y = 0;
				if (!page.packer.Allocate(g.w, g.h, x, y))
					continue;

				// 배치 성공
				g.page = pageIdx;
				g.x = x; g.y = y;

				// RGBA 변환
				ConvertBitmapToRGBA(bm, page.stagingRGBA);

				// UpdateSubresource로 부분 업로드
				D3D11_BOX box{};
				box.left = static_cast<UINT>(g.x);
				box.top = static_cast<UINT>(g.y);
				box.front = 0;
				box.right = static_cast<UINT>(g.x + g.w);
				box.bottom = static_cast<UINT>(g.y + g.h);
				box.back = 1;

				const UINT rowPitch = static_cast<UINT>(g.w * 4);
				dc->UpdateSubresource(page.tex.Get(), 0, &box, page.stagingRGBA.data(), rowPitch, 0);

				return true;
			}

			// 자리가 없으면 새 페이지 생성 시도
			if (attempt == 0)
			{
				auto device = GraphicsDevice::Get().GetDevice();
				if (!CreateNewPage(device.Get()))
					return false;
			}
		}

		return false;
	}

	void FontData::ConvertBitmapToRGBA(const FT_Bitmap& bm, std::vector<uint8_t>& outRGBA) const
	{
		const int w = static_cast<int>(bm.width);
		const int h = static_cast<int>(bm.rows);

		outRGBA.resize(static_cast<size_t>(w) * static_cast<size_t>(h) * 4);

		const uint8_t* src = bm.buffer;
		const int pitch = bm.pitch;

		for (int y = 0; y < h; ++y)
		{
			const uint8_t* row = src + y * pitch;
			for (int x = 0; x < w; ++x)
			{
				const uint8_t a = row[x]; // 0..255
				const size_t idx = (static_cast<size_t>(y) * w + x) * 4;

				// 흰색 + alpha (UIQuad_PS에서 color 곱하면 됨)
				outRGBA[idx + 0] = 255;
				outRGBA[idx + 1] = 255;
				outRGBA[idx + 2] = 255;
				outRGBA[idx + 3] = a;
			}
		}
	}
}