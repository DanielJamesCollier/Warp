@echo off
REM Batch file to run clang-format on all .h and .c files recursively
REM Uses Chromium style

setlocal
set CLANG_FORMAT="clang-format.exe"

REM Check if clang-format.exe exists
where %CLANG_FORMAT% >nul 2>nul
if errorlevel 1 (
    echo Error: clang-format.exe not found. Ensure it's in your PATH.
    exit /b 1
)

echo Formatting all .h and .c files using Chromium style...
for /r %%f in (*.h *.c) do (
    echo Formatting: %%f
    %CLANG_FORMAT% -i --style=chromium "%%f"
)

echo Done!
endlocal
pause
