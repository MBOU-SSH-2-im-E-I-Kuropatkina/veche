@echo off
chcp 65001 > nul
setlocal enabledelayedexpansion

:: Переходим в папку, где лежит сам батник
cd /d "%~dp0"

echo [Вече] Сборка...

:: Собираем список всех .cpp файлов из папки src в одну строчку
set "srcs="
for %%f in (src\*.cpp) do (
    set "srcs=!srcs! %%f"
)

:: Запуск компиляции со всеми флагами
g++ -std=c++17 -O2 -Wall -Wextra ^
    -finput-charset=UTF-8 -fexec-charset=UTF-8 ^
    -static -static-libgcc -static-libstdc++ ^
    -o veche.exe !srcs! -I src -lgdi32

if %errorlevel% neq 0 (
    echo [ОШИБКА] Сборка не удалась.
    pause
    exit /b 1
)

echo [OK] Собран veche.exe

:: Проверяем версию собранного экзешника
if exist veche.exe (
    veche.exe -v
) else (
    echo [ОШИБКА] Файл veche.exe не найден после компиляции.
)

pause
