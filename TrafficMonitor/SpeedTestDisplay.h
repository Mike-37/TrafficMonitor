#pragma once

// 任务栏 speed-test 显示数据端口
// 读取 speedtest-latest.json（默认 C:\Users\mikea\tools\speedtest-latest.json，
// 若不存在则回退到 exe 同目录下的 speedtest-latest.json）。
// 文件内容示例：
// {"download_mbps":19.45,"upload_mbps":7.54,"ping_ms":60.1,"jitter_ms":2.3,"checked_at":"2026-10-06 13:10"}
// 文件缺失或解析失败时，所有字段显示 "--"。
class CSpeedTestDisplay
{
public:
    //任务栏 "结果" 字段，如 "↓19.4 ↑7.5Mb"
    static CString GetResultText();
    //任务栏 "ping" 字段，如 "60ms"
    static CString GetPingText();
    //任务栏 "jitter" 字段，如 "±2ms"
    static CString GetJitterText();
    //任务栏 "last-checked" 字段，如 "@13:10"
    static CString GetLastCheckedText();
};
