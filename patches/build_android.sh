#!/usr/bin/env bash
# Собирает libbox.aar для Android через gomobile bind.
# Использование: ./patches/build_android.sh [abis] (по умолчанию arm64,arm,amd64)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
. "$ROOT/patches/VERSION"

ABIS="${1:-arm64,arm,amd64}"
PLATFORM="android/$(echo "$ABIS" | sed 's/,/,android\//g')"

for tool in gomobile gobind; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "нет $tool в PATH — установите:" >&2
    echo "  go install github.com/sagernet/gomobile/cmd/gomobile@$GOMOBIL_VERSION" >&2
    echo "  go install github.com/sagernet/gomobile/cmd/gobind@$GOMOBIL_VERSION" >&2
    exit 1
  fi
done

"$ROOT/scripts/fetch-sing-box.sh"

cd "$ROOT/.tooling/sing-box"
go mod download

rm -f libbox.aar libbox-legacy.aar
echo "gomobile bind: $PLATFORM (API $ANDROID_API)"
go run ./cmd/internal/build_libbox -target android -platform "$PLATFORM"

mkdir -p "$ROOT/build/android"
mv libbox.aar libbox-legacy.aar "$ROOT/build/android/"
ls -la "$ROOT/build/android"
sha256sum "$ROOT/build/android/"*.aar
