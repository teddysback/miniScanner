@echo off
pushd %~dp0
fltmc unload WdmDriver
sc stop WdmDriver
sc query WdmDriver
popd