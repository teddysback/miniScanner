@echo off
pushd %~dp0
net start WdmDriver
sc start WdmDriver
sc query WdmDriver
popd