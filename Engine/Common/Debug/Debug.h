#pragma once

#define HR_CHECK(x) engine::Debug::__CheckResult(x, __FILE__, __LINE__)
#define FATAL_CHECK(cond, msg) \
    engine::Debug::__CheckFatal(cond, msg, __FILE__, __LINE__);\
    _Analysis_assume_(cond)
#define LOG_ERROR(...) engine::Debug::__LogError(__VA_ARGS__)

#ifdef _DEBUG

#define LOG_INFO(...) engine::Debug::__Log(__VA_ARGS__)
#define LOG_PRINT(...) engine::Debug::__Print(__VA_ARGS__)

#else

#define LOG_INFO(...) (void)0
#define LOG_PRINT(...) (void)0

#endif

namespace engine
{
    class Debug
    {
    public:
        template<typename... Args>
        static void __Log(std::format_string<Args...> fmt, Args&&... args)
        {
            try
            {
                std::string msg = std::format(fmt, std::forward<Args>(args)...);
                std::string output = std::format("[INFO] {}\n", msg);
                OutputDebugStringA(output.c_str());

                WriteToFile("[INFO] ", msg);
            }
            catch (...)
            {
                OutputDebugStringA("[LOG ERROR] Formatting failed.\n");
            }
        }

        template<typename... Args>
        static void __Print(std::format_string<Args...> fmt, Args&&... args)
        {
            try
            {
                std::string msg = std::format(fmt, std::forward<Args>(args)...);
                std::string output = std::format("[PRINT] {}\n", msg);
                OutputDebugStringA(output.c_str());
            }
            catch (...)
            {
                OutputDebugStringA("[LOG ERROR] Formatting failed.\n");
            }
        }

        template<typename... Args>
        static void __LogError(std::format_string<Args...> fmt, Args&&... args)
        {
            try
            {
                std::string msg = std::format(fmt, std::forward<Args>(args)...);
                std::string output = std::format("[ERROR] {}\n", msg);
                OutputDebugStringA(output.c_str());

                WriteToFile("[ERROR] ", msg);

                MessageBoxA(nullptr, msg.c_str(), "Engine Error", MB_ICONERROR | MB_OK);
            }
            catch (...)
            {
                OutputDebugStringA("[LOG ERROR] Formatting failed.\n");
            }
        }

        static void __CheckResult(HRESULT hr, const char* file, int line);
        static void __CheckFatal(bool condition, std::string_view msg, const char* file, int line);

    private:
        static void WriteToFile(const std::string& prefix, const std::string& msg);
    };

    class LeakCheck
    {
    public:
        LeakCheck();
        ~LeakCheck();
    };
}