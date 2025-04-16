@echo off

:A
cls

echo Setting IS_SENDER to 0...
(
  echo #define IS_SENDER 0
) > .\src\user.hpp

echo Uploading to COM6...
"C:\Users\Manuel Westermeier\.platformio\penv\Scripts\platformio.exe" run --target upload --upload-port COM6

echo Setting IS_SENDER to 1...
(
  echo #define IS_SENDER 1
) > .\src\user.hpp

echo Uploading to COM4...
"C:\Users\Manuel Westermeier\.platformio\penv\Scripts\platformio.exe" run --target upload --upload-port COM4

echo.
echo Completed.

pause
goto A