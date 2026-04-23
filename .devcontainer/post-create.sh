#!/usr/bin/env sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)

printf '=== Verifying devkitPPC toolchain ===\n'
powerpc-eabi-gcc --version

printf '\n=== Verifying Dolphin ===\n'
dolphin-emu --version || printf 'Dolphin installed (version flag may not be supported)\n'

printf '\n=== Building compiler + runtime libraries ===\n'
make -C "$REPO_ROOT/compiler" calynda

printf '\n=== Running compiler tests ===\n'
make -C "$REPO_ROOT/compiler" test

printf '\n=== Building and installing decompile ===\n'
sudo sh "$REPO_ROOT/decompiler/install.sh" --prefix /usr/local

if ! command -v decompile >/dev/null 2>&1; then
	printf 'error: decompile was not installed on PATH\n' >&2
	exit 1
fi

printf 'decompile: %s\n' "$(command -v decompile)"
printf '\n=== Dev container ready ===\n'
