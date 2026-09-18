@echo off
chcp 65001
setlocal

set "VECHE_DIR=%CD%"

echo Добавляю в PATH: %VECHE_DIR%

REM Проверим, нет ли уже такого пути в пользовательском PATH
for /f "usebackq tokens=2*" %%A in (`reg query "HKCU\Environment" /v Path 2^>nul`) do set "USER_PATH=%%B"

echo %USER_PATH% | find /I "%VECHE_DIR%" >nul
if not errorlevel 1 (
    echo Путь уже присутствует в PATH. Ничего не меняю.
    goto :eof
)

REM setx пишет в пользовательский PATH. Если USER_PATH пуст — ставим только наш путь.
if "%USER_PATH%"=="" (
    setx PATH "%VECHE_DIR%"
) else (
    setx PATH "%USER_PATH%;%VECHE_DIR%"
)

echo Готово. Перезапустите cmd, чтобы изменения вступили в силу.
endlocal
