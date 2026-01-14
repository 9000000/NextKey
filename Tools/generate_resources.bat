@echo off
setlocal

:: Define paths relative to this script (inside Tools)
set "SCRIPT_DIR=%~dp0"
:: Project root is one level up from Tools
set "PROJECT_ROOT=%SCRIPT_DIR%..\"
set "PACKFOLDER=%PROJECT_ROOT%Resources\Sciter\packfolder.exe"
set "SCITER_RESOURCES_DIR=%PROJECT_ROOT%Resources\Sciter"
set "OUTPUT_FILE=resources.cpp"

:: Check if packfolder.exe exists
if not exist "%PACKFOLDER%" (
    echo Error: packfolder.exe not found at %PACKFOLDER%
    pause
    exit /b 1
)

:: Navigate to the resources directory
pushd "%SCITER_RESOURCES_DIR%"

:: Run packfolder
echo Running packfolder...
"%PACKFOLDER%" . "%OUTPUT_FILE%" -v "resources"

if %ERRORLEVEL% EQU 0 (
    echo Successfully generated %OUTPUT_FILE%
) else (
    echo Failed to run packfolder. Error code: %ERRORLEVEL%
)

:: Restore original directory
popd

echo Done.
pause
