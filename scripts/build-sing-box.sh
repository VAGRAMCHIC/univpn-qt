#!/usr/bin/env bash
# Собирает консольный sing-box для настольных платформ.
# Использование: ./scripts/build-sing-box.sh [os] [arch]
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
. "$ROOT/patches/VERSION"

OS="${1:-linux}"
ARCH="${2:-amd64}"

case "$OS" in
windows) TAGS_FILE=release/DEFAULT_BUILD_TAGS_WINDOWS ;;
*) TAGS_FILE=release/DEFAULT_BUILD_TAGS_OTHERS ;;
esac

"$ROOT/scripts/fetch-sing-box.sh"

SRC="$ROOT/.tooling/sing-box"
OUT_DIR="$ROOT/build"
OUT="$OUT_DIR/sing-box"
[ "$OS" = windows ] && OUT="$OUT.exe"
mkdir -p "$OUT_DIR"

cd "$SRC"
TAGS="$(cat "$TAGS_FILE")"
LDFLAGS="-X github.com/sagernet/sing-box/constant.Version=$SING_BOX_TAG \
  -X runtime.godebugDefault=multipathtcp=0,tlssha1=1 -checklinkname=0 -s -w -buildid="

echo "собираю sing-box для $OS/$ARCH"
CGO_ENABLED=0 GOOS="$OS" GOARCH="$ARCH" \
  go build -trimpath -buildvcs=false -tags "$TAGS" -ldflags "$LDFLAGS" \
  -o "$OUT" ./cmd/sing-box

"$OUT" version
sha256sum "$OUT"
