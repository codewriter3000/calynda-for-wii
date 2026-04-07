#!/bin/bash
set -e

EXTENSION_DIR="$HOME/.vscode-server/extensions/local.calynda-syntax-0.1.0"

# Use local VS Code extensions dir if not in a remote/container environment
if [ ! -d "$HOME/.vscode-server" ]; then
  EXTENSION_DIR="$HOME/.vscode/extensions/local.calynda-syntax-0.1.0"
fi

echo "Installing Calynda syntax extension to $EXTENSION_DIR ..."

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

rm -rf "$EXTENSION_DIR"
mkdir -p "$EXTENSION_DIR/syntaxes"

cp "$SCRIPT_DIR/package.json" "$EXTENSION_DIR/"
cp "$SCRIPT_DIR/language-configuration.json" "$EXTENSION_DIR/"
cp "$SCRIPT_DIR/syntaxes/"* "$EXTENSION_DIR/syntaxes/"
[ -f "$SCRIPT_DIR/README.md" ] && cp "$SCRIPT_DIR/README.md" "$EXTENSION_DIR/"

echo "Done. Reload VS Code to activate the extension."
