@echo off

if not exist build mkdir build
cd build

set vswhere="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set cmakeLookup=call %vswhere% -latest -requires Microsoft.VisualStudio.Component.VC.CMake.Project -find Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe

for /f "tokens=*" %%i in ('%cmakeLookup%') do set cmake="%%i"

%cmake% ..
%cmake% --build . --config Release

cd ..
