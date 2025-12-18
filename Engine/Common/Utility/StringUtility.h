#pragma once

namespace engine
{
    // string/const char* -> wstring
    std::wstring ToWideChar(std::string_view multibyteStr);

    // wstring/const wchar_t* -> string
    std::string ToMultibyte(std::wstring_view wideCharStr);

    std::string FormatBytes(UINT64 bytes);
}
