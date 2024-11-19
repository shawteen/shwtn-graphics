@echo off

set CodeDir=..\code
set DataDir=..\data
set OutputDir=..\build_win32

If NOT EXIST %DataDir% mkdir %DataDir%
If NOT EXIST %OutputDir% mkdir %OutputDir%

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

set CompilerFlags=-Od -nologo -Zi
set LinkerFlags=-incremental:no -opt:ref user32.lib gdi32.lib

pushd %DataDir%
pushd %OutputDir%

del *.pdb > NUL 2> NUL

call cl.exe %CompilerFlags% %CodeDir%\win32_graphics.cpp -Fmwin32_graphics.map /link %LinkerFlags%
popd
