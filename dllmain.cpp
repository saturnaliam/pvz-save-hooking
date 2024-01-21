#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <conio.h>
#include <cstdio>
#include <iostream>
#include <MinHook.h>
#include <fstream>
#pragma comment(lib, "libMinHook.x86.lib")

int numFiles = 0;
typedef BOOL(WINAPI* writeFile)(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
writeFile pWriteFile = nullptr;
writeFile pWriteFileTarget;

BOOL WINAPI detourWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    if (((const char*)(lpBuffer))[0] != '[') {
        FILE_NAME_INFO fileInfo;
        GetFileInformationByHandleEx(hFile, FileNameInfo, &fileInfo, sizeof(fileInfo));

        std::string buffer;
        if (fileInfo.FileNameLength != 0) {
            for (int i = 0; i < nNumberOfBytesToWrite; i++) {
                buffer += ((const char*)(lpBuffer))[i];
                std::cout << ((const char*)(lpBuffer))[i];
            }

            std::cout << "\nwriting to save" << ++numFiles << ".dat\n" << buffer << "\n-------\n";

            std::string filename = "E:\\dev\\pvz-save-hooking\\saves\\save" + std::to_string(numFiles) + ".dat";
            std::ofstream x(filename);
            x << buffer;
            x.close();
        }
    }

    return pWriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

void WINAPI panic(std::string message, HMODULE hModule) {
    MessageBoxA(0, "panicked!", message.c_str(), MB_OK);
    FreeLibraryAndExitThread(hModule, 0);
}

BOOL WINAPI dllInjected(HMODULE hModule) {
    AllocConsole();
    FILE* fpw;
    freopen_s(&fpw, "CONOUT$", "w", stdout);

    std::cout << "[V] Disable Hook // [E] Enable Hook // [D] Exit\n";

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        std::string sStatus = MH_StatusToString(status);
        panic(sStatus, hModule);
        return FALSE;
    }

    if (MH_CreateHookApi(L"Kernel32", "WriteFile", &detourWriteFile, reinterpret_cast<void**>(&pWriteFile)) != MH_OK) {
        panic("hook failed!", hModule);
        return FALSE;
    }

    bool hookEnabled = false;
    while (true) {
        if (GetAsyncKeyState('D')) break;

        if (GetAsyncKeyState('E') && !hookEnabled) {
            hookEnabled = true;
            std::cout << "hook enabled!\n";
            if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
                panic("enable hook failed!", hModule);
                return FALSE;
            }
        } else if (GetAsyncKeyState('V') && hookEnabled) {
            hookEnabled = false;
            std::cout << "hook disabled!\n";
            if (MH_DisableHook(MH_ALL_HOOKS) != MH_OK) {
                panic("disable hook failed!", hModule);
                return FALSE;
            }
        }
    }

    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    fclose(fpw);
    FreeConsole();
    FreeLibraryAndExitThread(hModule, 0);
    return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    if (dwReason == DLL_PROCESS_ATTACH) {
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)dllInjected, hModule, 0, NULL);
    }

    return TRUE;
}

