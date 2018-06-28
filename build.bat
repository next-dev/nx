@echo off

set "folder=%cd%"

if not exist "_build\nx.sln" (
	call gen.bat
)

if "%VSINSTALLDIR%"=="" (
	if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat" (
	    call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"
	) else (
	    if exist "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvars64.bat" (
	        call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvars64.bat"
	    )
	)
)
cd /d %folder%
pushd _build
msbuild nx.sln /t:rebuild /p:configuration=Release /p:platform=Win64 /m
popd
