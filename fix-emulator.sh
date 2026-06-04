#!/usr/bin/env bash
#
# fix-emulator.sh — recover a stuck / broken Pebble QEMU emulator.
#
# Two classes of problem this handles:
#
#  1) STUCK STATE (most common): a qemu-pebble process crashed into a zombie
#     and the stale /tmp/pb-emulator.json still points at dead PIDs, so every
#     new `pebble install --emulator ...` hangs. Reinstalling the SDK does NOT
#     fix this — leftover runtime state is the problem.
#
#  2) CORRUPTED PLATFORM FLASH (e.g. gabbro "App install failed" /
#     WatchVersion TimeoutError): the emulator's persisted SPI-flash image at
#     ~/.pebble-sdk/<ver>/<platform>/qemu_spi_flash.bin is bad. Restoring it
#     from the pristine copy shipped in the SDK gives a clean boot.
#
# Usage:
#   ./fix-emulator.sh                 # just clear stuck state (all platforms)
#   ./fix-emulator.sh emery           # clear state, then relaunch + screenshot emery
#   ./fix-emulator.sh gabbro --flash  # ALSO restore gabbro's SPI flash from the SDK
#   ./fix-emulator.sh --flash         # restore flash for ALL platforms, then stop
#
set -u

STATE_FILE="/tmp/pb-emulator.json"
SDK_VER="$(ls -1 ~/.pebble-sdk/SDKs 2>/dev/null | grep -E '^[0-9]' | sort -V | tail -1)"
[ -z "${SDK_VER:-}" ] && SDK_VER="4.9.169"
PRISTINE_BASE="$HOME/.pebble-sdk/SDKs/$SDK_VER/sdk-core/pebble"
WORK_BASE="$HOME/.pebble-sdk/$SDK_VER"
ALL_PLATFORMS="aplite basalt chalk diorite emery flint gabbro"

PLATFORM=""
DO_FLASH=0
for arg in "$@"; do
  case "$arg" in
    --flash) DO_FLASH=1 ;;
    -*)      echo "Unknown option: $arg" ;;
    *)       PLATFORM="$arg" ;;
  esac
done

# ── Restore one platform's persisted SPI flash from the pristine SDK copy ──
restore_flash() {
  local plat="$1"
  local bz2="$PRISTINE_BASE/$plat/qemu/qemu_spi_flash.bin.bz2"
  local dest="$WORK_BASE/$plat/qemu_spi_flash.bin"
  if [ ! -f "$bz2" ]; then
    echo "    [$plat] no pristine flash found at $bz2 — skipping"
    return
  fi
  mkdir -p "$WORK_BASE/$plat"
  rm -f "$dest"
  if bunzip2 -kc "$bz2" > "$dest" 2>/dev/null; then
    echo "    [$plat] restored SPI flash from SDK ($(du -h "$dest" | cut -f1))"
  else
    echo "    [$plat] FAILED to decompress $bz2"
  fi
}

echo "==> Killing hung pebble / qemu / pypkjs processes..."
pkill -9 -f "qemu-pebble"       2>/dev/null && echo "    killed qemu-pebble"        || true
pkill -9 -f "pebble install"    2>/dev/null && echo "    killed pebble install"     || true
pkill -9 -f "pebble .*emulator" 2>/dev/null && echo "    killed pebble emulator cmd"|| true
pkill -9 -f "pypkjs"            2>/dev/null && echo "    killed pypkjs"              || true

if [ -f "$STATE_FILE" ]; then
  for pid in $(grep -oE '"pid":[[:space:]]*[0-9]+' "$STATE_FILE" | grep -oE '[0-9]+'); do
    kill -9 "$pid" 2>/dev/null && echo "    killed PID $pid from state file" || true
  done
fi

echo "==> pebble kill (best effort)..."
timeout 30 pebble kill >/dev/null 2>&1 && echo "    ok" || echo "    skipped/timed out"

echo "==> Removing stale state file $STATE_FILE..."
rm -f "$STATE_FILE" && echo "    removed (or already gone)"

# ── Optional flash restore ──
if [ "$DO_FLASH" = "1" ]; then
  echo "==> Restoring SPI flash from SDK $SDK_VER..."
  if [ -n "$PLATFORM" ]; then
    restore_flash "$PLATFORM"
  else
    for p in $ALL_PLATFORMS; do
      [ -e "$WORK_BASE/$p/qemu_spi_flash.bin" ] && restore_flash "$p"
    done
  fi
fi

sleep 1
REMAIN=$(pgrep -af "qemu-pebble|pypkjs" 2>/dev/null | grep -v grep)
[ -n "$REMAIN" ] && { echo "==> WARNING, still running:"; echo "$REMAIN"; } || echo "==> Clean. No emulator processes running."

# ── Optional relaunch + verify ──
if [ -n "$PLATFORM" ]; then
  echo "==> Relaunching emulator: $PLATFORM"
  if timeout 150 pebble install --emulator "$PLATFORM"; then
    echo "==> Install OK; capturing screenshot to confirm it renders..."
    timeout 40 pebble screenshot --no-open --emulator "$PLATFORM" "fix-emulator-check.png" \
      && echo "==> OK — saved fix-emulator-check.png" \
      || echo "==> Screenshot failed."
  else
    echo "==> Install failed/timed out."
    echo "    If this is gabbro with a 'WatchVersion TimeoutError', try:"
    echo "      ./fix-emulator.sh gabbro --flash"
    echo "    and if it still fails, reinstall the SDK firmware:"
    echo "      pebble sdk install $SDK_VER"
  fi
else
  echo "==> Done. Now run, e.g.:  pebble install --emulator emery"
  echo "    Deeper reset (corrupted flash):  ./fix-emulator.sh <platform> --flash"
fi
