#!/usr/bin/env bash
# install.sh — установка «Вече» на Linux
# 1. Копирует veche в ~/.local/bin
# 2. Регистрирует MIME-тип text/x-veche для .veche / .vech
# 3. Ставит .desktop-файл (двойной клик → veche)
# 4. Копирует иконку, если есть veche.png
set -e

SRC_DIR="$(cd "$(dirname "$0")" && pwd)"
BIN_SRC="$SRC_DIR/veche"

if [ ! -f "$BIN_SRC" ]; then
    echo "[ОШИБКА] Не найден $BIN_SRC"
    echo "Сначала соберите: ./compile.sh"
    exit 1
fi

BIN_DIR="$HOME/.local/bin"
APP_DIR="$HOME/.local/share/applications"
MIME_DIR="$HOME/.local/share/mime/packages"
ICON_DIR="$HOME/.local/share/icons/hicolor/256x256/apps"

mkdir -p "$BIN_DIR" "$APP_DIR" "$MIME_DIR" "$ICON_DIR"

# --- 1. Бинарник ---
cp "$BIN_SRC" "$BIN_DIR/veche"
chmod +x "$BIN_DIR/veche"
echo "[1/4] Установлен $BIN_DIR/veche"

# --- 2. Иконка ---
if [ -f "$SRC_DIR/veche.png" ]; then
    cp "$SRC_DIR/veche.png" "$ICON_DIR/veche.png"
    echo "[2/4] Установлена иконка"
else
    echo "[2/4] veche.png не найден — пропускаю"
fi

# --- 3. MIME-тип ---
cp "$SRC_DIR/text-x-veche.xml" "$MIME_DIR/veche.xml"
update-mime-database "$HOME/.local/share/mime" 2>/dev/null || true
echo "[3/4] MIME-тип зарегистрирован"

# --- 4. .desktop ---
sed "s|^Exec=.*|Exec=$BIN_DIR/veche %f|" \
    "$SRC_DIR/veche.desktop" > "$APP_DIR/veche.desktop"
update-desktop-database "$APP_DIR" 2>/dev/null || true
echo "[4/4] .desktop-файл установлен"

# --- PATH подсказка ---
case ":$PATH:" in
    *":$BIN_DIR:"*) ;;
    *)
        echo ""
        echo "ВНИМАНИЕ: $BIN_DIR не в PATH."
        echo "Добавьте в ~/.bashrc или ~/.profile:"
        echo ""
        echo "    export PATH=\"\$HOME/.local/bin:\$PATH\""
        echo ""
        echo "Затем: source ~/.bashrc"
        ;;
esac

echo ""
echo "Готово. Проверьте:"
echo "    veche -v"
echo "    veche ../veche_linux/examples/hello.veche"