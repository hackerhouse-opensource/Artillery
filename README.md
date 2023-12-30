# Artillery

Artillery is a UAC bypass method described by the CIA's AED in the "Fine Dining" toolkit, 
which are a modular components used to create malware. The purpose is to bypass UAC and
obtain persistence on a host. Artillery utilizes elevated COM object to write to System32 
and an auto-elevated process to execute as administrator, it was then be combined with a
FAX DLL injection for explorer.exe for persistence on a host. This UAC bypass and persistence
has been tested on Windows 7 and Windows Vista.

