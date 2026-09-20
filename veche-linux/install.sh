#!/usr/bin/env bash
# install.sh — установка «Вече» на Linux
# Копирует veche в ~/.local/bin.
set -e

SRC_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN_SRC="$SRC_DIR/veche"

if [ ! -f "$BIN_SRC" ]; then
    echo "[ОШИБКА] Не найден $BIN_SRC"
    echo "Сначала соберите: ./compile.sh"
    exit 1
fi

BIN_DIR="$HOME/.local/bin"
mkdir -p "$BIN_DIR"

cp "$BIN_SRC" "$BIN_DIR/veche"
chmod +x "$BIN_DIR/veche"
echo "[OK] Установлен $BIN_DIR/veche"

case ":$PATH:" in
    *":$BIN_DIR:"*) ;;
    *)
        echo ""
        echo "ВНИМАНИЕ: $BIN_DIR не в PATH."
        echo "Добавьте в ~/.bashrc:"
        echo ""
        echo "    export PATH=\"\$HOME/.local/bin:\$PATH\""
        echo ""
        echo "Затем: source ~/.bashrc"
        ;;
esac

echo ""
echo "Готово. Проверьте:"
echo "    veche -v"
echo "    veche examples/hello.veche"
echo "    veche -r"
