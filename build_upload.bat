@echo off

:A
cls

REM --- Set sender mode and upload to COM4 ---
echo Setting IS_SENDER to 1...
(
  echo #define IS_SENDER 1
) > .\src\user.hpp

REM Run PlatformIO upload for COM4 (adjust path if needed)
echo Uploading to COM4...
"C:\Users\Manuel Westermeier\.platformio\penv\Scripts\platformio.exe" run --target upload --upload-port COM4

REM Optional delay if needed

REM --- Set receiver mode and upload to COM6 ---
echo Setting IS_SENDER to 0...
(
  echo #define IS_SENDER 0
) > .\src\user.hpp

echo Uploading to COM6...
"C:\Users\Manuel Westermeier\.platformio\penv\Scripts\platformio.exe" run --target upload --upload-port COM6


echo.
echo Completed.

pause
goto A