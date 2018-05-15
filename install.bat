@echo off
if [%INSTALL_PATH%] NEQ [] (
    copy /y _bin\Win64\Release\nx\nx.exe %INSTALL_PATH%
) else (
    echo Please define INSTALL_PATH in the environment
)

