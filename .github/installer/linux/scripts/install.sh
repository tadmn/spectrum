#!/usr/bin/env bash
#
# Installs the Spectrum CLAP and VST3 plugins for the current user on Linux.
#
# Usage:
#   ./install.sh              installs both CLAP and VST3
#   ./install.sh --clap       installs only the CLAP plugin
#   ./install.sh --vst3       installs only the VST3 plugin
#   ./install.sh --uninstall  removes previously installed plugins
#   ./install.sh --help       shows this message

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_NAME="Spectrum"

CLAP_SRC="${SCRIPT_DIR}/${PLUGIN_NAME}.clap"
VST3_SRC="${SCRIPT_DIR}/${PLUGIN_NAME}.vst3"

CLAP_DEST_DIR="${HOME}/.clap"
VST3_DEST_DIR="${HOME}/.vst3"

install_clap=true
install_vst3=true
uninstall=false

for arg in "$@"; do
  case "$arg" in
    --clap)
      install_vst3=false
      ;;
    --vst3)
      install_clap=false
      ;;
    --uninstall)
      uninstall=true
      ;;
    -h|--help)
      grep '^#' "$0" | sed 's/^#//'
      exit 0
      ;;
    *)
      echo "Unknown argument: $arg" >&2
      exit 1
      ;;
  esac
done

if [ "$uninstall" = true ]; then
  echo "Removing ${PLUGIN_NAME} plugins..."
  rm -rf "${CLAP_DEST_DIR:?}/${PLUGIN_NAME}.clap"
  rm -rf "${VST3_DEST_DIR:?}/${PLUGIN_NAME}.vst3"
  echo "Done."
  exit 0
fi

if [ "$install_clap" = true ]; then
  if [ ! -e "$CLAP_SRC" ]; then
    echo "Error: ${CLAP_SRC} not found next to this script." >&2
    exit 1
  fi
  mkdir -p "$CLAP_DEST_DIR"
  rm -rf "${CLAP_DEST_DIR:?}/${PLUGIN_NAME}.clap"
  cp -r "$CLAP_SRC" "$CLAP_DEST_DIR/"
  echo "Installed CLAP plugin to ${CLAP_DEST_DIR}/${PLUGIN_NAME}.clap"
fi

if [ "$install_vst3" = true ]; then
  if [ ! -e "$VST3_SRC" ]; then
    echo "Error: ${VST3_SRC} not found next to this script." >&2
    exit 1
  fi
  mkdir -p "$VST3_DEST_DIR"
  rm -rf "${VST3_DEST_DIR:?}/${PLUGIN_NAME}.vst3"
  cp -r "$VST3_SRC" "$VST3_DEST_DIR/"
  echo "Installed VST3 plugin to ${VST3_DEST_DIR}/${PLUGIN_NAME}.vst3"
fi

echo "Installation complete."