#include "pch.h"
#include "Debug.h"

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#include <dxgidebug.h>
#include <dxgi1_3.h>

#pragma comment(lib, "dxguid.lib")
#endif // _DEBUG

#include <mutex>
#include <comdef.h>
#include <fstream>

namespace engine
{
    namespace
    {
        static std::mutex s_mutex;
        static const std::string s_logFilePath = "DebugLog.txt";
    }

    void Debug::__CheckResult(HRESULT hr, const char* file, int line)
    {
        if (FAILED(hr))
        {
            _com_error err(hr);

            std::string errorMsg = std::format(
                "Critical Error Detected!\n"
                "File: {}\n"
                "Line: {}\n"
                "HRESULT: {:#08x}\n"
                "Message: {}",
                file,
                line,
                static_cast<unsigned int>(hr),
                ToMultibyte(err.ErrorMessage())
            );

            std::wstring wideErrorMsg = ToWideChar(errorMsg);

            OutputDebugStringW(std::format(L"[FATAL] {}\n", wideErrorMsg).c_str());

            WriteToFile("[FATAL] ", errorMsg);
            MessageBoxW(nullptr, wideErrorMsg.c_str(), L"Error", MB_ICONERROR | MB_OK);

            if (IsDebuggerPresent())
            {
                __debugbreak();
            }
            else
            {
                std::exit(-1);
            }
        }
    }

    void Debug::__CheckFatal(bool condition, std::string_view msg, const char* file, int line)
    {
        if (!condition)
        {
            std::string errorMsg = std::format(
                "Fatal Check Failed!\n"
                "File: {}\n"
                "Line: {}\n"
                "Message: {}",
                file,
                line,
                msg
            );

            std::wstring wideErrorMsg = ToWideChar(errorMsg);

            OutputDebugStringW(std::format(L"[FATAL] {}\n", wideErrorMsg).c_str());

            WriteToFile("[FATAL] ", errorMsg);

            MessageBoxW(nullptr, wideErrorMsg.c_str(), L"Fatal Error", MB_ICONERROR | MB_OK);

            if (IsDebuggerPresent())
            {
                __debugbreak();
            }
            else
            {
                std::exit(-1);
            }
        }
    }

    void Debug::WriteToFile(std::string_view prefix, std::string_view msg)
    {
        //std::lock_guard<std::mutex> lock(s_mutex);

        std::ofstream file(s_logFilePath, std::ios::app);
        if (file.is_open())
        {
            auto now = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());

            file << std::format("[{:%Y-%m-%d %X}] {}{}\n", now, prefix, msg);
        }
    }

    LeakCheck::LeakCheck()
    {
#ifdef _DEBUG
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif // _DEBUG
    }

    LeakCheck::~LeakCheck()
    {
#ifdef _DEBUG
        // std::chrono::current_zone() 사용 후에
        // 메모리 누수 리포트 제거용
        // 실제 누수는 아닌데 그냥 보기 싫어서 해둠
        // 정상적인 방법 필요
        std::chrono::get_tzdb_list().~tzdb_list();
        IDXGIDebug1* pDebug = nullptr;

        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
        {
            pDebug->ReportLiveObjects(
                DXGI_DEBUG_ALL,
                DXGI_DEBUG_RLO_ALL
            );

            pDebug->Release();
        }
#endif // _DEBUG
    }
}
