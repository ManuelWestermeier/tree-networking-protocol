@echo off

:A
cls


echo Setting USER to 4...
(
  echo #define USER 4
) > .\src\user.hpp

echo Uploading to COM10...
"C:\Users\Manuel Westermeier\.platformio\penv\Scripts\platformio.exe" run --target upload --upload-port COM10


echo Setting USER to 3...
(
  echo #define USER 3
) > .\src\user.hpp

echo Uploading to COM9...
"C:\Users\Manuel Westermeier\.platformio\penv\Scripts\platformio.exe" run --target upload --upload-port COM9


echo Setting USER to 2...
(
  echo #define USER 2
) > .\src\user.hpp

echo Uploading to COM7...
"C:\Users\Manuel Westermeier\.platformio\penv\Scripts\platformio.exe" run --target upload --upload-port COM7

echo Setting USER to 1...
(
  echo #define USER 1
) > .\src\user.hpp

echo Uploading to COM4...
"C:\Users\Manuel Westermeier\.platformio\penv\Scripts\platformio.exe" run --target upload --upload-port COM4

echo.
echo Completed.

pause
goto A