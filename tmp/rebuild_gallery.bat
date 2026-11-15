@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul
set PATH=C:\Qt\6.11.1\msvc2022_64\bin;%PATH%
cmake --build D:\Codez\WinUI3Style\build-pr62 --config Release --target winui3style_gallery 2>&1