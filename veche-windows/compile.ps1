# compile.ps1 — сборка «Вече» под Windows
$ErrorActionPreference = "Stop"
Set-Location -Path $PSScriptRoot

Write-Host "[Вече] Сборка..." -ForegroundColor Cyan

$srcs = (Get-ChildItem src\*.cpp).FullName

g++ -std=c++17 -O2 -Wall -Wextra `
    -finput-charset=UTF-8 -fexec-charset=UTF-8 `
    -static -static-libgcc -static-libstdc++ `
    -o veche.exe $srcs -I src -lgdi32

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ОШИБКА] Сборка не удалась." -ForegroundColor Red
    exit 1
}

Write-Host "[OK] Собран veche.exe" -ForegroundColor Green
.\veche.exe -v