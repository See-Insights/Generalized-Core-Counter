#!/usr/bin/env bash

# Simple helper to bump firmware version, Doxygen project number,
# README version header, PRODUCT_VERSION, and append to the changelog.
# Release notes go to CHANGELOG.md only; they are no longer built into the binary.
#
# Usage:
#   ./bump_version.sh 49.1-SomeName "New feature release"
#   ./bump_version.sh v49.1 "New feature release"
#
# Notes:
# - This script is tailored for macOS (uses BSD sed with -i '').
# - Escapes values for safe use in sed replacement.
# - Derives PRODUCT_VERSION from the version: <major>.<iteration>[-name] or
#   v<major>.<iteration>[-name] -> major * 100 + iteration ("49.1" and
#   "v49.1-SomeName" -> 4901). The legacy dotless form ("v28-CloseBeforeSleep"
#   -> 28) still works.

set -euo pipefail

# Derive the integer PRODUCT_VERSION from the version string.
#   [v]<major>.<iteration>[-name]  ->  major * 100 + iteration  (1-2 digits each)
#                                      "49.1" -> 4901, "v49.1-Foo" -> 4901, "v48.90" -> 4890
#   v<NN>[-name]   LEGACY, no dot  ->  NN   ("v28-CloseBeforeSleep" -> 28, "v49" -> 49)
# Anything else is rejected. Prints the integer; returns 1 on rejection.
derive_product_version() {
  local re_new='^v?([0-9]{1,2})\.([0-9]{1,2})(-.+)?$'
  local re_legacy='^v([0-9]{1,2})(-.+)?$'
  if [[ "$1" =~ $re_new ]]; then
    echo $((10#${BASH_REMATCH[1]} * 100 + 10#${BASH_REMATCH[2]}))
  elif [[ "$1" =~ $re_legacy ]]; then
    echo $((10#${BASH_REMATCH[1]}))
  else
    return 1
  fi
}

if [ "$#" -lt 2 ]; then
  echo "Usage: $0 <[v]major.iteration[-name]> <release-notes>   (e.g. 49.1 or v49.1-Name)"
  exit 1
fi

VERSION="$1"
shift
NOTES="$*"

if ! PRODUCT_VERSION_INT="$(derive_product_version "$VERSION")"; then
  echo "Error: '$VERSION' is not [v]<major>.<iteration>[-name] (1-2 digits each; legacy: v<NN>[-name])"
  exit 1
fi

# The firmware string always reads v<major>.<iteration>[-name], even when called as 49.1.
case "$VERSION" in v*) ;; *) VERSION="v${VERSION}" ;; esac

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

# Insert the release into CHANGELOG.md, newest first: directly after the
# "## Unreleased" section (before the first "## [" release heading), in the
# file's bracketed format. Falls back to appending if there is no release
# heading yet. The entry travels via the environment so awk does not
# reinterpret backslashes in the notes.
DATE="$(date +%Y-%m-%d)"
CHANGELOG_ENTRY="$(printf '## [%s] - %s\n\n### Changed\n\n- %s\n' "$VERSION" "$DATE" "$NOTES")"
CHANGELOG_TMP="$(mktemp)"
CHANGELOG_ENTRY="$CHANGELOG_ENTRY" awk '
  !inserted && /^## \[/ { print ENVIRON["CHANGELOG_ENTRY"]; print ""; inserted = 1 }
  { print }
  END { if (!inserted) { print ""; print ENVIRON["CHANGELOG_ENTRY"] } }
' "$CHANGELOG" > "$CHANGELOG_TMP"
mv "$CHANGELOG_TMP" "$CHANGELOG"

echo "Updated to version ${VERSION}"