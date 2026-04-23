#!/usr/bin/env sh

set -eu

usage() {
    cat <<'EOF'
Usage: sh decompiler/install.sh [--prefix DIR] [--bin-dir DIR] [--help]

Builds the decompiler and installs:
  - the CLI binary at <bin-dir>/decompile

Defaults:
  - root:    prefix=/usr/local
  - non-root: prefix=$HOME/.local

Overrides:
  PREFIX   Install prefix
  BIN_DIR  Install bin directory

Examples:
  sh decompiler/install.sh
  sh decompiler/install.sh --prefix /usr/local
  BIN_DIR=$HOME/bin sh decompiler/install.sh
EOF
}

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        printf 'error: required command not found: %s\n' "$1" >&2
        exit 1
    fi
}

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)

PREFIX_SET=0
BIN_DIR_SET=0

if [ "${PREFIX+x}" = x ]; then
    PREFIX_SET=1
else
    if [ "$(id -u)" -eq 0 ]; then
        PREFIX=/usr/local
    else
        PREFIX=$HOME/.local
    fi
fi

if [ "${BIN_DIR+x}" = x ]; then
    BIN_DIR_SET=1
else
    BIN_DIR=$PREFIX/bin
fi

while [ "$#" -gt 0 ]; do
    case "$1" in
        --prefix)
            [ "$#" -ge 2 ] || {
                printf 'error: --prefix requires a value\n' >&2
                exit 1
            }
            PREFIX=$2
            PREFIX_SET=1
            if [ "$BIN_DIR_SET" -eq 0 ]; then
                BIN_DIR=$PREFIX/bin
            fi
            shift 2
            ;;
        --bin-dir)
            [ "$#" -ge 2 ] || {
                printf 'error: --bin-dir requires a value\n' >&2
                exit 1
            }
            BIN_DIR=$2
            BIN_DIR_SET=1
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            printf 'error: unknown argument: %s\n\n' "$1" >&2
            usage >&2
            exit 1
            ;;
    esac
done

require_command make
require_command install
require_command gcc

BUILD_DIR=$SCRIPT_DIR/build
BUILD_TARGET=build/decompile
SOURCE_BIN=$BUILD_DIR/decompile
INSTALL_BIN=$BIN_DIR/decompile

restore_build_owner() {
    if [ "$(id -u)" -ne 0 ]; then
        return
    fi
    if [ -z "${SUDO_UID:-}" ] || [ -z "${SUDO_GID:-}" ] || [ ! -d "$BUILD_DIR" ]; then
        return
    fi
    chown -R "$SUDO_UID:$SUDO_GID" "$BUILD_DIR"
}

trap restore_build_owner EXIT

printf '==> Building decompile\n'
make -C "$SCRIPT_DIR" clean >/dev/null
make -C "$SCRIPT_DIR" "$BUILD_TARGET" >/dev/null

if [ ! -f "$SOURCE_BIN" ]; then
    printf 'error: build did not produce %s\n' "$SOURCE_BIN" >&2
    exit 1
fi

printf '==> Installing decompile\n'
install -d "$BIN_DIR"
install -m 755 "$SOURCE_BIN" "$INSTALL_BIN"

printf '\nInstalled decompile:\n'
printf '  binary: %s\n' "$INSTALL_BIN"

case ":${PATH}:" in
    *":$BIN_DIR:"*)
        ;;
    *)
        printf '\n%s is not on your PATH. Add this line to your shell profile:\n' "$BIN_DIR"
        printf '  export PATH="%s:$PATH"\n' "$BIN_DIR"
        ;;
esac

printf '\nTry: decompile --help\n'