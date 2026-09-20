@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul

REM ============================================================
REM  install.bat — установка «Вече» на Windows
REM  1. Добавляет папку в пользовательский PATH
REM  2. Регистрирует .veche / .vech как "Интерпретируемый файл Вече"
REM  3. Ассоциирует двойной клик с veche.exe
REM  4. Ставит иконку, если рядом есть veche.ico
REM ============================================================

set "VECHE_DIR=%~dp0"
if "%VECHE_DIR:~-1%"=="\" set "VECHE_DIR=%VECHE_DIR:~0,-1%"

set "VECHE_EXE=%VECHE_DIR%\veche.exe"
set "VECHE_ICO=%VECHE_DIR%\veche.ico"

echo.
echo ============================================================
echo   Установка «Вече»
echo   Папка: %VECHE_DIR%
echo ============================================================
echo.

if not exist "%VECHE_EXE%" (
    echo [ОШИБКА] Не найден %VECHE_EXE%
    echo Сначала соберите:  pwsh -File compile.ps1
    echo.
    pause
    exit /b 1
)

REM ============================================================
REM  1. PATH
REM ============================================================
echo [1/3] Добавление в PATH...

set "USER_PATH="
for /f "usebackq tokens=1,2,*" %%A in (`reg query "HKCU\Environment" /v Path 2^>nul`) do (
    set "USER_PATH=%%C"
)

if not defined USER_PATH (
    set "NEW_PATH=%VECHE_DIR%"
    goto :write_path
)

echo ;!USER_PATH!; | find /I ";%VECHE_DIR%;" >nul
if not errorlevel 1 (
    echo   Путь уже в PATH — пропускаю.
    goto :path_done
)

set "NEW_PATH=!USER_PATH!;%VECHE_DIR%"

:write_path
reg add "HKCU\Environment" /v Path /t REG_EXPAND_SZ /d "!NEW_PATH!" /f >nul
if errorlevel 1 (
    echo   [ПРЕДУПРЕЖДЕНИЕ] Не удалось записать PATH.
) else (
    echo   PATH обновлён.
)

:path_done

REM ============================================================
REM  2. ProgID «Вече.Файл»
REM ============================================================
echo [2/3] Регистрация ProgID «Вече.Файл»...

reg add "HKCU\Software\Classes\Вече.Файл" /ve /t REG_SZ /d "Интерпретируемый файл Вече" /f >nul

if exist "%VECHE_ICO%" (
    reg add "HKCU\Software\Classes\Вече.Файл\DefaultIcon" /ve /t REG_SZ /d "%VECHE_ICO%,0" /f >nul
) else (
    reg add "HKCU\Software\Classes\Вече.Файл\DefaultIcon" /ve /t REG_SZ /d "%VECHE_EXE%,0" /f >nul
)

reg add "HKCU\Software\Classes\Вече.Файл\shell\open\command" /ve /t REG_SZ /d "\"%VECHE_EXE%\" \"%%1\"" /f >nul

echo   ProgID зарегистрирован.

REM ============================================================
REM  3. Ассоциация .veche / .vech
REM ============================================================
echo [3/3] Ассоциация расширений...

reg add "HKCU\Software\Classes\.veche" /ve /t REG_SZ /d "Вече.Файл" /f >nul
reg add "HKCU\Software\Classes\.vech"  /ve /t REG_SZ /d "Вече.Файл" /f >nul

reg add "HKCU\Software\Classes\.veche\OpenWithProgids" /v "Вече.Файл" /t REG_NONE /f >nul
reg add "HKCU\Software\Classes\.vech\OpenWithProgids"  /v "Вече.Файл" /t REG_NONE /f >nul

reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.veche\UserChoice" /f >nul 2>&1
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.vech\UserChoice"  /f >nul 2>&1

echo   Ассоциации установлены.

echo.
echo Обновление ассоциаций в Проводнике...
taskkill /f /im explorer.exe >nul 2>&1
start "" explorer.exe

echo.
echo ============================================================
echo   Готово.
echo ============================================================
echo.
echo   Проверьте:
echo     - Новое окно cmd:   veche -v
echo     - Двойной клик на .veche
echo.

pause
endlocal