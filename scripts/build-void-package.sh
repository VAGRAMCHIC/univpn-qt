#!/usr/bin/env bash
# Сборка пакета univpn-qt для Void Linux.
# Требует: git, xbps-src (входит в пакет base-devel или void-packages).
# Использование: ./build-void-package.sh

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VOID_PKGS_DIR="${ROOT}/void-packages"
PKG_NAME="univpn-qt"

# 1. Клонируем void-packages, если его нет
if [ ! -d "$VOID_PKGS_DIR" ]; then
  echo "Клонирую void-packages..."
  git clone --depth 1 https://github.com/void-linux/void-packages.git "$VOID_PKGS_DIR"
  cd "$VOID_PKGS_DIR"
  ./xbps-src binary-bootstrap
  cd "$ROOT"
fi

# 2. Создаем директорию для шаблона
SRC_DIR="$VOID_PKGS_DIR/srcpkgs/$PKG_NAME"
mkdir -p "$SRC_DIR"

# 3. Копируем template и файлы (если есть)
cp "$ROOT/packaging/void/template" "$SRC_DIR/template"

# Если есть дополнительные файлы (иконки, desktop-файл), копируем их в files/
if [ -d "$ROOT/packaging/void/files" ]; then
  cp -r "$ROOT/packaging/void/files" "$SRC_DIR/files"
fi

# 4. Запускаем сборку
cd "$VOID_PKGS_DIR"
echo "Собираю $PKG_NAME..."
./xbps-src pkg "$PKG_NAME"

echo "Готово. Пакет должен быть в $VOID_PKGS_DIR/hostdir/binpkgs/"
echo "Установка: sudo xbps-install --repository=$VOID_PKGS_DIR/hostdir/binpkgs $PKG_NAME"
