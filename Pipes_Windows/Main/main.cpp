#define UNICODE
#define _UNICODE
#include <windows.h>
#include <string>
#include <iostream>

bool CreateChildProcess(const std::wstring& cmdline, HANDLE hStdIn, HANDLE hStdOut, PROCESS_INFORMATION& pi)
{
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hStdIn;
    si.hStdOutput = hStdOut;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    std::wstring cmd = cmdline; 
    BOOL ok = CreateProcessW(
        nullptr,
        &cmd[0],
        nullptr,
        nullptr,
        TRUE,  
        0,
        nullptr,
        nullptr,
        &si,
        &pi
    );
    return ok == TRUE;
}

int wmain()
{
    std::string inputLine = "1 2 3 4 5\n";

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = TRUE; 

    // 1 main -> M
    HANDLE hMainToM_Read = nullptr;   
    HANDLE hMainToM_Write = nullptr;  
    if (!CreatePipe(&hMainToM_Read, &hMainToM_Write, &sa, 0)) { std::cerr << "CreatePipe failed\n"; return 1; }
    SetHandleInformation(hMainToM_Write, HANDLE_FLAG_INHERIT, 0);

    // 2 M -> A
    HANDLE hMOut_Read = nullptr;  
    HANDLE hMOut_Write = nullptr;
    if (!CreatePipe(&hMOut_Read, &hMOut_Write, &sa, 0)) { std::cerr << "CreatePipe failed\n"; return 1; }


    // 3 A -> P
    HANDLE hAOut_Read = nullptr;  
    HANDLE hAOut_Write = nullptr; 
    if (!CreatePipe(&hAOut_Read, &hAOut_Write, &sa, 0)) { std::cerr << "CreatePipe failed\n"; return 1; }

    // 4 P -> S
    HANDLE hPOut_Read = nullptr;  
    HANDLE hPOut_Write = nullptr; 
    if (!CreatePipe(&hPOut_Read, &hPOut_Write, &sa, 0)) { std::cerr << "CreatePipe failed\n"; return 1; }

    // 5 S -> Main 
    HANDLE hSOut_Read = nullptr;  
    HANDLE hSOut_Write = nullptr; 
    if (!CreatePipe(&hSOut_Read, &hSOut_Write, &sa, 0)) { std::cerr << "CreatePipe failed\n"; return 1; }
    SetHandleInformation(hSOut_Read, HANDLE_FLAG_INHERIT, 0);

    PROCESS_INFORMATION piM, piA, piP, piS;
    ZeroMemory(&piM, sizeof(piM));
    ZeroMemory(&piA, sizeof(piA));
    ZeroMemory(&piP, sizeof(piP));
    ZeroMemory(&piS, sizeof(piS));

    if (!CreateChildProcess(L"M.exe", hMainToM_Read, hMOut_Write, piM)) { std::wcerr << L"Create M failed, err=" << GetLastError() << L"\n"; return 1; }

    if (!CreateChildProcess(L"A.exe", hMOut_Read, hAOut_Write, piA)) { std::wcerr << L"Create A failed, err=" << GetLastError() << L"\n"; return 1; }

    if (!CreateChildProcess(L"P.exe", hAOut_Read, hPOut_Write, piP)) { std::wcerr << L"Create P failed, err=" << GetLastError() << L"\n"; return 1; }

    if (!CreateChildProcess(L"S.exe", hPOut_Read, hSOut_Write, piS)) { std::wcerr << L"Create S failed, err=" << GetLastError() << L"\n"; return 1; }

    CloseHandle(hMainToM_Read);   
    CloseHandle(hMOut_Write);    
    CloseHandle(hMOut_Read);      
    CloseHandle(hAOut_Write);    
    CloseHandle(hAOut_Read);      
    CloseHandle(hPOut_Write);    
    CloseHandle(hPOut_Read);     
    CloseHandle(hSOut_Write);     

    DWORD written = 0;
    BOOL ok = WriteFile(hMainToM_Write, inputLine.c_str(), (DWORD)inputLine.size(), &written, nullptr);
    if (!ok) { std::cerr << "WriteFile failed, err=" << GetLastError() << "\n"; }
    CloseHandle(hMainToM_Write);

    const DWORD bufSize = 4096;
    char buffer[bufSize];
    std::string result;
    DWORD read = 0;
    while (ReadFile(hSOut_Read, buffer, bufSize, &read, nullptr) && read > 0) {
        result.append(buffer, buffer + read);
    }
    CloseHandle(hSOut_Read);

    WaitForSingleObject(piM.hProcess, INFINITE);
    WaitForSingleObject(piA.hProcess, INFINITE);
    WaitForSingleObject(piP.hProcess, INFINITE);
    WaitForSingleObject(piS.hProcess, INFINITE);

    CloseHandle(piM.hProcess); CloseHandle(piM.hThread);
    CloseHandle(piA.hProcess); CloseHandle(piA.hThread);
    CloseHandle(piP.hProcess); CloseHandle(piP.hThread);
    CloseHandle(piS.hProcess); CloseHandle(piS.hThread);

    std::cout << "Final output from chain (S): " << result;

        FILE* f = nullptr;
        freopen_s(&f, "CONIN$", "r", stdin);
        if (f) {
            std::cout << "Press Enter to exit...";
            std::string dummy;
            std::getline(std::cin, dummy);
        }
        else {
            HANDLE hCon = CreateFileW(L"CONIN$", GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
            if (hCon != INVALID_HANDLE_VALUE) {
                std::cout << "Press Enter to exit...";
                char ch;
                DWORD rd;
                ReadFile(hCon, &ch, 1, &rd, nullptr);
                CloseHandle(hCon);
            }
        }

    return 0;
}
