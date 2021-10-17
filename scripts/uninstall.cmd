@echo off
pushd %~dp0
RunDLL32.Exe SETUPAPI.DLL,InstallHinfSection DefaultUninstall 132 %CD%\WdmDriver.inf
popd