#!/usr/bin/env bash
set -e

echo "=== Verifying devkitPPC toolchain ==="
powerpc-eabi-gcc --version

echo ""
echo "=== Verifying Dolphin ==="
dolphin-emu --version || echo "Dolphin installed (version flag may not be supported)"

echo ""
echo "=== Building compiler ==="
make -C compiler all

echo ""
echo "=== Running compiler tests ==="
make -C compiler test

echo ""
echo "=== Dev container ready ==="
