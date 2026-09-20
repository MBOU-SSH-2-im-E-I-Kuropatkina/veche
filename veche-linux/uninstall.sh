#!/usr/bin/env bash
set -e

rm -f "$HOME/.local/bin/veche"
rm -f "$HOME/.local/share/applications/veche.desktop"
rm -f "$HOME/.local/share/mime/packages/veche.xml"
rm -f "$HOME/.local/share/icons/hicolor/256x256/apps/veche.png"

update-mime-database "$HOME/.local/share/mime" 2>/dev/null || true
update-desktop-database "$HOME/.local/share/applications" 2>/dev/null || true

echo "Удалено."