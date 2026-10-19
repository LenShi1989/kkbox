@echo off
chcp 65001 >nul
setlocal enabledelayedexpansion

rem ============================================================
rem  build_spiffs.bat
rem  將 sketch 的 data\ 目錄打包成 SPIFFS 映像檔 build\spiffs.bin
rem
rem  用法:
rem    build_spiffs.bat <sketch資料夾> [SPIFFS大小(bytes)]
rem
rem  範例:
rem    build_spiffs.bat WT32_ETH01_receiver
rem    build_spiffs.bat ESP32_D1_mini_rc
rem    build_spiffs.bat ESP32_D1_mini_rc 0x20000
rem
rem  預設大小(依專案/分割表):
rem    WT32_ETH01_receiver : 0x20000   (Minimal SPIFFS 1.9MB APP)
rem    ESP32_D1_mini_rc    : 0x20000   (Minimal SPIFFS 1.9MB APP)
rem
rem  輸出: <sketch>\build\spiffs.bin
rem
rem  可用環境變數 MKSPIFFS_EXE 指定 mkspiffs.exe 路徑;
rem  否則自動由 PATH 或 Arduino15 ESP32 工具鏈尋找。
rem ============================================================

set "SKETCH=%~1"
if not defined SKETCH set "SKETCH=%CD%"
for %%I in ("%SKETCH%") do set "SKETCH=%%~fI"

if not exist "%SKETCH%\data\" (
  echo [錯誤] 找不到 data 資料夾: "%SKETCH%\data"
  exit /b 1
)

rem ---- 輸出資料夾 (build\) ----
set "OUT_DIR=%SKETCH%\build"
if not exist "%OUT_DIR%\" mkdir "%OUT_DIR%"
set "OUT=%OUT_DIR%\spiffs.bin"

rem ---- 決定 SPIFFS 分割區大小 ----
set "SIZE=%~2"
if not defined SIZE (
  echo !SKETCH! | findstr /I "WT32_ETH01_receiver" >nul
  if not errorlevel 1 set "SIZE=0x20000"
)
if not defined SIZE (
  echo !SKETCH! | findstr /I "ESP32_D1_mini_rc" >nul
  if not errorlevel 1 set "SIZE=0x20000"
)
if not defined SIZE (
  echo [錯誤] 無法判斷 SPIFFS 大小, 請以第二個參數指定 ^(例: 0x160000^)
  exit /b 1
)
set /a "SIZE_DEC=!SIZE!"

rem ---- 尋找 mkspiffs.exe ----
set "MKSPIFFS="
if defined MKSPIFFS_EXE set "MKSPIFFS=%MKSPIFFS_EXE%"
if not defined MKSPIFFS for /f "delims=" %%P in ('where mkspiffs 2^>nul') do if not defined MKSPIFFS set "MKSPIFFS=%%P"
if not defined MKSPIFFS (
  for /f "delims=" %%P in ('dir /b /s /o-n "%LOCALAPPDATA%\Arduino15\packages\esp32\tools\mkspiffs\mkspiffs.exe" 2^>nul') do if not defined MKSPIFFS set "MKSPIFFS=%%P"
)
if not defined MKSPIFFS (
  echo [錯誤] 找不到 mkspiffs.exe
  echo         請安裝 ESP32 開發板核心, 或設定環境變數 MKSPIFFS_EXE 指向 mkspiffs.exe
  exit /b 1
)

echo.
echo   sketch   : !SKETCH!
echo   mkspiffs : !MKSPIFFS!
echo   size     : !SIZE! ^(!SIZE_DEC! bytes^)
echo   打包 data\ ^-^> build\spiffs.bin ...
echo.

"!MKSPIFFS!" -c "!SKETCH!\data" -p 256 -b 4096 -s !SIZE_DEC! "%OUT%"

echo.
echo [完成] 已產生: "%OUT%"
echo.
echo 燒錄範例 (依實際 COM 埠與 partition offset):
echo   esptool.py --chip esp32 --port COMx write_flash 0x3D0000 "%OUT%"
echo   兩者皆為 Minimal SPIFFS 分割表, offset=0x3D0000
