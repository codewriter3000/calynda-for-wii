#!/usr/bin/env sh
set -eu

SCRIPT_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
REPO_ROOT="$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)"
CALYNDA="$REPO_ROOT/compiler/build/calynda"

# Defaults
SRC_DIR="$SCRIPT_DIR/src"
DIST_DIR="$SCRIPT_DIR/dist"
ENTRY="$SRC_DIR/main.cal"
OUTPUT_NAME=""

usage() {
    cat <<EOF
Usage: $(basename "$0") [options]

Compiles Calynda source into .elf and .dol files for Wii.

Options:
  -e, --entry FILE    Entry-point .cal file (default: game/src/main.cal)
  -o, --output NAME   Base name for output files (default: game/dist/<entry>)
  -h, --help          Show this help
EOF
}

usage_error() {
    printf 'error: %s\n\n' "$1" >&2
    usage >&2
    exit 1
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        -e|--entry)
            [ "$#" -ge 2 ] || usage_error '--entry requires a value'
            ENTRY="$2"
            shift 2
            ;;
        -o|--output)
            [ "$#" -ge 2 ] || usage_error '--output requires a value'
            OUTPUT_NAME="$2"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            usage_error "unknown option: $1"
            ;;
    esac
done

# Resolve entry to absolute path
if [ ! -f "$ENTRY" ]; then
    # Try relative to game/src/ directory
    if [ -f "$SRC_DIR/$ENTRY" ]; then
        ENTRY="$SRC_DIR/$ENTRY"
    else
        printf 'error: entry file not found: %s\n' "$ENTRY" >&2
        exit 1
    fi
fi

# Derive output base name from entry filename if not specified
if [ -z "$OUTPUT_NAME" ]; then
    OUTPUT_NAME="$(basename "$ENTRY" .cal)"
fi

# Check that the compiler exists
if [ ! -x "$CALYNDA" ]; then
    printf 'error: calynda compiler not found at %s\n' "$CALYNDA" >&2
    printf "run 'make -C compiler calynda' from the repo root first.\n" >&2
    exit 1
fi

# Ensure dist/ directory exists
mkdir -p "$DIST_DIR"

# If OUTPUT_NAME looks like a bare name (no slashes), put it in game/dist/
case "$OUTPUT_NAME" in
    */*)
        OUTPUT_BASE="$OUTPUT_NAME"
        ;;
    *)
        OUTPUT_BASE="$DIST_DIR/$OUTPUT_NAME"
        ;;
esac

OUTPUT_ELF="$OUTPUT_BASE.elf"
OUTPUT_DOL="$OUTPUT_BASE.dol"

printf 'Entry:  %s\n' "$ENTRY"
printf 'Target: wii\n'
printf 'Output: %s\n' "$OUTPUT_DOL"
printf '\n'

# Build — calynda places .elf and .dol alongside the -o path.
"$CALYNDA" build "$ENTRY" -o "$OUTPUT_BASE" --target wii

# The compiler outputs <name>.elf alongside the base name, and runs
# elf2dol to produce <name>.dol. Verify they exist.
if [ -f "$OUTPUT_DOL" ]; then
    printf '\nBuild successful!\n'
    printf '  ELF: %s\n' "$OUTPUT_ELF"
    printf '  DOL: %s\n' "$OUTPUT_DOL"
else
    printf '\nerror: expected output not found at %s\n' "$OUTPUT_DOL" >&2
    exit 1
fi
