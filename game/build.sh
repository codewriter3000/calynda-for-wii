#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
CALYNDA="$REPO_ROOT/compiler/build/calynda"

# Defaults
SRC_DIR="$SCRIPT_DIR/src"
DIST_DIR="$SCRIPT_DIR/dist"
ENTRY="$SRC_DIR/main.cal"
OUTPUT_NAME=""

usage() {
    echo "Usage: $(basename "$0") [options]"
    echo ""
    echo "Compiles Calynda source into .elf and .dol files for Wii."
    echo ""
    echo "Options:"
    echo "  -e, --entry FILE    Entry-point .cal file (default: game/src/main.cal)"
    echo "  -o, --output NAME   Base name for output files (default: game/dist/<entry>)"
    echo "  -h, --help          Show this help"
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -e|--entry)
            ENTRY="$2"
            shift 2
            ;;
        -o|--output)
            OUTPUT_NAME="$2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage
            ;;
    esac
done

# Resolve entry to absolute path
if [[ ! -f "$ENTRY" ]]; then
    # Try relative to game/src/ directory
    if [[ -f "$SRC_DIR/$ENTRY" ]]; then
        ENTRY="$SRC_DIR/$ENTRY"
    else
        echo "Error: entry file not found: $ENTRY" >&2
        exit 1
    fi
fi

# Derive output base name from entry filename if not specified
if [[ -z "$OUTPUT_NAME" ]]; then
    OUTPUT_NAME="$(basename "$ENTRY" .cal)"
fi

# Check that the compiler exists
if [[ ! -x "$CALYNDA" ]]; then
    echo "Error: calynda compiler not found at $CALYNDA" >&2
    echo "Run 'make -C compiler calynda' from the repo root first." >&2
    exit 1
fi

# Ensure dist/ directory exists
mkdir -p "$DIST_DIR"

# If OUTPUT_NAME looks like a bare name (no slashes), put it in game/dist/
if [[ "$OUTPUT_NAME" != */* ]]; then
    OUTPUT_BASE="$DIST_DIR/$OUTPUT_NAME"
else
    OUTPUT_BASE="$OUTPUT_NAME"
fi

OUTPUT_ELF="$OUTPUT_BASE.elf"
OUTPUT_DOL="$OUTPUT_BASE.dol"

echo "Entry:  $ENTRY"
echo "Target: wii"
echo "Output: $OUTPUT_DOL"
echo ""

# Build — calynda places .elf and .dol alongside the -o path.
"$CALYNDA" build "$ENTRY" -o "$OUTPUT_BASE" --target wii

# The compiler outputs <name>.elf alongside the base name, and runs
# elf2dol to produce <name>.dol. Verify they exist.
if [[ -f "$OUTPUT_DOL" ]]; then
    echo ""
    echo "Build successful!"
    echo "  ELF: $OUTPUT_ELF"
    echo "  DOL: $OUTPUT_DOL"
else
    echo ""
    echo "Error: expected output not found at $OUTPUT_DOL" >&2
    exit 1
fi
