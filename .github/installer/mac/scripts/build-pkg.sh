#!/bin/bash
# Builds an unsigned .pkg installer for the Spectrum plugin bundles on macOS,
# with a choice screen letting the user pick which plugin formats to install.
# All formats are selected by default.
#
# Usage: ./scripts/build-pkg.sh <artifacts_dir> <output_pkg_path> <version> [build_version]
set -euo pipefail

ARTIFACTS_DIR="$1"   # e.g. Builds/spectrum_artifacts
OUTPUT_PKG="$2"      # e.g. Spectrum-macOS.pkg
VERSION="$3"         # e.g. 2.0.0 -- display version, shown in the installer title
BUILD_VERSION="${4:-$VERSION}"   # e.g. 2.0.0.29656318533 -- unique per-build version,
                                  # used for pkgbuild/pkg-ref versioning (upgrade detection).
                                  # Defaults to VERSION if not provided.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

# Strip symbols from the Mach-O binaries in a bundle to minimize install size.
# We operate on staged copies only, never the original artifacts.
strip_bundle_binaries() {
  local bundle_path="$1"
  find "$bundle_path" -type f | while read -r f; do
    if file "$f" | grep -q "Mach-O"; then
      echo "Stripping $f"
      # -x removes local (non-global) symbols, safe default for shipping plugin binaries.
      # Use -S to also strip debug symbol tables if present.
      strip -x -S "$f" || echo "  warning: strip failed on $f, leaving as-is"
    fi
  done
}

# Stages one plugin format into its own root, strips it, and builds a
# component .pkg for it. Component packages are what productbuild's
# distribution XML references as individually selectable choices.
#
# Args: <format_id> <bundle_name> <install_subdir>
#   format_id      short id used in identifiers/filenames, e.g. "vst3"
#   bundle_name    e.g. "Spectrum.vst3"
#   install_subdir e.g. "VST3" (relative to /Library/Audio/Plug-Ins)
build_component_pkg() {
  local format_id="$1"
  local bundle_name="$2"
  local install_subdir="$3"

  local staging="$WORK/root-$format_id"
  local scripts_staging="$WORK/scripts-$format_id"
  local install_dir="$staging/Library/Audio/Plug-Ins/$install_subdir"

  mkdir -p "$install_dir"
  mkdir -p "$scripts_staging"

  cp -R "$ARTIFACTS_DIR/$bundle_name" "$install_dir/"
  strip_bundle_binaries "$install_dir/$bundle_name"

  cp "$SCRIPT_DIR/postinstall" "$scripts_staging/postinstall"
  chmod +x "$scripts_staging/postinstall"

  pkgbuild \
    --root "$staging" \
    --scripts "$scripts_staging" \
    --identifier "com.tadmn.spectrum.installer.$format_id" \
    --version "$BUILD_VERSION" \
    --install-location "/" \
    "$WORK/spectrum-$format_id.pkg"
}

build_component_pkg "vst3" "Spectrum.vst3" "VST3"
build_component_pkg "au"   "Spectrum.component" "Components"
build_component_pkg "clap" "Spectrum.clap" "CLAP"

# Distribution XML drives productbuild's choice screen. Each <line> under
# <choices-outline> corresponds to a <choice> tied to one component package
# via <pkg-ref>. selected="true" makes it checked by default; the user can
# still uncheck it because it's not tagged with a "must be installed" trait.
# customize="always" forces the Customize/choice screen to show by default
# instead of being hidden behind an "Customize..." button click.
# No <welcome> element -> no custom Introduction text (the Introduction pane
# itself is always shown by Installer.app; there's no way to suppress it).
cat > "$WORK/distribution.xml" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>Spectrum ${VERSION}</title>
    <organization>com.tadmn.spectrum</organization>
    <domains enable_localSystem="true"/>
    <options customize="always" require-scripts="false" rootVolumeOnly="true"/>

    <choices-outline>
        <line choice="choice-vst3"/>
        <line choice="choice-au"/>
        <line choice="choice-clap"/>
    </choices-outline>

    <choice id="choice-vst3" title="VST3" description="Installs the VST3 plugin to /Library/Audio/Plug-Ins/VST3" start_selected="true">
        <pkg-ref id="com.tadmn.spectrum.installer.vst3"/>
    </choice>
    <choice id="choice-au" title="Audio Unit (AU)" description="Installs the Audio Unit component to /Library/Audio/Plug-Ins/Components" start_selected="true">
        <pkg-ref id="com.tadmn.spectrum.installer.au"/>
    </choice>
    <choice id="choice-clap" title="CLAP" description="Installs the CLAP plugin to /Library/Audio/Plug-Ins/CLAP" start_selected="true">
        <pkg-ref id="com.tadmn.spectrum.installer.clap"/>
    </choice>

    <pkg-ref id="com.tadmn.spectrum.installer.vst3" version="${BUILD_VERSION}" onConclusion="none">spectrum-vst3.pkg</pkg-ref>
    <pkg-ref id="com.tadmn.spectrum.installer.au" version="${BUILD_VERSION}" onConclusion="none">spectrum-au.pkg</pkg-ref>
    <pkg-ref id="com.tadmn.spectrum.installer.clap" version="${BUILD_VERSION}" onConclusion="none">spectrum-clap.pkg</pkg-ref>
</installer-gui-script>
EOF

productbuild \
  --distribution "$WORK/distribution.xml" \
  --package-path "$WORK" \
  "$OUTPUT_PKG"

echo "Built $OUTPUT_PKG (display version ${VERSION}, build version ${BUILD_VERSION})"