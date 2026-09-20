@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul

set "VECHE_DIR=%~dp0"
if "%VECHE_DIR:~-1%"=="\" set "VECHE_DIR=%VECHE_DIR:~0,-1%"

echo.
echo ============================================================
echo   Удаление «Вече» из системы
echo ============================================================
echo.

reg delete "HKCU\Software\Classes\Вече.Файл" /f >nul 2>&1
reg delete "HKCU\Software\Classes\.veche"     /f >nul 2>&1
reg delete "HKCU\Software\Classes\.vech"      /f >nul 2>&1
echo Ассоциации удалены.

set "USER_PATH="
for /f "usebackq tokens=1,2,*" %%A in (`reg query "HKCU\Environment" /v Path 2^>nul`) do (
    set "USER_PATH=%%C"
)

if not defined USER_PATH goto :path_done

set "NEW_PATH=!USER_PATH!"
set "NEW_PATH=!NEW_PATH:;%VECHE_DIR%=!"
set "NEW_PATH=!NEW_PATH:%VECHE_DIR%;=!"
set "NEW_PATH=!NEW_PATH:%VECHE_DIR%=!"

if "!NEW_PATH!"=="!USER_PATH!" (
    echo PATH не содержал нашу папку — пропускаю.
) else (
    reg add "HKCU\Environment" /v Path /t REG_EXPAND_SZ /d "!NEW_PATH!" /f >nul
    echo PATH обновлён.
)

:path_done

taskkill /f /im explorer.exe >nul 2>&1
start "" explorer.exe

echo.
echo Готово. Перезапустите cmd.
pause
endlocal