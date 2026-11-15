@echo off
set PATH=C:\Qt\6.11.1\msvc2022_64\bin;D:\Codez\WinUI3Style\build-pr62\src\lib\Release;%PATH%
set QT_QPA_PLATFORM=windows
set QT_QPA_FONTDIR=C:\Windows\Fonts
cd /d D:\Codez\WinUI3Style\build-pr62\src\gallery\Release
start "" winui3style_gallery.exe