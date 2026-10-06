#include "stdafx.h"
#include "SpeedTestDisplay.h"
#include <string>

namespace
{
    struct SpeedTestData
    {
        CString result_text{ _T("--") };
        CString ping_text{ _T("--") };
        CString jitter_text{ _T("--") };
        CString last_checked_text{ _T("--") };
        bool valid{ false };
    };

    SpeedTestData g_data{};
    bool g_loaded{ false };
    FILETIME g_mtime{};
    bool g_has_mtime{ false };

    CString GetJsonFilePath()
    {
        //优先使用固定路径（speed-test 任务的写入位置）
        CString path{ _T("C:\Users\mikea\tools\speedtest-latest.json") };
        if (GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES)
            return path;
        //回退：exe 同目录下的 speedtest-latest.json
        wchar_t exe_path[MAX_PATH]{};
        GetModuleFileNameW(NULL, exe_path, MAX_PATH);
        CString dir{ exe_path };
        int pos = dir.ReverseFind(L'\');
        if (pos > 0)
            dir = dir.Left(pos);
        return dir + L"\speedtest-latest.json";
    }

    bool GetFileMtime(const CString& path, FILETIME& ft)
    {
        WIN32_FILE_ATTRIBUTE_DATA fad{};
        if (!GetFileAttributesEx(path, GetFileExInfoStandard, &fad))
            return false;
        ft = fad.ftLastWriteTime;
        return true;
    }

    //从 JSON 文本中提取 "key":<number>
    bool ExtractNumber(const std::wstring& json, const wchar_t* key, double& out)
    {
        const std::wstring pattern = std::wstring(L"\"") + key + L"\"";
        size_t pos = json.find(pattern);
        if (pos == std::wstring::npos)
            return false;
        pos = json.find(L':', pos + pattern.size());
        if (pos == std::wstring::npos)
            return false;
        size_t start = json.find_first_not_of(L" \t\r\n", pos + 1);
        if (start == std::wstring::npos)
            return false;
        wchar_t* end = nullptr;
        out = wcstod(json.c_str() + start, &end);
        return (end != nullptr && end != json.c_str() + start);
    }

    //从 JSON 文本中提取 "key":"value"
    bool ExtractString(const std::wstring& json, const wchar_t* key, CString& out)
    {
        const std::wstring pattern = std::wstring(L"\"") + key + L"\"";
        size_t pos = json.find(pattern);
        if (pos == std::wstring::npos)
            return false;
        pos = json.find(L':', pos + pattern.size());
        if (pos == std::wstring::npos)
            return false;
        pos = json.find(L'"', pos + 1);
        if (pos == std::wstring::npos)
            return false;
        size_t end = json.find(L'"', pos + 1);
        if (end == std::wstring::npos)
            return false;
        out = CString(json.c_str() + pos + 1, static_cast<int>(end - pos - 1));
        return true;
    }

    void LoadData()
    {
        SpeedTestData data{};

        CString path = GetJsonFilePath();
        FILETIME mtime{};
        bool has_file = GetFileMtime(path, mtime);
        if (has_file && g_loaded && g_has_mtime && CompareFileTime(&mtime, &g_mtime) == 0)
            return;         //文件未变化，使用缓存
        if (!has_file && g_loaded && !g_has_mtime)
            return;         //文件一直不存在，保持 "--"

        g_has_mtime = has_file;
        g_mtime = mtime;

        if (has_file)
        {
            HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL, OPEN_EXISTING, 0, NULL);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                DWORD file_size = GetFileSize(hFile, NULL);
                if (file_size > 0 && file_size < 1024 * 1024)
                {
                    std::string bytes(file_size, 0);
                    DWORD bytes_read = 0;
                    if (ReadFile(hFile, &bytes[0], file_size, &bytes_read, NULL) && bytes_read > 0)
                    {
                        int wide_len = MultiByteToWideChar(CP_UTF8, 0, bytes.c_str(), static_cast<int>(bytes_read), NULL, 0);
                        std::wstring json(wide_len, 0);
                        MultiByteToWideChar(CP_UTF8, 0, bytes.c_str(), static_cast<int>(bytes_read), &json[0], wide_len);

                        double down_mbps{}, up_mbps{}, ping_ms{}, jitter_ms{};
                        CString checked_at;
                        bool ok_down_up = ExtractNumber(json, L"download_mbps", down_mbps)
                            && ExtractNumber(json, L"upload_mbps", up_mbps);
                        ExtractNumber(json, L"ping_ms", ping_ms);
                        ExtractNumber(json, L"jitter_ms", jitter_ms);
                        ExtractString(json, L"checked_at", checked_at);

                        if (ok_down_up)
                        {
                            data.valid = true;
                            data.result_text.Format(_T("↓%.1f ↑%.1fMb"), down_mbps, up_mbps);
                            data.ping_text.Format(_T("%dms"), static_cast<int>(ping_ms + 0.5));
                            data.jitter_text.Format(_T("±%dms"), static_cast<int>(jitter_ms + 0.5));
                            //checked_at 形如 "2026-10-06 13:10"，取时间部分并加 "@"
                            int sp = checked_at.ReverseFind(L' ');
                            if (sp >= 0 && sp + 1 < checked_at.GetLength())
                                data.last_checked_text = CString(L"@") + checked_at.Mid(sp + 1);
                            else if (!checked_at.IsEmpty())
                                data.last_checked_text = CString(L"@") + checked_at;
                        }
                    }
                }
                CloseHandle(hFile);
            }
        }

        g_data = data;
        g_loaded = true;
    }
}

CString CSpeedTestDisplay::GetResultText()
{
    LoadData();
    return g_data.result_text;
}

CString CSpeedTestDisplay::GetPingText()
{
    LoadData();
    return g_data.ping_text;
}

CString CSpeedTestDisplay::GetJitterText()
{
    LoadData();
    return g_data.jitter_text;
}

CString CSpeedTestDisplay::GetLastCheckedText()
{
    LoadData();
    return g_data.last_checked_text;
}
