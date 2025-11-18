
#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>

static std::wstring ToLower(const std::wstring& s) {
    std::wstring r = s;
    std::transform(r.begin(), r.end(), r.begin(), ::towlower);
    return r;
}

bool KillProcessById(DWORD pid) {
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!h) {
        std::wcerr << L"OpenProcess failed for PID " << pid << L" (error " << GetLastError() << L")\n";
        return false;
    }
    BOOL ok = TerminateProcess(h, 1);
    CloseHandle(h);
    if (!ok) {
        std::wcerr << L"TerminateProcess failed for PID " << pid << L" (error " << GetLastError() << L")\n";
        return false;
    }
    std::wcout << L"Killed PID " << pid << L"\n";
    return true;
}

std::vector<DWORD> FindPidsByName(const std::wstring& name) {
    std::vector<DWORD> res;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return res;
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    std::wstring target = ToLower(name);
    if (Process32FirstW(snap, &pe)) {
        do {
            std::wstring exe = ToLower(pe.szExeFile);
            if (exe == target) {
                res.push_back(pe.th32ProcessID);
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return res;
}

bool KillProcessesByName(const std::wstring& name) {
    auto pids = FindPidsByName(name);
    if (pids.empty()) {
        std::wcout << L"No processes found with name " << name << L"\n";
        return false;
    }
    bool any = false;
    for (DWORD pid : pids) {
        if (KillProcessById(pid)) any = true;
    }
    return any;
}

std::vector<std::wstring> SplitNames(const std::wstring& s) {
    std::vector<std::wstring> out;
    std::wstringstream ss(s);
    std::wstring item;
    while (std::getline(ss, item, L',')) {
        size_t start = 0;
        while (start < item.size() && iswspace(item[start])) ++start;
        size_t end = item.size();
        while (end > start && iswspace(item[end - 1])) --end;
        std::wstring sub = item.substr(start, end - start);
        if (sub.size() >= 2 && ((sub.front() == L'"' && sub.back() == L'"') || (sub.front() == L'\'' && sub.back() == L'\''))) {
            sub = sub.substr(1, sub.size() - 2);
        }
        if (!sub.empty()) out.push_back(sub);
    }
    return out;
}

int wmain(int argc, wchar_t* argv[]) {
    std::locale::global(std::locale(""));
    std::wcout << L"Killer started\n";

    DWORD idToKill = 0;
    std::wstring nameToKill;
    for (int i = 1; i < argc; ++i) {
        std::wstring arg = argv[i];
        if (arg == L"--id" && i + 1 < argc) {
            idToKill = (DWORD)_wtoi(argv[++i]);
        }
        else if (arg == L"--name" && i + 1 < argc) {
            nameToKill = argv[++i];
        }
        else {
            std::wcout << L"Ignoring unknown arg: " << arg << L"\n";
        }
    }

    bool argsProvided = (idToKill != 0) || (!nameToKill.empty());

    if (idToKill != 0) {
        std::wcout << L"Attempting to kill by id: " << idToKill << L"\n";
        KillProcessById(idToKill);
    }

    if (!nameToKill.empty()) {
        std::wcout << L"Attempting to kill by name: " << nameToKill << L"\n";
        KillProcessesByName(nameToKill);
    }

    if (!argsProvided) {
        DWORD needed = GetEnvironmentVariableW(L"PROCTOKILL", NULL, 0);
        if (needed > 0) {
            std::wstring buf;
            buf.resize(needed);
            GetEnvironmentVariableW(L"PROCTOKILL", &buf[0], needed);
            if (!buf.empty() && buf.back() == L'\0') buf.pop_back();
            if (!buf.empty()) {
                std::wcout << L"PROCTOKILL=\"" << buf << L"\"\n";
                auto names = SplitNames(buf);
                for (auto& nm : names) {
                    std::wcout << L"Attempting to kill processes from PROCTOKILL: " << nm << L"\n";
                    KillProcessesByName(nm);
                }
            }
            else {
                std::wcout << L"PROCTOKILL is empty\n";
            }
        }
        else {
            std::wcout << L"PROCTOKILL not set\n";
        }
    }
    else {
        std::wcout << L"Skipping PROCTOKILL because explicit args were provided\n";
    }

    std::wcout << L"Killer finished\n";
    return 0;
}
