@echo off

:A
cls

echo Uploading to COM4...
"C:\Users\Manuel Westermeier\.platformio\penv\Scripts\platformio.exe" run --target upload --upload-port COM4

echo Uploading to COM10...
"C:\Users\Manuel Westermeier\.platformio\penv\Scripts\platformio.exe" run --target upload --upload-port COM7

echo.
echo Completed.

pause
goto A