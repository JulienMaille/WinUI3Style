@echo off
cd /d "%~dp0"
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul
set QT=C:\Qt\6.12.0\msvc2022_64
cl /nologo /std:c++17 /Zc:__cplusplus /permissive- /EHsc /MD /utf-8 /O2 probe.cpp ^
  /I"%QT%\include" /I"%QT%\include\QtWidgets" /I"%QT%\include\QtGui" /I"%QT%\include\QtCore" ^
  /link /LIBPATH:"%QT%\lib" Qt6Widgets.lib Qt6Gui.lib Qt6Core.lib ^
  /OUT:probe.exe
if errorlevel 1 exit /b 1
set QT_PLUGIN_PATH=%CD%\..\..\build\plugins
set QT_QPA_PLATFORM=offscreen
probe.exe
