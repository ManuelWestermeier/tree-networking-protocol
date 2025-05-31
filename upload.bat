@echo off
:A
cls

if "%~1"=="" (
    echo Please provide one or more COM ports as arguments.
    echo Example: upload.bat COM4 COM7 COM10
    pause
    exit /b
)

for %%P in (%*) do (
    echo Uploading to %%P...
    "C:\Users\Manuel Westermeier\.platformio\penv\Scripts\platformio.exe" run --target upload --upload-port %%P
    echo.
)

echo All uploads completed.
pause
goto A
