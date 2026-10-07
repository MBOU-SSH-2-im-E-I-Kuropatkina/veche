@echo off
setlocal enabledelayedexpansion

:: ПУТЬ К LLVM (ЗАМЕНИ НА СВОЙ!)
set LLVM_PATH=G:\clang+llvm-23.1.3-x86_64-pc-windows-msvc\clang+llvm-23.1.3-x86_64-pc-windows-msvc

:: Проверяем наличие clang++
if not exist "%LLVM_PATH%\bin\clang++.exe" (
    echo Ошибка: clang++ не найден в %LLVM_PATH%\bin\
    echo Проверь путь к LLVM
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

:: Получаем список библиотек LLVM через llvm-config
echo Получение списка библиотек LLVM...
set LLVM_LIBS=
for /f "delims=" %%i in ('"%LLVM_PATH%\bin\llvm-config.exe" --libs core support irreader target mc mcparser passes transformutils analysis bitwriter') do set LLVM_LIBS=%%i

:: Компиляция через clang++
echo Компиляция проекта...
"%LLVM_PATH%\bin\clang++.exe" -std=c++17 -O2 -Wall -Wextra ^
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
echo Использование:
echo   veche.exe script.veche              - интерпретация
echo   veche.exe -c script.veche           - компиляция в output.obj
echo   veche.exe -c script.veche -o out.obj - компиляция с именем файла
echo   veche.exe -i                        - REPL
echo   veche.exe -v                        - версия
echo   veche.exe -h                        - помощь
echo.

pause