@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set QT=C:\Qt\6.11.1\msvc2022_64
set PATH=%QT%\bin;D:\Codez\WinUI3Style\build-pr62\src\lib\Release;%PATH%
cd /d D:\Codez\WinUI3Style
cl /nologo /EHsc /std:c++17 /Zc:__cplusplus /permissive- /O2 /MD /I "%QT%\include" /I "%QT%\include\QtCore" /I "%QT%\include\QtGui" /I "%QT%\include\QtWidgets" /I src\include /I src\lib tmp\issue55_probe.cpp /Fe:tmp\issue55_probe.exe /link /LIBPATH:"%QT%\lib" /LIBPATH:build-pr62\src\lib\Release winui3style.lib Qt6Core.lib Qt6Gui.lib Qt6Widgets.lib
if errorlevel 1 exit /b 1
set QT_QPA_PLATFORM=offscreen
cd /d D:\Codez\WinUI3Style\tmp
issue55_probe.exe