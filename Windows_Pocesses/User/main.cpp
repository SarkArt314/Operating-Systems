
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

bool ProcessExistsById(DWORD pid) {
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return false;
    DWORD exitCode = 0;
    BOOL ok = GetExitCodeProcess(h, &exitCode);
    CloseHandle(h);
    if (!ok) return false;
    return (exitCode == STILL_ACTIVE);
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

PROCESS_INFORMATION LaunchProcessSimple(const std::wstring& cmd) {
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    std::wstring cmdline = cmd;
    if (!CreateProcessW(NULL, &cmdline[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::wcerr << L"CreateProcess failed for \"" << cmd << L"\" (error " << GetLastError() << L")\n";
        PROCESS_INFORMATION empty = { 0 };
        return empty;
    }
    return pi;
}

std::wstring GetKillerPath() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring p = path;
    size_t pos = p.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        p = p.substr(0, pos + 1);
    }
    else {
        p = L".\\";
    }
    p += L"Killer.exe";
    return p;
}

void WaitAndCloseHandle(HANDLE h) {
    if (!h) return;
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
}

int wmain() {
    std::locale::global(std::locale(""));
    std::wcout << L"User demo started\n";

    const wchar_t* envName = L"PROCTOKILL";
    const std::wstring envValueFull = L"mspaint.exe, notepad.exe";

    if (!SetEnvironmentVariableW(envName, envValueFull.c_str())) {
        std::wcerr << L"SetEnvironmentVariableW failed (error " << GetLastError() << L")\n";
    }
    else {
        std::wcout << L"Set PROCTOKILL=\"" << envValueFull << L"\"\n";
    }

    std::wstring killerPath = GetKillerPath();
    std::wcout << L"Killer path: " << killerPath << L"\n";

    auto runKiller = [&](const std::wstring& args) {
        std::wstring cmd = L"\"" + killerPath + L"\" " + args;
        std::wcout << L"Running: " << cmd << L"\n";
        PROCESS_INFORMATION pi = LaunchProcessSimple(cmd);
        if (pi.hProcess) {
            WaitAndCloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else {
            std::wcerr << L"Failed to start Killer\n";
        }
        };

    // Demo A
    std::wcout << L"\n     Demo A: kill by id\n";
    PROCESS_INFORMATION piA = LaunchProcessSimple(L"mspaint.exe");
    if (!piA.hProcess) {
        std::wcerr << L"Не удалось запустить mspaint.exe для demo A\n";
    }
    else {
        DWORD pidA = piA.dwProcessId;
        Sleep(500);
        std::wcout << L"Started mspaint.exe with PID " << pidA << L"\n";
        bool existsBefore = ProcessExistsById(pidA);
        std::wcout << L"Exists before Killer? " << (existsBefore ? L"Yes" : L"No") << L"\n";

        std::wstring arg = L"--id " + std::to_wstring(pidA);
        runKiller(arg);

        bool existsAfter = ProcessExistsById(pidA);
        std::wcout << L"Exists after Killer? " << (existsAfter ? L"Yes" : L"No") << L"\n";

        CloseHandle(piA.hThread);
        CloseHandle(piA.hProcess);
    }

    // Demo B
    std::wcout << L"\n    Demo B: kill by name\n";
    PROCESS_INFORMATION piB1 = LaunchProcessSimple(L"mspaint.exe");
    PROCESS_INFORMATION piB2 = LaunchProcessSimple(L"mspaint.exe");
    Sleep(500);
    std::vector<DWORD> pidsBefore = FindPidsByName(L"mspaint.exe");
    std::wcout << L"mspaint.exe PIDs before Killer --name: ";
    for (DWORD p : pidsBefore) std::wcout << p << L" ";
    std::wcout << L"\n";

    runKiller(L"--name mspaint.exe");
    Sleep(500);
    std::vector<DWORD> pidsAfter = FindPidsByName(L"mspaint.exe");
    std::wcout << L"mspaint.exe PIDs after Killer --name: ";
    for (DWORD p : pidsAfter) std::wcout << p << L" ";
    std::wcout << L"\n";

    if (piB1.hProcess) { CloseHandle(piB1.hThread); CloseHandle(piB1.hProcess); }
    if (piB2.hProcess) { CloseHandle(piB2.hThread); CloseHandle(piB2.hProcess); }

    // Demo C
    std::wcout << L"\n    Demo C: kill by PROCTOKILL\n";

    PROCESS_INFORMATION piC1 = LaunchProcessSimple(L"mspaint.exe");
    PROCESS_INFORMATION piC2 = LaunchProcessSimple(L"notepad.exe");
    Sleep(500);
    std::vector<DWORD> beforeC_mspaint = FindPidsByName(L"mspaint.exe");
    std::vector<DWORD> beforeC_notepad = FindPidsByName(L"notepad.exe");
    std::wcout << L"mspaint.exe PIDs before: ";
    for (DWORD p : beforeC_mspaint) std::wcout << p << L" ";
    std::wcout << L"\nnotepad.exe PIDs before: ";
    for (DWORD p : beforeC_notepad) std::wcout << p << L" ";
    std::wcout << L"\n";

    runKiller(L""); 
    Sleep(500);
    std::vector<DWORD> afterC_mspaint = FindPidsByName(L"mspaint.exe");
    std::vector<DWORD> afterC_notepad = FindPidsByName(L"notepad.exe");
    std::wcout << L"mspaint.exe PIDs after: ";
    for (DWORD p : afterC_mspaint) std::wcout << p << L" ";
    std::wcout << L"\nnotepad.exe PIDs after: ";
    for (DWORD p : afterC_notepad) std::wcout << p << L" ";
    std::wcout << L"\n";

    if (piC1.hProcess) { CloseHandle(piC1.hThread); CloseHandle(piC1.hProcess); }
    if (piC2.hProcess) { CloseHandle(piC2.hThread); CloseHandle(piC2.hProcess); }

    if (SetEnvironmentVariableW(envName, NULL)) {
        std::wcout << L"Removed PROCTOKILL variable\n";
    }
    else {
        std::wcerr << L"Failed to remove PROCTOKILL (error " << GetLastError() << L")\n";
    }

    std::wcout << L"User demo finished\n";
    return 0;
}
