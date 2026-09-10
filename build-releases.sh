#!/usr/bin/env bash
# ==============================================================================
# CrossMyLa Release Build Script
# Builds all release firmware binaries and places them in 'release-version/'
# with distinct, descriptive filenames.
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

OUTPUT_DIR="release-version"
mkdir -p "$OUTPUT_DIR"

# Matrix of release targets:
# Format: <pio_environment>|<output_filename>|<hardware_description>
TARGETS=(
  "gh_release|firmware-x4-x3.bin|Xteink X4 and X3 (ESP32-C3)"
  "sticky-gh_release|firmware-sticky.bin|Seeed reTerminal Sticky (ESP32-S3)"
  "x4pro-gh_release|firmware-x4pro.bin|Xteink X4Pro (ESP32-S3)"
  "papermono-gh_release|firmware-papermono.bin|M5PaperMono (ESP32-S3)"
  "x4c-gh_release|firmware-x4c.bin|Xteink X4 Classic (ESP32-S3)"
)

echo "============================================================"
echo " Starting CrossMyLa Release Builds"
echo " Destination folder: $OUTPUT_DIR/"
echo "============================================================"

# Check if 'pio' command is available
if ! command -v pio &> /dev/null; then
  echo "Error: 'pio' command not found. Please ensure PlatformIO is installed and in PATH."
  exit 1
fi

FILTER="${1:-all}"
SUCCESS_COUNT=0
TOTAL_COUNT=0

for item in "${TARGETS[@]}"; do
  IFS="|" read -r ENV_NAME OUTPUT_NAME DESC <<< "$item"

  # If a specific target filter was passed, skip non-matching
  if [ "$FILTER" != "all" ] && [ "$FILTER" != "$ENV_NAME" ] && [[ "$OUTPUT_NAME" != *"$FILTER"* ]]; then
    continue
  fi

  TOTAL_COUNT=$((TOTAL_COUNT + 1))
  echo ""
  echo "------------------------------------------------------------"
  echo " Building: $ENV_NAME"
  echo " Target:   $DESC"
  echo " Output:   $OUTPUT_DIR/$OUTPUT_NAME"
  echo "------------------------------------------------------------"

  if pio run -e "$ENV_NAME"; then
    SRC_BIN=".pio/build/$ENV_NAME/firmware.bin"
    if [ -f "$SRC_BIN" ]; then
      cp "$SRC_BIN" "$OUTPUT_DIR/$OUTPUT_NAME"

      # For C3 default (gh_release), also maintain standard 'firmware.bin'
      # for full compatibility with GitHub Releases / OTA asset naming conventions.
      if [ "$ENV_NAME" = "gh_release" ]; then
        cp "$SRC_BIN" "$OUTPUT_DIR/firmware.bin"
      fi

      SIZE_BYTES=$(wc -c < "$OUTPUT_DIR/$OUTPUT_NAME" | tr -d ' ')
      SIZE_MB=$(awk "BEGIN {printf \"%.2f\", $SIZE_BYTES/1048576}")
      echo "✔ Successfully generated: $OUTPUT_DIR/$OUTPUT_NAME ($SIZE_MB MB / $SIZE_BYTES bytes)"
      SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
    else
      echo "❌ Error: Built binary not found at $SRC_BIN"
    fi
  else
    echo "❌ Build failed for environment: $ENV_NAME"
    exit 1
  fi
done

echo ""
echo "============================================================"
echo " Build Summary: $SUCCESS_COUNT / $TOTAL_COUNT release binaries created"
echo " Files in $OUTPUT_DIR/:"
echo "============================================================"
ls -lh "$OUTPUT_DIR"/*.bin

echo ""
echo "All release binaries are ready in: $SCRIPT_DIR/$OUTPUT_DIR"
