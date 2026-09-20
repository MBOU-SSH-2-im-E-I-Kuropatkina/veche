#!/usr/bin/env bash
# compile.sh — сборка «Вече» под Linux
set -e
cd "$(dirname "$0")"

echo "[Вече] Сборка под Linux"

SDL_FLAGS=""
if pkg-config --exists sdl2 2>/dev/null; then
    SDL_FLAGS="$(pkg-config --cflags --libs sdl2)"
    echo "[Вече] SDL2 найден — графика будет работать."
else
    echo "[Вече] SDL2 не найден — графика будет заглушкой."
    echo "        Установить: sudo apt install libsdl2-dev"
fi

g++ -std=c++17 -O2 -Wall -Wextra \
    -o veche \
    src/main.cpp \
    src/graphics.cpp \
    src/lexer.cpp \
    src/parser.cpp \
    src/ast.cpp \
    src/analyzer.cpp \
    src/interpreter.cpp \
    src/value.cpp \
    src/builtins.cpp \
    src/errors.cpp \
    src/utf8.cpp \
    -I src \
    $SDL_FLAGS

echo "[OK] Собран ./veche"
./veche -v