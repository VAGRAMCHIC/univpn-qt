#!/usr/bin/env bash
# Клонирует исходники sing-box закреплённого тега в .tooling/sing-box.
# Применяет патчи из patches/.
# Использование: ./scripts/fetch-sing-box.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
. "$ROOT/patches/VERSION"

SRC="$ROOT/.tooling/sing-box"

if [ ! -d "$SRC/.git" ]; then
  echo "клонирую sing-box $SING_BOX_TAG"
  rm -rf "$SRC"
  git clone --quiet --depth 1 --branch "$SING_BOX_TAG" \
    https://github.com/SagerNet/sing-box "$SRC"
fi

actual="$(git -C "$SRC" rev-parse HEAD)"
if [ "$actual" != "$SING_BOX_COMMIT" ]; then
  echo "коммит sing-box не совпадает: $actual != $SING_BOX_COMMIT" >&2
  echo "пересоздайте клон: rm -rf .tooling/sing-box && ./scripts/fetch-sing-box.sh" >&2
  exit 1
fi

# Патчи из patches/
for patch in "$ROOT"/patches/*.patch; do
  [ -e "$patch" ] || continue
  if ! git -C "$SRC" apply --reverse --check "$patch" 2>/dev/null; then
    git -C "$SRC" apply "$patch"
    echo "применён патч $(basename "$patch")"
  fi
done

echo "sing-box $SING_BOX_TAG ($actual) на месте"
