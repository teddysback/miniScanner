@echo off
pushd %~dp0

:loop
sc stop WdmDriver

RunDLL32.Exe SETUPAPI.DLL,InstallHinfSection DefaultUninstall 132 %CD%\WdmDriver.inf

xcopy /y "\\Pandora\Virtual Machines\SHARED\WdmDriver*" "c:\Users\test\Desktop\miniFlt\"
xcopy /y "\\Pandora\Virtual Machines\SHARED\wdm_client*" "c:\Users\test\Desktop\miniFlt\"

RunDLL32.Exe syssetup,SetupInfObjectInstallAction DefaultInstall 128 %CD%\WdmDriver.inf

sc start WdmDriver
sc query WdmDriver

start cmd /k wdm_client.exe

pause
goto loop

popd