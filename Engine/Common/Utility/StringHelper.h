#pragma once

#include <filesystem>

namespace engine
{
    // string/const char* -> wstring
    std::wstring ToWideChar(std::string_view multibyteStr);

    // wstring/const wchar_t* -> string
    std::string ToMultibyte(std::wstring_view wideCharStr);

    // std::filesystem::path -> UTF-8 string (for cross-platform file path handling)
    std::string PathToUTF8(const std::filesystem::path& path);

    std::string FormatBytes(UINT64 bytes);
}
