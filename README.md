# Artillery

Artillery is a UAC bypass method described by the Central Intelligence Agency (CIA) Applied 
Engineering Department (AED) in the "Fine Dining" toolkit, which are modular components used 
to create malware. This module is part of the Privilege Escalation collection and is combined 
with other modules and payloads. The purpose is to bypass UAC and a second attack is used
to obtain persistence on a host. Artillery utilizes elevated COM object to write to System32 
and an auto-elevated process to execute as administrator, it can then be combined with a
FAX DLL injection for explorer.exe for persistence on a host. This UAC bypass and persistence
has been tested on Windows 7 and Windows Vista.

##  License

These files are available under a Attribution-NonCommercial-NoDerivatives 4.0 International license.
