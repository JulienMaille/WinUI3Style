@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul
set PATH=C:\Qt\6.11.1\msvc2022_64\bin;%PATH%
cd /d D:\Codez\WinUI3Style\build-pr62
cmake --build . --config Release 2>&1
ctest -C Release -R "tableheader_unit|micabackdrop_unit|source_contracts" --output-on-failure 2>&1