@echo off
setlocal enabledelayedexpansion

:: ПУТЬ К LLVM
set LLVM_PATH=G:\clang+llvm-23.1.3-x86_64-pc-windows-msvc\clang+llvm-23.1.3-x86_64-pc-windows-msvc

:: Проверяем наличие clang++
if not exist "%LLVM_PATH%\bin\clang++.exe" (
    echo Ошибка: clang++.exe не найден
    pause
    exit /b 1
)

echo ========================================
echo Компиляция Вече с LLVM бэкендом
echo ========================================
echo.

:: Собираем все .cpp файлы из src\
set SRC_FILES=
for %%f in (src\*.cpp) do (
    set SRC_FILES=!SRC_FILES! %%f
)

echo Исходные файлы:%SRC_FILES%
echo.

:: Получаем список библиотек LLVM
echo Получение списка библиотек LLVM...
set LLVM_LIBS=
for /f "delims=" %%i in ('"%LLVM_PATH%\bin\llvm-config.exe" --libs core support irreader target mc mcparser passes transformutils analysis bitwriter') do set LLVM_LIBS=%%i

:: Компиляция через clang++ с явным указанием таргета
echo Компиляция проекта...
"%LLVM_PATH%\bin\clang++.exe" -std=c++17 -O2 -Wall -Wextra ^
    --target=x86_64-pc-windows-msvc ^
    -fms-compatibility-version=19.35 ^
    -finput-charset=UTF-8 -fexec-charset=UTF-8 ^
    -I "%LLVM_PATH%\include" ^
    -I src ^
    %SRC_FILES% ^
    -L "%LLVM_PATH%\lib" ^
    %LLVM_LIBS% ^
    -lgdi32 ^
    -o veche.exe

if %errorlevel% neq 0 (
    echo.
    echo Ошибка компиляции!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Успешно скомпилировано в veche.exe!
echo ========================================
echo.

pause
