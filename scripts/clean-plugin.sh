#!/usr/bin/env bash
#
# clean_plugin.sh
# Removes all installed copies of an audio plugin (by name) from the
# common user & system plugin locations on macOS, Windows (Git Bash/MSYS),
# and Linux. Intended for dev use to avoid stale/duplicate installs.
#
# Usage:
#   ./clean_plugin.sh              # interactive, asks for confirmation
#   ./clean_plugin.sh -y           # no confirmation prompt
#   ./clean_plugin.sh -n           # dry run, just list what would be removed
#
# NOTE: system locations may require sudo/admin rights. On macOS/Linux run
# with `sudo` if you want system paths cleaned too. On Windows, run Git Bash
# "as Administrator".

set -uo pipefail

# ---- CONFIG ---------------------------------------------------------------
PLUGIN_NAME=Spectrum
# ----------------------------------------------------------------------------

# ---- Script location --------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"

DRY_RUN=false
ASSUME_YES=false

for arg in "$@"; do
  case "$arg" in
    -n|--dry-run) DRY_RUN=true ;;
    -y|--yes) ASSUME_YES=true ;;
    -h|--help)
      echo "Usage: $0 [-n|--dry-run] [-y|--yes]"
      exit 0
      ;;
  esac
done

# ---- OS detection -----------------------------------------------------------
UNAME_S="$(uname -s)"
case "$UNAME_S" in
  Darwin*) PLATFORM="macos" ;;
  Linux*)  PLATFORM="linux" ;;
  MINGW*|MSYS*|CYGWIN*) PLATFORM="windows" ;;
  *) echo "Unsupported OS: $UNAME_S"; exit 1 ;;
esac

# Helper: fetch a Windows env var that may contain parentheses, e.g. "ProgramFiles(x86)"
# Git Bash/MSYS uppercases "simple" (letters-only) Windows env var names when it
# imports the environment (e.g. ProgramFiles -> PROGRAMFILES), but leaves names
# containing special characters like "(x86)" in their original mixed case. So we
# try the name as given first, then fall back to an all-uppercase lookup.
win_env() {
  local val
  val="$(printenv "$1" 2>/dev/null)"
  if [ -z "$val" ]; then
    val="$(printenv "${1^^}" 2>/dev/null)"
  fi
  echo "$val"
}

# Helper: convert a Windows path (C:\Foo\Bar) to a unix-style path Git Bash can use
to_unix_path() {
  local p="$1"
  [ -z "$p" ] && return 0
  if command -v cygpath >/dev/null 2>&1; then
    cygpath -u "$p"
  else
    # crude fallback: C:\Foo\Bar -> /c/Foo/Bar
    echo "$p" | sed -e 's#\\#/#g' -e 's#^\([A-Za-z]\):#/\L\1#'
  fi
}

SEARCH_PATHS=()
EXTENSIONS=()

case "$PLATFORM" in
  macos)
    EXTENSIONS=(vst vst3 component clap aaxplugin)
    SEARCH_PATHS=(
      "$HOME/Library/Audio/Plug-Ins/VST"
      "$HOME/Library/Audio/Plug-Ins/VST3"
      "$HOME/Library/Audio/Plug-Ins/Components"
      "$HOME/Library/Audio/Plug-Ins/CLAP"
      "/Library/Audio/Plug-Ins/VST"
      "/Library/Audio/Plug-Ins/VST3"
      "/Library/Audio/Plug-Ins/Components"
      "/Library/Audio/Plug-Ins/CLAP"
      "/Library/Application Support/Avid/Audio/Plug-Ins"
    )
    ;;

  linux)
    EXTENSIONS=(vst vst3 clap lv2 so)
    SEARCH_PATHS=(
      "$HOME/.vst"
      "$HOME/.vst3"
      "$HOME/.clap"
      "$HOME/.lv2"
      "/usr/lib/vst"
      "/usr/lib/vst3"
      "/usr/lib/clap"
      "/usr/lib/lv2"
      "/usr/local/lib/vst"
      "/usr/local/lib/vst3"
      "/usr/local/lib/clap"
      "/usr/local/lib/lv2"
      "/usr/lib/x86_64-linux-gnu/vst3"
      "/usr/lib/x86_64-linux-gnu/clap"
      "/usr/lib/x86_64-linux-gnu/lv2"
    )
    ;;

  windows)
    EXTENSIONS=(vst3 clap dll aaxplugin)

    PROGRAMFILES_W="$(win_env ProgramFiles)"
    PROGRAMFILES_X86_W="$(win_env 'ProgramFiles(x86)')"
    COMMONPROGRAMFILES_W="$(win_env CommonProgramFiles)"
    COMMONPROGRAMFILES_X86_W="$(win_env 'CommonProgramFiles(x86)')"
    LOCALAPPDATA_W="$(win_env LOCALAPPDATA)"

    PROGRAMFILES_U="$(to_unix_path "$PROGRAMFILES_W")"
    PROGRAMFILES_X86_U="$(to_unix_path "$PROGRAMFILES_X86_W")"
    COMMONPROGRAMFILES_U="$(to_unix_path "$COMMONPROGRAMFILES_W")"
    COMMONPROGRAMFILES_X86_U="$(to_unix_path "$COMMONPROGRAMFILES_X86_W")"
    LOCALAPPDATA_U="$(to_unix_path "$LOCALAPPDATA_W")"

    SEARCH_PATHS=(
      "$COMMONPROGRAMFILES_U/VST3"
      "$COMMONPROGRAMFILES_X86_U/VST3"
      "$COMMONPROGRAMFILES_U/CLAP"
      "$COMMONPROGRAMFILES_X86_U/CLAP"
      "$COMMONPROGRAMFILES_U/Avid/Audio/Plug-Ins"
      "$LOCALAPPDATA_U/Programs/Common/VST3"
      "$LOCALAPPDATA_U/Programs/Common/CLAP"
      "$PROGRAMFILES_U/VstPlugins"
      "$PROGRAMFILES_X86_U/VstPlugins"
      "$PROGRAMFILES_U/Steinberg/VstPlugins"
      "$PROGRAMFILES_X86_U/Steinberg/VstPlugins"
    )
    ;;
esac

# ---- Local build output folders (all platforms) ------------------------------
# Also check for CLAP/VST3 artifacts produced by local builds sitting next to
# this script, e.g.:
#   ../build/artifacts/...
#   ../cmake-build-debug/artifacts/...
# The "*build*" glob matches any sibling directory whose name contains "build"
# (build, Build, my-build-debug, etc.). nullglob ensures the literal pattern
# is dropped (rather than searched for verbatim) if nothing matches.
#
# Plugin bundles can land directly in artifacts/ (macOS) or in
# artifacts/CLAP, artifacts/VST3 subfolders depending on the build setup, so
# these get searched recursively (BUILD_SEARCH_PATHS below), unlike the
# fixed OS install locations in SEARCH_PATHS which are only searched one
# level deep.
shopt -s nullglob
BUILD_SEARCH_PATHS=("$SCRIPT_DIR"/../*build*/artifacts)
shopt -u nullglob

# Build folders may contain CLAP/VST3 bundles even on platforms whose default
# EXTENSIONS list doesn't include them (shouldn't happen given the lists
# above, but keep this safe if EXTENSIONS is ever trimmed per-platform).
for ext in clap vst3; do
  if [[ ! " ${EXTENSIONS[*]} " == *" $ext "* ]]; then
    EXTENSIONS+=("$ext")
  fi
done

echo "Platform: $PLATFORM"
echo "Plugin name: $PLUGIN_NAME"
$DRY_RUN && echo "Mode: DRY RUN (nothing will be deleted)"
echo

# Helper: resolve a path to a clean absolute form (no "..", no "./"), without
# requiring GNU `realpath`/`readlink -f` (BSD readlink on macOS lacks -f).
# Works for files and directories (plugin bundles are directories).
norm_path() {
  local p="$1" dir base
  dir="$(cd "$(dirname "$p")" 2>/dev/null && pwd)" || { echo "$p"; return; }
  base="$(basename "$p")"
  echo "$dir/$base"
}

# ---- Find matches -----------------------------------------------------------
MATCHES=()

for dir in "${SEARCH_PATHS[@]}"; do
  [ -n "$dir" ] || continue
  [ -d "$dir" ] || continue
  for ext in "${EXTENSIONS[@]}"; do
    while IFS= read -r -d '' match; do
      MATCHES+=("$(norm_path "$match")")
    done < <(find "$dir" -maxdepth 1 -iname "${PLUGIN_NAME}.${ext}" -print0 2>/dev/null)
  done
done

for dir in "${BUILD_SEARCH_PATHS[@]}"; do
  [ -n "$dir" ] || continue
  [ -d "$dir" ] || continue
  for ext in "${EXTENSIONS[@]}"; do
    while IFS= read -r -d '' match; do
      MATCHES+=("$(norm_path "$match")")
    done < <(find "$dir" -iname "${PLUGIN_NAME}.${ext}" -print0 2>/dev/null)
  done
done

if [ "${#MATCHES[@]}" -eq 0 ]; then
  echo "No matches found for '$PLUGIN_NAME' in any known plugin location."
  exit 0
fi

echo "Found ${#MATCHES[@]} match(es):"
for m in "${MATCHES[@]}"; do
  echo "  $m"
done
echo

if $DRY_RUN; then
  echo "Dry run complete. No files were removed."
  exit 0
fi

if ! $ASSUME_YES; then
  read -r -p "Delete all of the above? [y/N] " reply
  case "$reply" in
    [yY]|[yY][eE][sS]) ;;
    *) echo "Aborted."; exit 0 ;;
  esac
fi

REMOVED=0
FAILED=0
for m in "${MATCHES[@]}"; do
  if rm -rf -- "$m" 2>/dev/null; then
    echo "Removed: $m"
    REMOVED=$((REMOVED + 1))
  else
    echo "FAILED to remove (permissions? try sudo/admin): $m"
    FAILED=$((FAILED + 1))
  fi
done

echo
echo "Done. Removed: $REMOVED, Failed: $FAILED"
if [ "$FAILED" -gt 0 ]; then
  echo "Some paths failed to delete. Re-run with sudo (macOS/Linux) or as Administrator (Windows) for system locations."
  exit 1
fi