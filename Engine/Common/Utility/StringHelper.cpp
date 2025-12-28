#include "pch.h"
#include "StringHelper.h"

namespace engine
{
    std::wstring ToWideChar(std::string_view multibyteStr)
    {
        if (multibyteStr.empty())
        {
            return std::wstring();
        }

        int length = static_cast<int>(multibyteStr.length());

        int count = MultiByteToWideChar(CP_UTF8, 0, multibyteStr.data(), length, nullptr, 0);

        if (count == 0)
        {
            return std::wstring();
        }

        std::wstring str(count, L'\0');

        MultiByteToWideChar(CP_UTF8, 0, multibyteStr.data(), length, &str[0], count);

        return str;
    }

    std::string ToMultibyte(std::wstring_view wideCharStr)
    {
        if (wideCharStr.empty())
        {
            return std::string();
        }

        int length = static_cast<int>(wideCharStr.length());

        int count = WideCharToMultiByte(CP_UTF8, 0, wideCharStr.data(), length, nullptr, 0, nullptr, nullptr);

        if (count == 0)
        {
            return std::string();
        }

        std::string str(count, '\0');

        WideCharToMultiByte(CP_UTF8, 0, wideCharStr.data(), length, &str[0], count, nullptr, nullptr);

        return str;
    }

    std::string FormatBytes(UINT64 bytes)
    {
        constexpr double KB = 1024.0;
        constexpr double MB = KB * 1024.0;
        constexpr double GB = MB * 1024.0;

        if (bytes >= GB)
        {
            return std::format("{:.2f} GB", bytes / GB);
        }
        if (bytes >= MB)
        {
            return std::format("{:.2f} MB", bytes / MB);
        }
        if (bytes >= KB)
        {
            return std::format("{:.2f} KB", bytes / KB);
        }

        return std::format("{} B", bytes);
    }
}
