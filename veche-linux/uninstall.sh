#!/usr/bin/env bash
# uninstall.sh — удаление «Вече» из ~/.local
set -e

rm -f "$HOME/.local/bin/veche"

# Чистим возможные остатки от старых установок
rm -f "$HOME/.local/share/icons/hicolor/256x256/apps/veche.png"
rm -f "$HOME/.local/share/applications/veche.desktop"
rm -f "$HOME/.local/share/mime/packages/veche.xml"

update-mime-database "$HOME/.local/share/mime" 2>/dev/null || true
update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true
gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" 2>/dev/null || true

echo "Удалено."