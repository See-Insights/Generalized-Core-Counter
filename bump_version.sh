#!/usr/bin/env bash

# Simple helper to bump firmware version, Doxygen project number,
# README version header, PRODUCT_VERSION, and append to the changelog.
# Release notes go to CHANGELOG.md only; they are no longer built into the binary.
#
# Usage:
#   ./bump_version.sh v28-CloseBeforeSleep "New feature release"
#
# Notes:
# - This script is tailored for macOS (uses BSD sed with -i '').
# - Escapes values for safe use in sed replacement.
# - Derives PRODUCT_VERSION from the version ("v28-CloseBeforeSleep" -> 28)

set -euo pipefail

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 <version> <release-notes>"
  exit 1
fi

VERSION="$1"
shift
NOTES="$*"

# Extract the integer PRODUCT_VERSION from the version string. Handles both
# the bare/point form ("4.00" -> 4, "4" -> 4) and the current release naming
# ("v28-CloseBeforeSleep" -> 28).
PRODUCT_VERSION_INT="$(printf '%s' "$VERSION" | sed -E 's/^v//; s/[.-].*$//')"

if ! printf '%s' "$PRODUCT_VERSION_INT" | grep -Eq '^[0-9]+$'; then
  echo "Error: cannot derive an integer PRODUCT_VERSION from '$VERSION'"
  exit 1
fi

sed_escape_replacement() {
  # Escapes characters that are special in the sed replacement part.
  # - '&' expands to the matched text
  # - '\\' is the escape character
  # - '/' is the delimiter we use
  # - '"' must be escaped because we embed the replacement in a double-quoted sed script
  printf '%s' "$1" | sed -e 's/[\\&/]/\\&/g' -e 's/"/\\"/g'
}

VERSION_ESCAPED="$(sed_escape_replacement "$VERSION")"
NOTES_ESCAPED="$(sed_escape_replacement "$NOTES")"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

FIRMWARE_VERSION_FILE="src/FirmwareVersion.h"
DOXYFILE="Doxyfile"
CHANGELOG="CHANGELOG.md"
README="README.md"

if [ ! -f "$FIRMWARE_VERSION_FILE" ]; then
  echo "Error: $FIRMWARE_VERSION_FILE not found"
  exit 1
fi

if [ ! -f "$DOXYFILE" ]; then
  echo "Error: $DOXYFILE not found"
  exit 1
fi

if [ ! -f "$README" ]; then
  echo "Error: $README not found"
  exit 1
fi

# Update the firmware version string and the product version, both in
# FirmwareVersion.h (release notes now live only in CHANGELOG.md).
sed -i '' "s/^inline const char\\* FIRMWARE_VERSION.*/inline const char* FIRMWARE_VERSION = \"${VERSION_ESCAPED}\";/" "$FIRMWARE_VERSION_FILE"

# Update FIRMWARE_PRODUCT_VERSION in FirmwareVersion.h
sed -i '' "s/^#define FIRMWARE_PRODUCT_VERSION.*/#define FIRMWARE_PRODUCT_VERSION ${PRODUCT_VERSION_INT}/" "$FIRMWARE_VERSION_FILE"

# Update Doxygen project number
sed -i '' "s/^PROJECT_NUMBER.*/PROJECT_NUMBER         = \"${VERSION_ESCAPED}\"/" "$DOXYFILE"

# Update README version line
sed -i '' "s/^\*\*Version:\*\*.*/\*\*Version:\*\* ${VERSION_ESCAPED} | \*\*Latest:\*\* ${NOTES_ESCAPED}/" "$README"

# Append to CHANGELOG.md
DATE="$(date +%Y-%m-%d)"
{
  echo "## ${VERSION} – ${DATE}"
  echo "- ${NOTES}"
  echo
} >> "$CHANGELOG"

echo "Updated to version ${VERSION}"