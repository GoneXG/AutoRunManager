@echo off
rem Build AutoRunManager (GUI, admin manifest) and selftest (console) with MSVC x64.
call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d "%~dp0src"

rc.exe /nologo resource.rc
if errorlevel 1 exit /b 1

cl.exe /nologo /W3 /EHsc /O2 /utf-8 /DUNICODE /D_UNICODE main.cpp autorun_core.cpp resource.res /Fe:"%~dp0build\AutoRunManager.exe" /link /SUBSYSTEM:WINDOWS /MANIFEST:NO user32.lib gdi32.lib comctl32.lib ole32.lib advapi32.lib shell32.lib shlwapi.lib uuid.lib
if errorlevel 1 exit /b 1

cl.exe /nologo /W3 /EHsc /O2 /utf-8 /DUNICODE /D_UNICODE selftest.cpp autorun_core.cpp /Fe:"%~dp0build\selftest.exe" /link /SUBSYSTEM:CONSOLE advapi32.lib shell32.lib shlwapi.lib uuid.lib
if errorlevel 1 exit /b 1


rc.exe /nologo /fo resource_debug.res resource_debug.rc
if errorlevel 1 exit /b 1

cl.exe /nologo /W3 /EHsc /O2 /utf-8 /DUNICODE /D_UNICODE main.cpp autorun_core.cpp resource_debug.res /Fe:"%~dp0build\AutoRunManager_debug.exe" /link /SUBSYSTEM:WINDOWS /MANIFEST:NO user32.lib gdi32.lib comctl32.lib ole32.lib advapi32.lib shell32.lib shlwapi.lib uuid.lib
if errorlevel 1 exit /b 1

echo DEBUG_BUILD_OK

