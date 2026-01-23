#pragma once
#include <windows.h>
#include <commdlg.h>
#include <filesystem>

namespace fs = std::filesystem;
// filter 예시: "Audio\0*.wav\0" 또는 "Image\0*.png;*.jpg\0"

namespace engine
{
    inline std::string OpenAudioFileDialog(const char* filter)
    {
        OPENFILENAMEA ofn;
        CHAR szFile[260] = { 0 };
        ZeroMemory(&ofn, sizeof(ofn));

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = nullptr;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = nullptr;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = nullptr;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

        if (GetOpenFileNameA(&ofn) == TRUE)
        {
            return std::string(szFile);
        }
        return std::string();
    }

    inline std::vector<std::string> OpenAudioFilesDialog(const char* filter)
    {
        std::vector<std::string> selectedFiles;

        const int BUFFER_SIZE = 65536; // 64KB
        char* buffer = new char[BUFFER_SIZE] { 0 };

        OPENFILENAMEA ofn = { 0 };
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = GetActiveWindow();
        ofn.lpstrFile = buffer;
        ofn.nMaxFile = BUFFER_SIZE;
        ofn.lpstrFilter = filter;
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = nullptr;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = nullptr;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

        if (GetOpenFileNameA(&ofn) == TRUE)
        {
            std::string directory = buffer;
            char* nextFile = buffer + directory.length() + 1;

            if (*nextFile == '\0')
            {
                selectedFiles.push_back(directory);
            }
            else
            {
                while (*nextFile != '\0')
                {
                    std::string filename = nextFile;
                    std::string fullPath = directory + "\\" + filename;
                    selectedFiles.push_back(fullPath);
                    nextFile += filename.length() + 1;
                }
            }
        }

        delete[] buffer;
        return selectedFiles;
    }
}